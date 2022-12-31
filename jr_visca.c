/*
    Copyright 2021 Jacob Rau
    
    This file is part of libjr_visca.

    libjr_visca is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    libjr_visca is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libjr_visca.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "jr_visca.h"

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct {
    uint8_t sender;
    uint8_t receiver;
    uint8_t data[JR_VISCA_MAX_ENCODED_MESSAGE_DATA_LENGTH - 2];
    uint8_t dataLength;
} jr_viscaFrame;

/**
 * Extract a frame from the given buffer.
 * 
 * `data` is a buffer containing VISCA data. It can be truncated or contain
 * multiple frames.
 * `dataLength` is the count of bytes in `data`.
 * 
 * If at least one full frame is present, it will be written to `frame`.
 * 
 * If less than one full frame is present in `buffer`, returns `0`.
 * 
 * If data corruption is detected (e.g. too many bytes occur before the end-of-frame marker),
 * returns `-1`.
 */
int jr_viscaDataToFrame(uint8_t *data, int dataLength, jr_viscaFrame *frame) {
    // We only decode a frame if the entire frame is present, i.e. 0xff terminator is present.
    int terminatorIndex;
    for (terminatorIndex = 0; terminatorIndex < dataLength; terminatorIndex++) {
        if (data[terminatorIndex] == 0xff) {
            break;
        }
    }

    // If we didn't find a terminator, the index will == dataLength.
    if (!(terminatorIndex < dataLength)) {
        // No bytes consumed, since we're waiting for more bytes to arrive.
        return 0;
    }

    if (terminatorIndex > JR_VISCA_MAX_ENCODED_MESSAGE_DATA_LENGTH - 2 - 1) {
        // All our internal buffers are fixed-length. If the frame exceeds that length, bail.
        return -1;
    }

    if (terminatorIndex == 0) {
        // If no header present, flag an error.
        return -1;
    }

    // First byte is header containing sender and receiver addresses.
    // Except for Address Set (aka Camera Number) and IFClear(Broadcast), which are 0x88, but they don't apply to visca over IP.
    frame->sender = (data[0] >> 4) & 0x7;
    frame->receiver = data[0] & 0xF;

    // N bytes of packet data between header byte and 0xff terminator.
    memcpy(frame->data, data + 1, terminatorIndex - 1);

    frame->dataLength = terminatorIndex - 1;

    return terminatorIndex + 1;
}

int jr_viscaFrameToData(uint8_t *data, int dataLength, jr_viscaFrame frame) {
    if (frame.dataLength + 2 > dataLength) {
        return -1;
    }

    if ((frame.sender > 7) || (frame.receiver > 0xF)) {
        return -1;
    }

    data[0] = 0x80 + (frame.sender << 4) + frame.receiver;
    memcpy(data + 1, frame.data, frame.dataLength);
    data[frame.dataLength + 1] = 0xff;
    return frame.dataLength + 2;
}

typedef struct {
    uint8_t signature[JR_VISCA_MAX_ENCODED_MESSAGE_DATA_LENGTH - 2];
    uint8_t signatureMask[JR_VISCA_MAX_ENCODED_MESSAGE_DATA_LENGTH - 2];
    int signatureLength;
    int commandType;
    void (*handleParameters)(jr_viscaFrame* frame, union jr_viscaMessageParameters *messageParameters, bool isDecodingFrame);
} jr_viscaMessageDefinition;

/**
 * `buffer` looks like 0x01 0x02 0x03 0x04
 * Returned result will look like 0x1234
 * This is a common way for VISCA to bit pack things.
 */
int16_t _jr_viscaRead16FromBuffer(uint8_t *buffer) {
    int16_t result = 0;
    result += (buffer[0] & 0xf) * 0x1000;
    result += (buffer[1] & 0xf) * 0x100;
    result += (buffer[2] & 0xf) * 0x10;
    result += (buffer[3] & 0xf);
    return result;
}

/**
 * Given `value` looks like 0x1234
 * `buffer` will look like 0x01 0x02 0x03 0x04
 * We won't touch the upper nibble of each byte-- it may be significant per the specific comand.
 */
