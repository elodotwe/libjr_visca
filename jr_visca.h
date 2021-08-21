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

#define JR_VISCA_MAX_FRAME_DATA_LENGTH 14

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

#define JR_VISCA_MESSAGE_PAN_TILT_POSITION_INQ 1
#define JR_VISCA_MESSAGE_ZOOM_POSITION_INQ 2

union jr_viscaMessageParameters
{
    
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

#endif