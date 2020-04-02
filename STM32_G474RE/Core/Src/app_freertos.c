/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : app_freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include "string.h"
#include "freertos_logger_service.h"
#include "sensor_speed_service.h"
#include "i2c.h"
#include "dwt_delay.h"
#include "mpu6050_service.h"
#include "button_handler.h"
#include "buzzer_service.h"
#include "sdcard_service.h"
#include "sensor_hr04_service.h"
#include "IRQ_Handler.h"
#include "lcd_service.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
        .name = "defaultTask",
        .priority = (osPriority_t) osPriorityNormal,
        .stack_size = 256 * 4 };

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void)
{
	/* USER CODE BEGIN Init */
	DWT_Init();

	char *msg = "\n\r-------------------------- Starting program... Initializing services...\n\n\r";
	HAL_UART_Transmit(&hlpuart1, (uint8_t*) msg, strlen(msg), 0xFFFF);

	if (log_initialize() == EXIT_FAILURE) {
		char *msg2 = "Failed Initializing Logger Service.. cannot continue sorry...\n\r";
		HAL_UART_Transmit(&hlpuart1, (uint8_t*) msg2, strlen(msg2), 0xFFFF);
		Error_Handler();
	}

	if (lcdService_initialize(&hi2c1) == EXIT_FAILURE) {
		loggerE("Error Initializing LCD Service");
		Error_Handler();
	}

	if (buttonService_initialize() == EXIT_FAILURE) {
		loggerE("Error Initializing Button Service");
		Error_Handler();
	}

	/*if (sensor_speed_initialize() == EXIT_FAILURE) {
	 loggerE("Error Initializing Speed Sensors Service");
	 Error_Handler();
	 }*/

	if (MPU6050_Service_Initialize(&hi2c2) == EXIT_FAILURE) {
		loggerE("Error Initializing MPU6050 Sensor Service");
		Error_Handler();
	}

	//buzzerService_initialize();
	//if (sensor_HR04_initialize() == EXIT_FAILURE) {
	//	loggerE("Error Initializing HR-SC04 Distance Sensors Service");
	//}

	/*if (sdcardService_initialize() == EXIT_FAILURE) {
	 loggerE("Error Initializing SD Card Service");
	 Error_Handler();
	 }*/

	lcd_send_string("Init Complete");
	/** let's start the 1µs timer for the whole application */

	/* USER CODE END Init */

	/* USER CODE BEGIN RTOS_MUTEX */
	/* add mutexes, ... */
	/* USER CODE END RTOS_MUTEX */

	/* USER CODE BEGIN RTOS_SEMAPHORES */
	/* add semaphores, ... */
	/* USER CODE END RTOS_SEMAPHORES */

	/* USER CODE BEGIN RTOS_TIMERS */
	/* start timers, add new ones, ... */
	/* USER CODE END RTOS_TIMERS */

	/* USER CODE BEGIN RTOS_QUEUES */
	/* add queues, ... */
	/* USER CODE END RTOS_QUEUES */

	/* Create the thread(s) */
	/* creation of defaultTask */
	defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

	/* USER CODE BEGIN RTOS_THREADS */
	/* add threads, ... */
	/* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
	/* USER CODE BEGIN StartDefaultTask */
	loggerI("Initializing default task...");
	/* Infinite loop */
	for (;;) {

		osDelay(100);
	}
	/* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
