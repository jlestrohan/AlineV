/*
 * lcd_service.h
 *
 *  Created on: Mar 25, 2020
 *      Author: Jack Lestrohan
 *
 *      PINOUT => 	SCL to PB8
 *      			SDA TO PB9
 *      			GND + VCC5 at your discretion
 */

#ifndef INC_LCD_SERVICE_H_
#define INC_LCD_SERVICE_H_

#include "cmsis_os2.h"
#include "i2c.h"
#include <stdbool.h>

/**
 * Initialize lcd
 * @return
 */
uint8_t lcd_service_init (I2C_HandleTypeDef *hi2c);

/**
 * Send command to the LCD
 * @param cmd
 */
void lcd_send_cmd (char cmd);

/**
 * Send data to the LCD
 * @param data
 */
void lcd_send_data (char data);

/**
 * Send string to the LCD
 * @param str
 */
void lcd_send_string (char *str);

/**
 * Put cursor at the entered position row (0 or 1), col (0-15)
 * @param row
 * @param col
 */
void lcd_put_cur(int row, int col);

/**
 * Clear the LCD
 */
void lcd_clear (void);

#endif /* INC_LCD_SERVICE_H_ */
