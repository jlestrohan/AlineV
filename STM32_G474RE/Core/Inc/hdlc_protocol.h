/*
 * hdlc_protocol.h
 *
 *  Created on: May 6, 2020
 *      Author: Jack Lestrohan
 */

#ifndef INC_HDLC_PROTOCOL_H_
#define INC_HDLC_PROTOCOL_H_

#include <stdint.h>
#include <stdbool.h>
#include "cmsis_os2.h"

extern osMutexId_t mHdlcProtocolMutex;

typedef void (*sendchar_type)(uint8_t);
typedef void (*frame_handler_type)(uint8_t *framebuffer, uint8_t framelength);

/**
 * @Brief Main routine Initialization
 * @param
 * @param
 * @param max_frame_length
 */
void uHdlcProtInit(sendchar_type, frame_handler_type, uint16_t max_frame_length);
void vCharReceiver(uint8_t data);
void vSendFrame(const uint8_t *framebuffer, uint16_t frame_length);



#endif /* INC_HDLC_PROTOCOL_H_ */