void _jr_viscaWrite16ToBuffer(int16_t value, uint8_t *buffer) {
    buffer[0] |= (value >> 12) & 0xf;
    buffer[1] |= (value >> 8) & 0xf;
    buffer[2] |= (value >> 4) & 0xf;
    buffer[3] |= value & 0xf;
}

void jr_visca_handlePanTiltPositionInqResponseParameters(jr_viscaFrame* frame, union jr_viscaMessageParameters *messageParameters, bool isDecodingFrame) {
    if (isDecodingFrame) {
        messageParameters->panTiltPositionInqResponseParameters.panPosition = _jr_viscaRead16FromBuffer(frame->data + 1);
        messageParameters->panTiltPositionInqResponseParameters.tiltPosition = _jr_viscaRead16FromBuffer(frame->data + 5);
    } else {
        _jr_viscaWrite16ToBuffer(messageParameters->panTiltPositionInqResponseParameters.panPosition, frame->data + 1);
        _jr_viscaWrite16ToBuffer(messageParameters->panTiltPositionInqResponseParameters.tiltPosition, frame->data + 5);
    }
}

// AbsolutePosition [81] 01 06 02 [3]VV [4]WW [5]0Y 0Y 0Y 0Y [9]0Z 0Z 0Z 0Z FF
// VV: Pan speed 0x01 (low speed) to 0x18 (high speed)
// WW: Tilt speed 0x01 (low speed) to 0x14 (high speed)
// YYYY: Pan Position
// ZZZZ: Tilt Position
void jr_visca_handleAbsolutePanTiltPositionParameters(jr_viscaFrame* frame, union jr_viscaMessageParameters *messageParameters, bool isDecodingFrame) {
    if (isDecodingFrame) {
        messageParameters->absolutePanTiltPositionParameters.panSpeed = frame->data[3] & 0xf;
        messageParameters->absolutePanTiltPositionParameters.tiltSpeed = frame->data[4] & 0xf;
        messageParameters->absolutePanTiltPositionParameters.panPosition = _jr_viscaRead16FromBuffer(frame->data + 5);
        messageParameters->absolutePanTiltPositionParameters.tiltPosition = _jr_viscaRead16FromBuffer(frame->data + 9);
    } else {
        frame->data[3] = messageParameters->absolutePanTiltPositionParameters.panSpeed;
        frame->data[4] = messageParameters->absolutePanTiltPositionParameters.tiltSpeed;
        _jr_viscaWrite16ToBuffer(messageParameters->absolutePanTiltPositionParameters.panPosition, frame->data + 5);
        _jr_viscaWrite16ToBuffer(messageParameters->absolutePanTiltPositionParameters.tiltPosition, frame->data + 9);
    }
}

void jr_visca_handleZoomPositionInqResponseParameters(jr_viscaFrame* frame, union jr_viscaMessageParameters *messageParameters, bool isDecodingFrame) {
    if (isDecodingFrame) {
        messageParameters->zoomPositionParameters.zoomPosition = _jr_viscaRead16FromBuffer(frame->data + 1);
    } else {
        _jr_viscaWrite16ToBuffer(messageParameters->zoomPositionParameters.zoomPosition, frame->data + 1);
    }
}

void jr_visca_handleAckCompletionParameters(jr_viscaFrame* frame, union jr_viscaMessageParameters *messageParameters, bool isDecodingFrame) {
    if (isDecodingFrame) {
        messageParameters->ackCompletionParameters.socketNumber = frame->data[0] & 0xf;
    } else {
        frame->data[0] += messageParameters->ackCompletionParameters.socketNumber;
    }
}

void jr_visca_handleCameraNumberParameters(jr_viscaFrame* frame, union jr_viscaMessageParameters *messageParameters, bool isDecodingFrame) {
    // Request: 88 30 01 FF, reply: 88 30 0w FF, w is 2-8 (camera+1)
    if (isDecodingFrame) {
        messageParameters->cameraNumberParameters.cameraNum = frame->data[1] & 0xf;
    } else {
        frame->data[1] += messageParameters->cameraNumberParameters.cameraNum;
    }
}

