
#ifndef INC_BUTTON_HANDLER_CONFIG_H_
#define INC_BUTTON_HANDLER_CONFIG_H_
/*********************************************************************************************
 * Button handler
 *
 * Author: Jack Lestrohan (jlestrohan@gmail.com)
 *
 * FreeRTOS Utility to handle a button on the nucleo cards.
 * This configuration module works with the button_handler.h & button_handler.c additional files
 *
 * It is exclusively written for Nucleo series of cards under FreeRTOS
 * Once the blue button is pressed on the card it triggers a 'evt_usrbtn_id' event that you can
 * check and use anywhere else in your program.
 *
 * Includes a full debouncing routine
 *
 * osEventFlagsId_t evt_usrbtn_id;
 *
 * *******************************************************************************************
 */


/**
 * USER CONFIG - Include your nucleo's model hal file here
 * ie: #include "stm32g0xx_hal.h"
 */
#include "stm32g4xx_hal.h"

/**
 * USER CONFIG - Button debouncing time
 * Delay within which a button press is considered as a "bounce" click
 */
#define BTN_DEBOUNCE_MS			40 // ms










#endif /* INC_BUTTON_HANDLER_CONFIG_H_ */
