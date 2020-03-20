/*
 * button_handler.c
 *
 *  Created on: 26 févr. 2020
 *      Author: jack
 */

#include "button_handler.h"
#include "button_handler_config.h"
#include "stdint.h"
#include <stdbool.h>
#include "gpio.h"


uint32_t 	lastPressedTick = 0;
/**
 * returns true if not a bounce while releasing
 */
uint8_t buttonDebounce (uint32_t tick)
{
	return (HAL_GetTick() - tick > BTN_DEBOUNCE_MS  ? true : false);
}


/**
 * handles buttonIRQ events
 */
void buttonIRQ_cb() {

	if (buttonDebounce(lastPressedTick) || lastPressedTick == 0) {
		lastPressedTick = HAL_GetTick();
		HAL_GPIO_TogglePin(GPIOA, LD2_Pin);
		osEventFlagsSet( evt_usrbtn_id, BTN_PRESSED_FLAG);
	}
}