void jr_visca_handleZoomVariableParameters(jr_viscaFrame* frame, union jr_viscaMessageParameters *messageParameters, bool isDecodingFrame) {
    if (isDecodingFrame) {
        messageParameters->zoomVariableParameters.zoomSpeed = frame->data[3] & 0xf;
    } else {
        frame->data[3] += messageParameters->zoomVariableParameters.zoomSpeed;
    }
}

void jr_visca_handlePresetSpeedParameters(jr_viscaFrame* frame, union jr_viscaMessageParameters *messageParameters, bool isDecodingFrame) {
    if (isDecodingFrame) {
        uint8_t speed = frame->data[3] & 0xff;
        speed = (speed < 1) ? 1 : ((speed > 0x18) ? 0x18 : speed);
        messageParameters->presetSpeedParameters.presetSpeed = speed;
    } else {
        frame->data[3] = messageParameters->presetSpeedParameters.presetSpeed;
    }
}

void jr_visca_handleMemoryParameters(jr_viscaFrame* frame, union jr_viscaMessageParameters *messageParameters, bool isDecodingFrame) {
    if (isDecodingFrame) {
        messageParameters->memoryParameters.memory = frame->data[4] & 0xff;
        messageParameters->memoryParameters.mode = frame->data[3] & 0xff;
    } else {
        frame->data[4] = messageParameters->memoryParameters.memory;
    }
}

void jr_visca_handleZoomDirectParameters(jr_viscaFrame* frame, union jr_viscaMessageParameters *messageParameters, bool isDecodingFrame) {
    if (isDecodingFrame) {
        messageParameters->zoomPositionParameters.zoomPosition = _jr_viscaRead16FromBuffer(frame->data + 3);
    } else {
        _jr_viscaWrite16ToBuffer(messageParameters->zoomPositionParameters.zoomPosition, frame->data + 3);
    }
}

void jr_visca_handlePanTiltDriveParameters(jr_viscaFrame* frame, union jr_viscaMessageParameters *messageParameters, bool isDecodingFrame) {
    if (isDecodingFrame) {
        messageParameters->panTiltDriveParameters.panDirection = frame->data[5];
        messageParameters->panTiltDriveParameters.tiltDirection = frame->data[6];
        messageParameters->panTiltDriveParameters.panSpeed = frame->data[3];
        messageParameters->panTiltDriveParameters.tiltSpeed = frame->data[4];
    } else {
        frame->data[3] = messageParameters->panTiltDriveParameters.panSpeed;
        frame->data[4] = messageParameters->panTiltDriveParameters.tiltSpeed;
        frame->data[5] = messageParameters->panTiltDriveParameters.panDirection;
        frame->data[6] = messageParameters->panTiltDriveParameters.tiltDirection;
    }
}

