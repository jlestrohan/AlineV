/**
 ******************************************************************************
 * @file    lcd_service.c
 * @author  Jack Lestrohan
 * @brief   LCD module service file
 ******************************************************************************
 * @attention
 *				PINOUT => 	SCL to PB8
 *							SDA TO PB7
 *							GND + VCC5 at your discretion
 *
 ******************************************************************************
 */
#include <stdlib.h>
#include <FreeRTOS.h>
#include "lcd_service.h"
#include "freertos_logger_service.h"
#include <string.h>
#include <stdio.h>

#define SLAVE_ADDRESS_LCD 	0x27 << 1 	/* have to shift 7bits arduino address to the left for 8 bits compat */
#define MAX_LINE_CHAR		0x0A		/* max chars per line */

static I2C_HandleTypeDef *_hi2cxHandler;
typedef StaticQueue_t osStaticMessageQDef_t;
static osStatus_t osStatus;

void lcd_send_data(char data);
static char *msgchar;

static osMessageQueueId_t queue_lcdHandle;

typedef enum
{
	lcdServiceNotInit, lcdServiceInitOK, lcdServiceInitError
} lcdServiceStatus_t;
static lcdServiceStatus_t lcdServiceStatus = lcdServiceInitError;

/**
 * Definitions for lcdServiceTask
 */
typedef StaticTask_t osStaticThreadDef_t;
static osThreadId_t lcdServiceTaHandle;
static uint32_t lcdServiceTaBuffer[256];
static osStaticThreadDef_t lcdServiceTaControlBlock;
static const osThreadAttr_t lcdServiceTa_attributes = {
		.name = "lcdServiceTask",
		.stack_mem = &lcdServiceTaBuffer[0],
		.stack_size = sizeof(lcdServiceTaBuffer),
		.cb_mem = &lcdServiceTaControlBlock,
		.cb_size = sizeof(lcdServiceTaControlBlock),
		.priority = (osPriority_t) osPriorityLow, };

/**
 * Pre LCD initialization
 */
static void lcd_prepare()
{
	/* LCD INITIALIZATION */
	/* 4 bit initialisation */
	HAL_Delay(50);
	; /* wait for >40ms */
	lcd_send_cmd(0x30);
	HAL_Delay(5);
	; /* wait for >4.1ms */
	lcd_send_cmd(0x30);
	HAL_Delay(1); /* wait for >100us */
	lcd_send_cmd(0x30);
	HAL_Delay(10);
	lcd_send_cmd(0x20); /* 4bit mode */
	HAL_Delay(10);

	/* dislay initialization */
	lcd_send_cmd(0x28); /* Function set --> DL=0 (4 bit mode), N = 1 (2 line display) F = 0 (5x8 characters) */
	HAL_Delay(1);
	lcd_send_cmd(0x08); /* Display on/off control --> D=0,C=0, B=0  ---> display off */
	HAL_Delay(1);
	lcd_send_cmd(0x01); /* clear display */
	HAL_Delay(2);
	lcd_send_cmd(0x06); /* Entry mode set --> I/D = 1 (increment cursor) & S = 0 (no shift) */
	HAL_Delay(1);
	lcd_send_cmd(0x0C); /* Display on/off control --> D = 1, C and B = 0. (Cursor and blink, last two bits) */
	HAL_Delay(5);
	lcd_send_cmd(0x80);
}

/**
 * LCD main task
 * @param argument
 */
void lcdService_task(void *argument)
{
	loggerI("Starting lcd service task...");
	for (;;) {

		/* receives a full string */
		osStatus = osMessageQueueGet(queue_lcdHandle, &msgchar, NULL, osWaitForever);
		if (osStatus == osOK) {
			while (*msgchar) {
				lcd_send_data(*msgchar++);
			}
		}
		osDelay(10);
	}
}

/**
 * Initialize lcd
 * @param hi2cx
 * @return
 */
uint8_t lcdService_initialize(I2C_HandleTypeDef *hi2cx)
{
	_hi2cxHandler = hi2cx;

	/** is device ready and responding ? */
	if (HAL_I2C_IsDeviceReady(_hi2cxHandler, SLAVE_ADDRESS_LCD, 2, 5) != HAL_OK) {
		loggerE("LCD Device not ready");
		return (EXIT_FAILURE);
	}

	queue_lcdHandle = osMessageQueueNew(MAX_LINE_CHAR, sizeof(char), NULL);
	if (!queue_lcdHandle) {
		return (EXIT_FAILURE);
	}

	lcd_prepare();

	osSemaphoreId_t sem_lcdService = osSemaphoreNew(01U, 01U, NULL);
	 if (sem_lcdService == NULL) {
		 loggerE("Initializing LCD Service - Failed (semaphore could not be created)");
	  }

	/* creation of LoggerServiceTask */
	lcdServiceTaHandle = osThreadNew(lcdService_task, NULL, &lcdServiceTa_attributes);
	if (!lcdServiceTaHandle) {
		lcdServiceStatus = lcdServiceInitError;
		loggerE("Initializing LCD Service - Failed");
		return (EXIT_FAILURE);
	}

	loggerI("Initializing LCD Service - Success!");
	return (EXIT_SUCCESS);
}

/**
 * Send command to the LCD
 * @param cmd
 */
void lcd_send_cmd(char cmd)
{
	osStatus_t val;
	val = osSemaphoreAcquire(sem_I2C1, osWaitForever);
	if (val == osOK) {
		char data_u, data_l;
		uint8_t data_t[4] = "";
		data_u = (cmd & 0xf0);
		data_l = ((cmd << 4) & 0xf0);
		*data_t = data_u | 0x0C; /* en=1, rs=0 */
		*(data_t + 1) = data_u | 0x08; /* en=0, rs=0 */
		*(data_t + 2) = data_l | 0x0C; /* en=1, rs=0 */
		*(data_t + 3) = data_l | 0x08; /* en=0, rs=0 */
		HAL_I2C_Master_Transmit(_hi2cxHandler, SLAVE_ADDRESS_LCD, (uint8_t*) data_t, 4, 100);
		osSemaphoreRelease(sem_I2C1);
	}
}

/**
 * Send data to the LCD
 * @param data
 */
void lcd_send_data(char data)
{
	osStatus_t val;
	val = osSemaphoreAcquire(sem_I2C1, osWaitForever);
	if (val == osOK) {
		char data_u, data_l;
		uint8_t data_t[4] = "";
		data_u = (data & 0xf0);
		data_l = ((data << 4) & 0xf0);
		*data_t = data_u | 0x0D; /* en=1, rs=0 */
		*(data_t + 1) = data_u | 0x09; /* en=0, rs=0 */
		*(data_t + 2) = data_l | 0x0D; /* en=1, rs=0 */
		*(data_t + 3) = data_l | 0x09; /* en=0, rs=0 */
		HAL_I2C_Master_Transmit(_hi2cxHandler, SLAVE_ADDRESS_LCD, (uint8_t*) data_t, 4, 100);
		osSemaphoreRelease(sem_I2C1);
	}
}

/**
 * Send string to the LCD
 * @param str
 */
void lcd_send_string(char *str)
{
	lcd_send_cmd(0x80); /* clear display */
	/* sends the whole string to the queue */
	msgchar = str;
	osMessageQueuePut(queue_lcdHandle, &msgchar, 0U, osWaitForever);
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

	lcd_send_cmd(col);
}

/**
 * Clear the LCD
 */
void lcd_clear(void)
{
	lcd_send_cmd(0x80);
	for (int i = 0; i < 70; i++) {
		lcd_send_data(' ');
	}
}

