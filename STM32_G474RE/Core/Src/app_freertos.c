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
#include <command_service.h>
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */     

#include <stdlib.h>
#include "configuration.h"
#include "string.h"
#include "printf.h"
#include "sensor_speed_service.h"
#include "i2c.h"
#include "Button_service.h"
#include "sdcard_service.h"
#include "IRQ_Handler.h"
#include "lcdMenu_service.h"
#include "QMC5883_service.h"
#include "HCSR04_service.h"
#include "MotorsControl_service.h"
#include "MG90S_service.h"
#include "MPU6050_service.h"
#include "navControl_service.h"
#include "esp32serial_service.h"
#include "SystemInfos.h"
#include "uvLed_service.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define SERVICE_LOGGER_COMPLETE			(1 << 0)
#define SERVICE_LCD_COMPLETE			(1 << 1)
#define SERVICE_BUTTON_COMPLETE			(1 << 2)
#define SERVICE_HR04_COMPLETE			(1 << 3)
#define SERVICE_V53L0X_COMPLETE			(1 << 4)
#define SERVICE_MPU6050_COMPLETE		(1 << 5)
#define SERVICE_SDCARD_COMPLETE			(1 << 6)
#define SERVICE_SPEED_COMPLETE			(1 << 7)
#define SERVICE_HCM5883_COMPLETE		(1 << 8)
#define SERVICE_NAVCONTROL_COMPLETE		(1 << 9)
#define SERVICE_MOTORSCONTROL_COMPLETE	(1 << 10)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
UART_HandleTypeDef hlpuart1;
uint16_t ServicesSuccessFlags = 0; /* holds the flags of succesfully running services */

/* USER CODE END Variables */
/* Definitions for supervisorTask */
osThreadId_t supervisorTaskHandle;
const osThreadAttr_t supervisorTask_attributes = {
		.name = "supervisorTask",
		.priority = (osPriority_t) osPriorityLow,
		.stack_size = 256 * 4
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartSupervisorTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void) {
	/* USER CODE BEGIN Init */

	char *msg = "\n\r-------------------------- Starting program... Initializing services...\n\n\r";
	HAL_UART_Transmit(&hlpuart1, (uint8_t*) msg, strlen(msg), HAL_MAX_DELAY);

#ifdef DEBUG_SERVICE_LCD_MENU
	if (uLcdMenuServiceInit() == EXIT_FAILURE) {
		printf("Error Initializing LCD Menu Service\n\r");
		Error_Handler();
	}
#endif

#ifdef DEBUG_SERVICE_BUTTON
	if (uButtonServiceInit() == EXIT_FAILURE) {
		printf("Error Initializing Button Service\n\r");
		Error_Handler();
	} else {
		ServicesSuccessFlags |= SERVICE_BUTTON_COMPLETE;
	}
#endif

#ifdef DEBUG_SERVICE_HCSR04
	if (uHcsr04ServiceInit() == EXIT_FAILURE) {
		printf("Error Initializing HR-SC04 Distance Sensors Service\n\r");
		Error_Handler();
	} else {
		ServicesSuccessFlags |= SERVICE_HR04_COMPLETE;
	}
#endif

#ifdef DEBUG_SERVICE_MG90S
	if (uMg90sServiceInit() == EXIT_FAILURE) {
		printf("Error Initializing Front Servo Service\n\r");
		Error_Handler();
	}
#endif

#ifdef DEBUG_SERVICE_QMC5883
	if (uQmc5883lServiceInit() == EXIT_FAILURE) {
		printf("Error Initializing QCM5883 Magnetometer Service\n\r");
		Error_Handler();
	} else {
		ServicesSuccessFlags |= SERVICE_HCM5883_COMPLETE;
	}
#endif

#ifdef DEBUG_SERVICE_CMD_PARSER
	if (uCmdParseServiceInit() == EXIT_FAILURE) {
		printf("Error Initializing Command Parser Service\n\r");
		Error_Handler();
	}
#endif

#ifdef DEBUG_SERVICE_ESP32_SERIAL
	if (uEsp32SerialServiceInit() == EXIT_FAILURE) {
		printf("Error Initializing ESP32 Serial TX/RX Service\n\r");
		Error_Handler();
	}
#endif


	/*if (sensor_speed_initialize() == EXIT_FAILURE) {
		loggerE("Error Initializing Speed Sensors Service");
		Error_Handler();
	} else {
		ServicesSuccessFlags |= SERVICE_SPEED_COMPLETE;
	}*/

	if (uMpu6050ServiceInit() == EXIT_FAILURE) {
		printf("Error Initializing MPU6050 Sensor Service\n\r");
	} else {
		ServicesSuccessFlags |= SERVICE_MPU6050_COMPLETE;
	}

	/*if (uSdCardServiceInit() == EXIT_FAILURE) {
		loggerE("Error Initializing SD Card Service");
		Error_Handler();
	} else {
		ServicesSuccessFlags |= SERVICE_SDCARD_COMPLETE;
	}*/

#ifdef DEBUG_SERVICE_MOTORS
	if (uMotorsControlServiceInit() == EXIT_FAILURE) {
		printf("Error Initializing Motors Control Service");
		Error_Handler();
	} else {
		ServicesSuccessFlags |= SERVICE_MOTORSCONTROL_COMPLETE;
	}
#endif

#ifdef DEBUG_SERVICE_NAVCONTROL
	if (uNavControlServiceInit() == EXIT_FAILURE) {
		printf("Error Initializing Nav Control Service");
		Error_Handler();
	} else {
		ServicesSuccessFlags |= SERVICE_NAVCONTROL_COMPLETE;
	}
#endif

	//FIXME: Crash here!
	/*if (uSystemInfoServiceInit()== EXIT_FAILURE) {
		dbg_printf("Error Initializing System Info Service");
		Error_Handler();
	}*/

#ifdef DEBUG_SERVICE_UVLED
	if (uUvLedServiceInit() == EXIT_FAILURE) {
		printf("Error Initializing UV Led Service");
		Error_Handler();
	}
#endif

	//dbg_printf("Init sequence complete....");
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
	/* creation of supervisorTask */
	supervisorTaskHandle = osThreadNew(StartSupervisorTask, NULL, &supervisorTask_attributes);

	/* USER CODE BEGIN RTOS_THREADS */
	/* add threads, ... */
	/* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartSupervisorTask */
/**
 * @brief  Function implementing the supervisorTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartSupervisorTask */
void StartSupervisorTask(void *argument)
{
	/* USER CODE BEGIN StartSupervisorTask */
	/* USER CODE BEGIN StartDefaultTask */
	osDelay(5000);

	/* Infinite loop */
	for(;;)
	{

		osDelay(100);
	}
	/* USER CODE END StartSupervisorTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