jr_viscaMessageDefinition definitions[] = {
    {
        {0x09, 0x06, 0x12}, //signature
        {0xff, 0xff, 0xff}, //signatureMask
        3, //signatureLength
        JR_VISCA_MESSAGE_PAN_TILT_POSITION_INQ, //commandType
        NULL //handleParameters
    },
    {
        // pan (signed) = 0xstuv
        // tilt (signed) = 0xwxyz
        //        s     t     u     v     w     y     x     z
        {0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0xff, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0},
        9,
        JR_VISCA_MESSAGE_PAN_TILT_POSITION_INQ_RESPONSE,
        &jr_visca_handlePanTiltPositionInqResponseParameters
    },
    {
        {0x09, 0x04, 0x47},
        {0xff, 0xff, 0xff},
        3,
        JR_VISCA_MESSAGE_ZOOM_POSITION_INQ,
        NULL
    },
    {
        {0x50, 0x00, 0x00, 0x00, 0x00},
        {0xff, 0xf0, 0xf0, 0xf0, 0xf0},
        5,
        JR_VISCA_MESSAGE_ZOOM_POSITION_INQ_RESPONSE,
        &jr_visca_handleZoomPositionInqResponseParameters
    },
    {
        {0x01, 0x04, 0x38, 0x02},
        {0xff, 0xff, 0xff, 0xff},
        4,
        JR_VISCA_MESSAGE_FOCUS_AUTOMATIC,
        NULL
    },
    {
        {0x01, 0x04, 0x38, 0x03},
        {0xff, 0xff, 0xff, 0xff},
        4,
        JR_VISCA_MESSAGE_FOCUS_MANUAL,
        NULL
    },
    {
        {0x40},
        {0xf0},
        1,
        JR_VISCA_MESSAGE_ACK,
        &jr_visca_handleAckCompletionParameters
    },
    {
        {0x50},
        {0xf0},
        1,
        JR_VISCA_MESSAGE_COMPLETION,
        &jr_visca_handleAckCompletionParameters
    },
    {
        {0x01, 0x04, 0x07, 0x00},
        {0xff, 0xff, 0xff, 0xff},
        4,
        JR_VISCA_MESSAGE_ZOOM_STOP,
        NULL
    },
    {
        {0x01, 0x04, 0x07, 0x02},
        {0xff, 0xff, 0xff, 0xff},
        4,
        JR_VISCA_MESSAGE_ZOOM_TELE_STANDARD,
        NULL
    },
    {
        {0x01, 0x04, 0x07, 0x03},
        {0xff, 0xff, 0xff, 0xff},
        4,
        JR_VISCA_MESSAGE_ZOOM_WIDE_STANDARD,
        NULL
    },
    {
        {0x01, 0x04, 0x07, 0x20},
        {0xff, 0xff, 0xff, 0xf0},
        4,
        JR_VISCA_MESSAGE_ZOOM_TELE_VARIABLE,
        &jr_visca_handleZoomVariableParameters
    },
    {
        {0x01, 0x04, 0x07, 0x30},
        {0xff, 0xff, 0xff, 0xf0},
        4,
        JR_VISCA_MESSAGE_ZOOM_WIDE_VARIABLE,
        &jr_visca_handleZoomVariableParameters
    },
    {
        {0x01, 0x04, 0x47, 0x00, 0x00, 0x00, 0x00},
        {0xff, 0xff, 0xff, 0xf0, 0xf0, 0xf0, 0xf0},
        7,
        JR_VISCA_MESSAGE_ZOOM_DIRECT,
        &jr_visca_handleZoomDirectParameters
    },
    {
        {0x01, 0x06, 0x01, 0x00, 0x00, 0x00, 0x00},
        {0xff, 0xff, 0xff, 0xe0, 0xe0, 0xf0, 0xf0},
        7,
        JR_VISCA_MESSAGE_PAN_TILT_DRIVE,
        &jr_visca_handlePanTiltDriveParameters
    },
    {
        {0x30, 0x01},
        {0xff, 0xff},
        2,
        JR_VISCA_MESSAGE_CAMERA_NUMBER,
        &jr_visca_handleCameraNumberParameters
    },
    {
        {0x01, 0x04, 0x3f, 0x00, 0x00},
        {0xff, 0xff, 0xff, 0x00, 0x00},
        5,
        JR_VISCA_MESSAGE_MEMORY,
        &jr_visca_handleMemoryParameters
    },
    {
        {0x01, 0x00, 0x01},
        {0xff, 0xff, 0xff},
        3,
        JR_VISCA_MESSAGE_CLEAR,
        NULL
    },
    {   // 01 06 01 pp
        {0x01, 0x06, 0x01, 0x00},
        {0xff, 0xff, 0xff, 0x00},
        4,
        JR_VISCA_MESSAGE_PRESET_RECALL_SPEED,
        &jr_visca_handlePresetSpeedParameters
    },
    {   // 01 06 02        VV    WW     0Y 0Y 0Y 0Y              0Z 0Z 0Z 0Z
        {0x01, 0x06, 0x02, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00},
        {0xff, 0xff, 0xff, 0x00, 0x00,  0xf0, 0xf0, 0xf0, 0xf0,  0xf0, 0xf0, 0xf0, 0xf0},
        13,
        JR_VISCA_MESSAGE_ABSOLUTE_PAN_TILT,
        &jr_visca_handleAbsolutePanTiltPositionParameters
    },
    {   // Home 81 01 06 04 FF
        {0x01, 0x06, 0x04},
        {0xff, 0xff, 0xff},
        3,
        JR_VISCA_MESSAGE_HOME,
        NULL
    },
    {   // Reset 81 01 06 05 FF
        {0x01, 0x06, 0x05},
        {0xff, 0xff, 0xff},
        3,
        JR_VISCA_MESSAGE_RESET,
        NULL
    },
    {   // Cancel 81 2z FF - supported by some cameras but apparently not PTZOptics, which returns syntax error instead of cancel reply. But it does interrupt the current operation.
        {0x20},
        {0xf0},
        1,
        JR_VISCA_MESSAGE_CANCEL,
        NULL
    },
    {
        {0x60, 0x04},
        {0xf0, 0xff},
        2,
        JR_VISCA_MESSAGE_CANCEL_REPLY,
        &jr_visca_handleAckCompletionParameters
    },
    { {}, {}, 0, 0, NULL} // Final definition must have `signatureLength` == 0.
};

