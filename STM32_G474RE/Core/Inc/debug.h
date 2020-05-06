/**
  ******************************************************************************
  * @file           : debug.h
  * @brief          : Header for debug.c file.
  *                   This file contains the debug tools.
  *                   Define DEBUG to have it
  ******************************************************************************
**/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DEBUG_H
#define __DEBUG_H

/* Private includes ----------------------------------------------------------*/

// HAL
#include "stm32g4xx_hal.h"

// use common libs
#include <stdio.h>		// sprintf & printf
#include <stdbool.h>	// bool
#include <string.h>		// strlen...

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

#define DEBUG_UART		hlpuart1

#ifndef DEBUG /* to avoid warings */
#define DEBUG /*to be defined or to be in comment*/
#endif /* to avoid warings */

#define TIMEOUT_IN		10   /* 10 ms short time out for char input */
#define TIMEOUT_OUT		1000 /* 1 sec time out for text output */
#define TXT_IN_SIZE		255  /* max size buffer in */
#define TXT_TAG_SIZE	255  /* max size tag */
#define TXT_OUT_SIZE	255  /* max size buffer out */
#define INPUT_EOT		'\r' /* Carriage return */

/* Exported macro ------------------------------------------------------------*/

// debug strings
char dbg_tag[TXT_TAG_SIZE];
char dbg_msg[TXT_OUT_SIZE];

#ifdef DEBUG /* DeBUGGING MODE */

#define TRACE true

/* macro to trace debug on serial monitor  with ticks */
#define dbg_printf(...) { \
if ( sprintf(dbg_tag, "<%lu>", HAL_GetTick()) &&  /* ticks tag */ \
	sprintf(dbg_msg, __VA_ARGS__) ) /* args */ \
	printf("\r\n%s\t%s", dbg_tag, dbg_msg); /* concat */ \
}; /* end debug macro */

/* macro to trace debug on serial monitor with file & line number */
#define dbg_log(...) { \
if ( sprintf(dbg_tag, "%s[%d]", __FILE__, __LINE__) &&  /* file line tag */ \
	sprintf(dbg_msg, __VA_ARGS__) ) /* args */ \
	printf("\r\n%s\t%s", dbg_tag, dbg_msg); /* concat */ \
}; /* end debug macro */

#else /* NO DEBUG */

#define TRACE false

/* macro are NOP */
#define dbg_printf(...) {}
#define dbg_log(...) {}

#endif /* DEBUG */

/* Exported functions prototypes ---------------------------------------------*/

/* Private defines -----------------------------------------------------------*/

/* Redefines the weak functions :

#ifdef __GNUC__
int __io_putchar(int ch);
int __io_getchar();
#else
int fputc(int ch, FILE *f);
int __io_getchar();
#endif

*/

#endif /* __DEBUG_H */
