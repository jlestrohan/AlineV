/**
 ******************************************************************************
 * @file    logger_service.c
 * @author  Jack Lestrohan
 * @brief   Logger service file
 ******************************************************************************
 */
#include <freertos_logger_service.h>
#include "configuration.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "cmsis_os2.h"
#include <string.h>
#include "usart.h"
#include <FreeRTOS.h>

#define		MESSAGE_BUFFER		200

static UART_HandleTypeDef *_huart;

/**
 * message queue definition
 */
typedef StaticQueue_t osStaticMessageQDef_t;
typedef StaticTask_t osStaticThreadDef_t;

typedef struct
{
	char msgBuf[MESSAGE_BUFFER];
	uint16_t msgIncId;
	uint8_t priority;
} MSGQUEUE_OBJ_t;
static MSGQUEUE_OBJ_t msg;

static uint16_t incMsgIdCounter = 0;
static osMessageQueueId_t queue_loggerHandle;

static uint8_t status;

typedef StaticTask_t osStaticThreadDef_t;

typedef enum
{
	logServiceNotInit, logServiceinitOK,
} logServiceStatus;
static logServiceStatus logStatus = logServiceNotInit;

/**
 * Definitions for LoggerServiceTask
 */
static osThreadId_t xLoggerServiceTaHandle;
static osStaticThreadDef_t LoggerTaControlBlock;
static uint32_t LoggerTaBuffer[256];
static const osThreadAttr_t LoggerServiceTa_attributes = {
		.name = "LoggerServiceTask",
		.stack_mem = &LoggerTaBuffer[0],
		.stack_size = sizeof(LoggerTaBuffer),
		.cb_mem = &LoggerTaControlBlock,
		.cb_size = sizeof(LoggerTaControlBlock),
		.priority = (osPriority_t) OSTASK_PRIORITY_LOGGER, };

/**
 * main logger service task
 * @param argument
 */
void StartLoggerServiceTask(void *argument)
{
	osStatus_t val;
	loggerI("Starting logger service task...");
	char finalmsg[MESSAGE_BUFFER + 50];

	for (;;) {
		/* prevent compilation warning */
		UNUSED(argument);

		osMessageQueueGet(queue_loggerHandle, &msg, NULL, osWaitForever); /* wait for message */
		if (status == osOK) {
			sprintf(finalmsg, "(id) %04d | (timestamp) %08lu %s | %s\r\n", msg.msgIncId, osKernelGetTickCount(), decodeLogPriority(msg.priority), msg.msgBuf);
			val = osSemaphoreAcquire(sem_UART1, osWaitForever);
			switch (val) {
			case osOK:
				HAL_UART_Transmit(_huart, (uint8_t*) finalmsg, strlen(finalmsg), HAL_MAX_DELAY);
				osSemaphoreRelease(sem_UART1);
				break;
			default:
				break;
			}
		}

		osDelay(20);
	}


}


/**
 * Initialize log service - must be called once at the start of the program
 * @param huart
 */
uint8_t uLoggerServiceInitialize(UART_HandleTypeDef *huart)
{
	_huart = huart;

	queue_loggerHandle = osMessageQueueNew(10, sizeof(MSGQUEUE_OBJ_t), NULL);
	if (!queue_loggerHandle) {
		return (EXIT_FAILURE);
	}

	logStatus = logServiceinitOK;

	/* creation of LoggerServiceTask */
	xLoggerServiceTaHandle = osThreadNew(StartLoggerServiceTask, NULL, &LoggerServiceTa_attributes);
	if (!xLoggerServiceTaHandle) {
		return (EXIT_FAILURE);
	}

	loggerI("Initializing Logger Service... Success!");
	return (EXIT_SUCCESS);
}

/**
 * raw logger function, must be called from macros
 * @param log_msg
 * @param priority
 * @return
 */
void log_service(const char *log_msg, LogPriority priority)
{
	incMsgIdCounter++;
	/*  fill up the queue with the logger message given as argument */

	memcpy(msg.msgBuf, log_msg, MESSAGE_BUFFER);
	msg.msgIncId = incMsgIdCounter;
	msg.priority = priority;
	osMessageQueuePut(queue_loggerHandle, &msg, 0U, osWaitForever);
}

/**
 * Returns a human readable log priority message according to the input LogPriority type provided
 * @param priority
 * @return
 */
char* decodeLogPriority(LogPriority priority)
{
	switch (priority)
	{
	case LOG_VERBOSE:
		return ("[VERBOSE]");
		break;
	case LOG_INFO:
		return ("[INFO]");
		break;
	case LOG_WARNING:
		return ("[WARNING]");
		break;
	case LOG_ERROR:
		return ("[ERROR]");
		break;
	case LOG_ALERT:
		return ("[ALERT]");
		break;
	default:
		return ("");
		break;
	}
}