void _jr_viscahex_print(char *buf, int buf_size) {
    for (int i = 0; i < buf_size; i++) {
        printf("%02hhx ", buf[i]);
    }
}

void _jr_viscaMemAnd(uint8_t *a, uint8_t *b, uint8_t *output, int length) {
    for (int i = 0; i < length; i++) {
        output[i] = a[i] & b[i];
    }
}

int jr_viscaDecodeFrame(jr_viscaFrame frame, union jr_viscaMessageParameters *messageParameters) {
    int i = 0;
    while (definitions[i].signatureLength) {
        uint8_t maskedFrame[JR_VISCA_MAX_ENCODED_MESSAGE_DATA_LENGTH - 2];
        _jr_viscaMemAnd(frame.data, definitions[i].signatureMask, maskedFrame, frame.dataLength);
        if (memcmp(maskedFrame, definitions[i].signature, definitions[i].signatureLength) == 0) {
            if (definitions[i].handleParameters != NULL) {
                definitions[i].handleParameters(&frame, messageParameters, true);
            }
            return definitions[i].commandType;
        }
#ifdef VERBOSE_DEF
         printf("definition %d: sig: ", i);
         _jr_viscahex_print(definitions[i].signature, definitions[i].signatureLength);
         printf(" sigmask: ");
         _jr_viscahex_print(definitions[i].signatureMask, definitions[i].signatureLength);
         printf("\n");
#endif
        i++;
    }

    return -1;
}

int jr_viscaEncodeFrame(int messageType, union jr_viscaMessageParameters messageParameters, jr_viscaFrame *frame) {
    int i = 0;
    while (definitions[i].signatureLength) {
        if (messageType == definitions[i].commandType) {
            memcpy(frame->data, definitions[i].signature, definitions[i].signatureLength);
            frame->dataLength = definitions[i].signatureLength;
            if (definitions[i].handleParameters != NULL) {
                definitions[i].handleParameters(frame, &messageParameters, false);
            }
            return 0;
        }
        i++;
    }

    return -1;
}

int jr_viscaDecodeMessage(uint8_t *data, int dataLength, int *message, union jr_viscaMessageParameters *messageParameters, uint8_t *sender, uint8_t *receiver) {
    jr_viscaFrame frame;
    int consumedBytes = jr_viscaDataToFrame(data, dataLength, &frame);
    if (consumedBytes <= 0) {
        return consumedBytes;
    }

    *message = jr_viscaDecodeFrame(frame, messageParameters);
    *sender = frame.sender;
    *receiver = frame.receiver;

    return consumedBytes;
}

int jr_viscaEncodeMessage(uint8_t *data, int dataLength, int message, union jr_viscaMessageParameters messageParameters, uint8_t sender, uint8_t receiver) {
    jr_viscaFrame frame;
    frame.sender = sender;
    frame.receiver = receiver;
    if (jr_viscaEncodeFrame(message, messageParameters, &frame) < 0) {
        return -1;
    }

    return jr_viscaFrameToData(data, dataLength, frame);
}
