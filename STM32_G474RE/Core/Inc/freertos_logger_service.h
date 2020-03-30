/**
 ******************************************************************************
 * @file    logger_service.h
 * @author  Jack Lestrohan
 * @brief   Logger service file header
 ******************************************************************************
 */
#ifndef INC_FREERTOS_LOGGER_SERVICE_H
#define INC_FREERTOS_LOGGER_SERVICE_H

#include "cmsis_os2.h"
#include "usart.h"

/**
 * Macros defines
 */
#define loggerV(fmt, ...)		log_service(fmt, ##__VA_ARGS__, LOG_VERBOSE)
#define loggerI(fmt, ...) 	 	log_service(fmt, ##__VA_ARGS__, LOG_INFO)
#define loggerW(fmt, ...) 		log_service(fmt, ##__VA_ARGS__, LOG_WARNING)
#define loggerE(fmt, ...)		log_service(fmt, ##__VA_ARGS__, LOG_ERROR)
#define loggerA(fmt, ...)  		log_service(fmt, ##__VA_ARGS__, LOG_ALERT)

typedef enum
{
	LOG_VERBOSE, //!< LOG_VERBOSE
	LOG_INFO,   //!< LOG_INFO
	LOG_WARNING,   //!< LOG_WARNING
	LOG_ERROR,  //!< LOG_ERROR
	LOG_ALERT   //!< LOG_ALERT
} LogPriority;

osMutexId_t mutex_loggerService_Hnd;

/**
 * raw logger function, must be called from macros
 * @param log_msg
 * @param priority
 */
void log_service(char *log_msg, LogPriority priority);

/**
 * Returns a human readable log priority message according to the input LogPriority type provided
 * @param priority
 * @return
 */
char* decodeLogPriority(LogPriority priority);

/**
 * processes the dequeuing and sends to the UART
 */
void log_processUart_task();

/**
 * Initializes the logger service - must be called prior to anything!
 *
 */
//todo: pass in a struct with as members at least one handler to a chosen log output peripheral (SD, Uart etc...)
uint8_t log_initialize(UART_HandleTypeDef *huart);

#endif /** end INC_FREERTOS_LOGGER_SERVICE_H */
