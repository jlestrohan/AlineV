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
#include "timeofflight_service.h"
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

#define SERVICE_LOGGER_COMPLETE			(1 << 0)
#define SERVICE_LCD_COMPLETE			(1 << 1)
#define SERVICE_BUTTON_COMPLETE			(1 << 2)
#define SERVICE_HR04_COMPLETE			(1 << 3)
#define SERVICE_V53L0X_COMPLETE			(1 << 4)
#define SERVICE_MPU6050_COMPLETE		(1 << 5)
#define SERVICE_SDCARD_COMPLETE			(1 << 6)
#define SERVICE_SPEED_COMPLETE			(1 << 7)
#define SERVICE_HCM5883_COMPLETE		(1 << 8)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

uint16_t ServicesSuccessFlags = 0; /* holds the flags of succesfully running services */
osStatus_t val;

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .priority = (osPriority_t) osPriorityLow,
  .stack_size = 256 * 4
};

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
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
	DWT_Init();

	char *msg = "\n\r-------------------------- Starting program... Initializing services...\n\n\r";
	val = osSemaphoreAcquire(sem_UART1, osWaitForever);
	switch (val) {
		case osOK:
			HAL_UART_Transmit(&hlpuart1, (uint8_t*) msg, strlen(msg), HAL_MAX_DELAY);
			osSemaphoreRelease(sem_UART1);
		break;
		default: break;
	}

	if (log_initialize(&hlpuart1) == EXIT_FAILURE) {
		char *msg2 = "Failed Initializing Logger Service.. cannot continue sorry...\n\r";
		HAL_UART_Transmit(&hlpuart1, (uint8_t*) msg2, strlen(msg2), 0xFFFF);
		Error_Handler();
	} else { ServicesSuccessFlags |= SERVICE_LOGGER_COMPLETE; }

	if (lcdService_initialize(&hi2c1) == EXIT_FAILURE) {
		loggerE("Error Initializing LCD Service");
		Error_Handler();
	} else { ServicesSuccessFlags |= SERVICE_LCD_COMPLETE; }

	if (buttonService_initialize() == EXIT_FAILURE) {
		loggerE("Error Initializing Button Service");
		Error_Handler();
	} else { ServicesSuccessFlags |= SERVICE_BUTTON_COMPLETE; }

	if (sensor_HR04_initialize() == EXIT_FAILURE) {
		loggerE("Error Initializing HR-SC04 Distance Sensors Service");
	} else { ServicesSuccessFlags |= SERVICE_HR04_COMPLETE; }

	if (QMC5883l_Initialize(&hi2c4) == EXIT_FAILURE) {
			loggerE("Error Initializing HCM5883 Magnetometer Service");
			Error_Handler();
		} else { ServicesSuccessFlags |= SERVICE_HCM5883_COMPLETE; }

	/*if (timeofflight_initialize(&hi2c3) == EXIT_FAILURE) {
		loggerE("Error Initializing Time of Flight Service");
		Error_Handler();
	} else { ServicesSuccessFlags |= SERVICE_V53L0X_COMPLETE; }

	if (sensor_speed_initialize() == EXIT_FAILURE) {
		loggerE("Error Initializing Speed Sensors Service");
		Error_Handler();
	} else { ServicesSuccessFlags |= SERVICE_SPEED_COMPLETE; }

	if (MPU6050_Service_Initialize(&hi2c2) == EXIT_FAILURE) {
		loggerE("Error Initializing MPU6050 Sensor Service");
		Error_Handler();
	} else { ServicesSuccessFlags |= SERVICE_MPU6050_COMPLETE; }*/

	/*buzzerService_initialize(); */

	/*if (sdcardService_initialize() == EXIT_FAILURE) {
		loggerE("Error Initializing SD Card Service");
		Error_Handler();
	} else { ServicesSuccessFlags |= SERVICE_SDCARD_COMPLETE; }*/
	osSemaphoreAcquire(sem_lcdService, osWaitForever);
	lcd_send_string("Init Complete");
	osSemaphoreRelease(sem_lcdService);

	loggerI("Init sequence complete....");
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
__weak void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */

	/* very low priority task, checks every second or so if all services are up and running */
	uint16_t servicesCheck = 0;

	/* Infinite loop */
	for (;;) {

		/*if (!(servicesCheck 	& (SERVICE_LOGGER_COMPLETE
							| SERVICE_LCD_COMPLETE
							| SERVICE_BUTTON_COMPLETE
							| SERVICE_HR04_COMPLETE
							| SERVICE_V53L0X_COMPLETE
							| SERVICE_MPU6050_COMPLETE
							| SERVICE_SDCARD_COMPLETE
							| SERVICE_HCM5883_COMPLETE
							| SERVICE_SPEED_COMPLETE) > 0)) {*/

			//char *msg = "\n\r---------------\n\rOne or more services are not running correctly\n\r";

			//HAL_UART_Transmit(&hlpuart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
		//}

		loggerI("ping");
		osDelay(1000);
	}
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
