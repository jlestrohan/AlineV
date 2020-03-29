/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"
#include "stm32g4xx_ll_pwr.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC
#define B1_EXTI_IRQn EXTI15_10_IRQn
#define BUZ_OUT_Pin GPIO_PIN_0
#define BUZ_OUT_GPIO_Port GPIOA
#define SDCS_Pin GPIO_PIN_4
#define SDCS_GPIO_Port GPIOA
#define LD2_Pin GPIO_PIN_5
#define LD2_GPIO_Port GPIOA
#define mpu6050_SCL_Pin GPIO_PIN_4
#define mpu6050_SCL_GPIO_Port GPIOC
#define HR04_1_TRIG_Pin GPIO_PIN_5
#define HR04_1_TRIG_GPIO_Port GPIOC
#define DS1302RTC_CLK_Pin GPIO_PIN_10
#define DS1302RTC_CLK_GPIO_Port GPIOB
#define SPI2_SD_SCK_Pin GPIO_PIN_13
#define SPI2_SD_SCK_GPIO_Port GPIOB
#define SPI2_SD_MISO_Pin GPIO_PIN_14
#define SPI2_SD_MISO_GPIO_Port GPIOB
#define SPI2_SD_MOSI_Pin GPIO_PIN_15
#define SPI2_SD_MOSI_GPIO_Port GPIOB
#define HR04_1_ECHO_Pin GPIO_PIN_6
#define HR04_1_ECHO_GPIO_Port GPIOC
#define SPDSens1_Pin GPIO_PIN_9
#define SPDSens1_GPIO_Port GPIOC
#define SPDSens1_EXTI_IRQn EXTI9_5_IRQn
#define mpu6050_SDA_Pin GPIO_PIN_8
#define mpu6050_SDA_GPIO_Port GPIOA
#define T_SWDIO_Pin GPIO_PIN_13
#define T_SWDIO_GPIO_Port GPIOA
#define T_SWCLK_Pin GPIO_PIN_14
#define T_SWCLK_GPIO_Port GPIOA
#define SPDSens2_Pin GPIO_PIN_10
#define SPDSens2_GPIO_Port GPIOC
#define SPDSens2_EXTI_IRQn EXTI15_10_IRQn
#define SPDSens3_Pin GPIO_PIN_11
#define SPDSens3_GPIO_Port GPIOC
#define SPDSens3_EXTI_IRQn EXTI15_10_IRQn
#define SPDSens4_Pin GPIO_PIN_12
#define SPDSens4_GPIO_Port GPIOC
#define SPDSens4_EXTI_IRQn EXTI15_10_IRQn
#define T_SWO_Pin GPIO_PIN_3
#define T_SWO_GPIO_Port GPIOB
#define DS1302RTC_SDA_Pin GPIO_PIN_4
#define DS1302RTC_SDA_GPIO_Port GPIOB
#define DS1302RTC_RST_Pin GPIO_PIN_5
#define DS1302RTC_RST_GPIO_Port GPIOB
#define LCD_I2C1_SDA_Pin GPIO_PIN_7
#define LCD_I2C1_SDA_GPIO_Port GPIOB
#define LCD_I2C1_SCL_Pin GPIO_PIN_8
#define LCD_I2C1_SCL_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
