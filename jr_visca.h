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

#define JR_VISCA_MAX_ENCODED_MESSAGE_DATA_LENGTH 18

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

#define JR_VISCA_MESSAGE_PAN_TILT_DRIVE 15

#define JR_VISCA_MESSAGE_CAMERA_NUMBER 16

#define JR_VISCA_MESSAGE_MEMORY 17

#define JR_VISCA_MESSAGE_CLEAR 18

#define JR_VISCA_MESSAGE_PRESET_RECALL_SPEED 19

#define JR_VISCA_MESSAGE_ABSOLUTE_PAN_TILT 20

#define JR_VISCA_MESSAGE_HOME 21

#define JR_VISCA_MESSAGE_RESET 22

#define JR_VISCA_MESSAGE_CANCEL 23

#define JR_VISCA_MESSAGE_CANCEL_REPLY 24

struct jr_viscaPanTiltPositionInqResponseParameters {
    int16_t panPosition;
    int16_t tiltPosition;
};

// AbsolutePosition 81 01 06 02 VV WW 0Y 0Y 0Y 0Y 0Z 0Z 0Z 0Z FF
// VV: Pan speed 0x01 (low speed) to 0x18 (high speed)
// WW: Tilt speed 0x01 (low speed) to 0x14 (high speed)
// YYYY: Pan Position
// ZZZZ: Tilt Position
struct jr_viscaAbsolutePanTiltPositionParameters {
    int16_t panPosition;
    int16_t tiltPosition;
    uint8_t panSpeed;
    uint8_t tiltSpeed;
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


struct jr_viscaPresetSpeedParameters {
    // 1-0x18
    uint8_t presetSpeed;
};

struct jr_viscaCameraNumberParameters {
    // 1-7
    uint8_t cameraNum;
};


struct jr_viscaMemoryParameters {
    // 1-127
    uint8_t memory;
    // 0=reset, 1=set, 2=recall
    uint8_t mode;
};

#define JR_VISCA_TILT_DIRECTION_UP 1
#define JR_VISCA_TILT_DIRECTION_DOWN 2
#define JR_VISCA_TILT_DIRECTION_STOP 3

#define JR_VISCA_PAN_DIRECTION_LEFT 1
#define JR_VISCA_PAN_DIRECTION_RIGHT 2
#define JR_VISCA_PAN_DIRECTION_STOP 3

#define JR_VISCA_MEMORY_MODE_RESET 0
#define JR_VISCA_MEMORY_MODE_SET 1
#define JR_VISCA_MEMORY_MODE_RECALL 2

struct jr_viscaPanTiltDriveParameters {
    uint8_t panSpeed; // 1-0x18
    uint8_t tiltSpeed; // 1-0x14
    uint8_t panDirection; // JR_VISCA_PAN_DIRECTION_*
    uint8_t tiltDirection; // JR_VISCA_TILT_DIRECTION_*
};

union jr_viscaMessageParameters
{
    struct jr_viscaPanTiltPositionInqResponseParameters panTiltPositionInqResponseParameters;
    struct jr_viscaZoomPositionParameters zoomPositionParameters;
    struct jr_viscaZoomVariableParameters zoomVariableParameters;
    struct jr_viscaAckCompletionParameters ackCompletionParameters;
    struct jr_viscaPanTiltDriveParameters panTiltDriveParameters;
    struct jr_viscaCameraNumberParameters cameraNumberParameters;
    struct jr_viscaMemoryParameters memoryParameters;
    struct jr_viscaPresetSpeedParameters presetSpeedParameters;
    struct jr_viscaAbsolutePanTiltPositionParameters absolutePanTiltPositionParameters;
};

/**
 * Decodes the first message from `data` into `message` and `messageParameters`.
 * 
 * Returns the byte count of the decoded message, or 0 if the buffer does not contain a complete message.
 * 
 * If the buffer contains at least one complete message (i.e. the return value is greater than 0):
 *  - `message` will either be set to -1 (unrecognized message) or one of the `JR_VISCA_MESSAGE_*` constants.
 *  - `messageParameters` will have the corresponding parameters set as appropriate for the detected message type.
 */
int jr_viscaDecodeMessage(uint8_t *data, int dataLength, int *message, union jr_viscaMessageParameters *messageParameters, uint8_t *sender, uint8_t *receiver);

/**
 * Encodes `message` and `messageParameters` and write it to `data`.
 * 
 * Returns the byte count of the encoded message, or -1 if the given `data` buffer is too short to hold the full message.
 */
int jr_viscaEncodeMessage(uint8_t *data, int dataLength, int message, union jr_viscaMessageParameters messageParameters, uint8_t sender, uint8_t receiver);

#endif
