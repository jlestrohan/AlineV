/**
  ******************************************************************************
  * @file           : debug.c
  * @brief          : debug tools
  ******************************************************************************
**/

/* Includes ------------------------------------------------------------------*/
#include "debug.h"

/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

extern UART_HandleTypeDef DEBUG_UART; // serial monitor

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

// redefines output input standard io

// extern HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout);
// extern HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout);

#ifdef __GNUC__
int __io_putchar(int ch)
{
   uint8_t ch8=ch;
   HAL_UART_Transmit(&DEBUG_UART,(uint8_t *)&ch8, 1, TIMEOUT_OUT);
   return ch;
}
int __io_getchar()
{
   uint8_t ch8;
   HAL_UART_Receive(&DEBUG_UART,&ch8,1,TIMEOUT_IN);
   return (int)ch8;
}
#else
int fputc(int ch, FILE *f)
{
  HAL_UART_Transmit(&DEBUG_UART, (uint8_t *)&ch, 1, TIMEOUT_OUT);
  return ch;
}
int __io_getchar()
{
   uint8_t ch8;
   HAL_UART_Receive(&DEBUG_UART,&ch8, 1, TIMEOUT_IN);
   return (int)ch8;
}
#endif /* __GNUC__ */

