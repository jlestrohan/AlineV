/**
 ******************************************************************************
 * @file    lcd_service.h
 * @author  Jack Lestrohan
 * @brief   LCD module service header file
 ******************************************************************************
 * @attention
 *				PINOUT => 	SCL to PB8
 *							SDA TO PB7
 *							GND + VCC5 at your discretion
 *
 ******************************************************************************
 */

#ifndef INC_LCD_SERVICE_H_
#define INC_LCD_SERVICE_H_

#include "cmsis_os2.h"
#include <stdbool.h>
#include "i2c.h"

/**
 * Initialize lcd
 * @return
 */
uint8_t lcdService_initialize(I2C_HandleTypeDef *hi2cx);


void lcd_send_string(char *str);

/**
 * Put cursor at the entered position row (0 or 1), col (0-15)
 * @param row
 * @param col
 */
void lcd_put_cur(int row, int col);

/**
 * Clear the LCD
 */
void lcd_clear(void);

#endif /* INC_LCD_SERVICE_H_ */
