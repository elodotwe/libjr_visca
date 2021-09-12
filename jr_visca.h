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

#ifndef JR_VISCA_H
#define JR_VISCA_H

#include <stdint.h>

#define JR_VISCA_MAX_FRAME_DATA_LENGTH 16

typedef struct {
    uint8_t sender;
    uint8_t receiver;
    uint8_t data[JR_VISCA_MAX_FRAME_DATA_LENGTH];
    uint8_t dataLength;
} jr_viscaFrame;

/**
 * Find a VISCA frame at the beginning of the given buffer.
 * 
 * `data` is the buffer to decode
 * `dataLength` is the number of bytes contained in `data`
 * 
 * If a full frame is found in `data`, `jr_viscaDataToFrame` will write
 * the found frame to `frame`, and return the number of bytes consumed from
 * `data`.
 * 
 * If a full frame is not found, returns 0.
 */
int jr_viscaDataToFrame(uint8_t *data, int dataLength, jr_viscaFrame *frame);

/**
 * Convert `frame` into a buffer to be sent.
 * 
 * Returns the actual number of bytes written on success, or
 * -1 on failure (e.g. given buffer is too short).
 */
int jr_viscaFrameToData(uint8_t *data, int dataLength, jr_viscaFrame frame);

#define JR_VISCA_MESSAGE_PAN_TILT_POSITION_INQ 1
#define JR_VISCA_MESSAGE_PAN_TILT_POSITION_INQ_RESPONSE 2
#define JR_VISCA_MESSAGE_ZOOM_POSITION_INQ 3
#define JR_VISCA_MESSAGE_ZOOM_POSITION_INQ_RESPONSE 4
#define JR_VISCA_MESSAGE_FOCUS_AUTOMATIC 5
#define JR_VISCA_MESSAGE_FOCUS_MANUAL 6

#define JR_VISCA_MESSAGE_ACK 7
#define JR_VISCA_MESSAGE_COMPLETION 8

#define JR_VISCA_MESSAGE_ZOOM_STOP 9
#define JR_VISCA_MESSAGE_ZOOM_TELE_STANDARD 10
#define JR_VISCA_MESSAGE_ZOOM_WIDE_STANDARD 11
#define JR_VISCA_MESSAGE_ZOOM_TELE_VARIABLE 12
#define JR_VISCA_MESSAGE_ZOOM_WIDE_VARIABLE 13
#define JR_VISCA_MESSAGE_ZOOM_DIRECT 14

struct jr_viscaPanTiltPositionInqResponseParameters {
    int16_t panPosition;
    int16_t tiltPosition;
};

struct jr_viscaZoomPositionParameters {
    int16_t zoomPosition;
};

struct jr_viscaAckCompletionParameters {
    uint8_t socketNumber;
};

struct jr_viscaZoomVariableParameters {
    // 0-7, 0=slowest, 7=fastest
    uint8_t zoomSpeed;
};

union jr_viscaMessageParameters
{
    struct jr_viscaPanTiltPositionInqResponseParameters panTiltPositionInqResponseParameters;
    struct jr_viscaZoomPositionParameters zoomPositionParameters;
    struct jr_viscaZoomVariableParameters zoomVariableParameters;
    struct jr_viscaAckCompletionParameters ackCompletionParameters;
};

/**
 * Decode a frame into a message.
 * 
 * Returns a `JR_VISCA_MESSAGE_*` value if the message was recognized,
 * or -1 if the message was not recognized.
 * 
 * `frame` is the frame to be decoded, previously obtained from `jr_viscaDataToFrame()`
 * If the message decoded from `frame` contains any parameters (not all messages do),
 * `messageParameters` will have said parameters written to it.
 */
int jr_viscaDecodeFrame(jr_viscaFrame frame, union jr_viscaMessageParameters *messageParameters);

/**
 * Encode a message into a frame.
 * 
 * `messageType` should be one of the `JR_VISCA_MESSAGE_*` values, indicating which
 * message is to be encoded.
 * 
 * `messageParameters` should have the member corresponding to the given `messageType` set,
 * if one exists (if there is no corresponding `messageParameters` member, `messageParameters` is
 * ignored for that message).
 * 
 * The encoded frame will be written to `frame`.
 * 
 * Returns 0 on success, -1 on failure.
 */
int jr_viscaEncodeFrame(int messageType, union jr_viscaMessageParameters messageParameters, jr_viscaFrame *frame);

#endif