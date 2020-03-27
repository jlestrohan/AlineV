/*
 * lcd_service.c
 *
 *  Created on: Mar 25, 2020
 *      Author: Jack Lestrohan
 */

#include <stdlib.h>
#include <FreeRTOS.h>
#include "lcd_service.h"
#include "i2c.h"
#include "freertos_logger_service.h"
#include <string.h>
#include <stdio.h>

#define SLAVE_ADDRESS_LCD 0x27 << 1 /* have to shift 7bits arduino address to the left for 8 bits compat */

extern I2C_HandleTypeDef hi2c1;  /** change your handler here accordingly */
typedef StaticTask_t osStaticThreadDef_t;

typedef enum {
	lcdServiceNotInit,
	lcdServiceInitOK,
	lcdServiceInitError
} lcdServiceStatus_t;
lcdServiceStatus_t lcdServiceStatus = lcdServiceInitError;

/**
 * Definitions for lcdServiceTask
 */
osThreadId_t lcdServiceTaHandle;
uint32_t lcdServiceTaBuffer[ 256 ];
osStaticThreadDef_t lcdServiceTaControlBlock;
const osThreadAttr_t lcdServiceTa_attributes = {
		.name = "lcdServiceTask",
		.stack_mem = &lcdServiceTaBuffer[0],
		.stack_size = sizeof(lcdServiceTaBuffer),
		.cb_mem = &lcdServiceTaControlBlock,
		.cb_size = sizeof(lcdServiceTaControlBlock),
		.priority = (osPriority_t) osPriorityLow,
};

void lcdService_task(void *argument);

void lcd_prepare()
{
	/* LCD INITIALIZATION */
	/* 4 bit initialisation */
	HAL_Delay(50);;  /* wait for >40ms */
	lcd_send_cmd (0x30);
	HAL_Delay(5);;  /* wait for >4.1ms */
	lcd_send_cmd (0x30);
	HAL_Delay(1);  /* wait for >100us */
	lcd_send_cmd (0x30);
	HAL_Delay(10);
	lcd_send_cmd (0x20);  /* 4bit mode */
	HAL_Delay(10);

	/* dislay initialization */
	lcd_send_cmd (0x28); /* Function set --> DL=0 (4 bit mode), N = 1 (2 line display) F = 0 (5x8 characters) */
	HAL_Delay(1);
	lcd_send_cmd (0x08); /* Display on/off control --> D=0,C=0, B=0  ---> display off */
	HAL_Delay(1);
	lcd_send_cmd (0x01);  /* clear display */
	HAL_Delay(2);
	lcd_send_cmd (0x06); /* Entry mode set --> I/D = 1 (increment cursor) & S = 0 (no shift) */
	HAL_Delay(1);
	lcd_send_cmd (0x0C); /* Display on/off control --> D = 1, C and B = 0. (Cursor and blink, last two bits) */
	HAL_Delay(5);
	lcd_send_cmd(0x80);
}

/**
 * Initialize lcd
 */
uint8_t lcd_service_init (void)
{
	/* creation of LoggerServiceTask */
	//lcdServiceTaHandle = osThreadNew(lcdService_task, NULL, &lcdServiceTa_attributes);
	//if (!lcdServiceTaHandle) {
	//	lcdServiceStatus = lcdServiceInitError;
	//	loggerE("LCD Service - Initialization Failure");
	//	return (EXIT_FAILURE);
	//}

	lcd_prepare();

	loggerI("LCD Service - Initialization complete");
	return (EXIT_SUCCESS);
}


/**
 * LCD main task
 * @param argument
 */
/*void lcdService_task(void* argument)
{

	for (;;)
	{

		osDelay(100);
	}
}*/

/**
 * Send command to the LCD
 * @param cmd
 */
void lcd_send_cmd (char cmd)
{
	char data_u, data_l;
	uint8_t data_t[4];
	data_u = (cmd&0xf0);
	data_l = ((cmd<<4)&0xf0);
	data_t[0] = data_u|0x0C;  /* en=1, rs=0 */
	data_t[1] = data_u|0x08;  /* en=0, rs=0 */
	data_t[2] = data_l|0x0C;  /* en=1, rs=0 */
	data_t[3] = data_l|0x08;  /* en=0, rs=0 */
	HAL_I2C_Master_Transmit (&hi2c1, SLAVE_ADDRESS_LCD,(uint8_t *) data_t, 4, 100);
}

/**
 * Send data to the LCD
 * @param data
 */
void lcd_send_data (char data)
{
	char data_u, data_l;
	uint8_t data_t[4];
	data_u = (data&0xf0);
	data_l = ((data<<4)&0xf0);
	data_t[0] = data_u|0x0D;  /* en=1, rs=0 */
	data_t[1] = data_u|0x09;  /* en=0, rs=0 */
	data_t[2] = data_l|0x0D;  /* en=1, rs=0 */
	data_t[3] = data_l|0x09;  /* en=0, rs=0 */
	HAL_I2C_Master_Transmit (&hi2c1, SLAVE_ADDRESS_LCD,(uint8_t *) data_t, 4, 100);
}

/**
 * Send string to the LCD
 * @param str
 */
void lcd_send_string (char *str)
{
	while (*str) lcd_send_data (*str++);
}

/**
 * Put cursor at the entered position row (0 or 1), col (0-15)
 * @param row
 * @param col
 */
void lcd_put_cur(int row, int col)
{
	switch (row)
	{
	case 0:
		col |= 0x80;
		break;
	case 1:
		col |= 0xC0;
		break;
	default:
		break;
	}

	lcd_send_cmd (col);
}


/**
 * Clear the LCD
 */
void lcd_clear (void)
{
	lcd_send_cmd (0x80);
	for (int i=0; i<70; i++)
	{
		lcd_send_data (' ');
	}
}




