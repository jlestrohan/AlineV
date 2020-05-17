/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-22 17:45:37
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-05 23:48:53
 * @ Description:	Ring Buffer implementation for easier UART TX/RX between MPU handling
 *
 *******************************************************************************************/

#ifndef UARTRINGBUFFER_H_
#define UARTRINGBUFFER_H_

#include "stm32g4xx_hal.h"
#include "cmsis_os2.h"

/**
 * change the size of the buffer
 */
#define UART_BUFFER_SIZE 1024

typedef struct
{
  unsigned char buffer[UART_BUFFER_SIZE];
  volatile unsigned int head;
  volatile unsigned int tail;
} ring_buffer;

/**
 * Initialize the ring buffer
 */
uint8_t Ringbuf_init(void);

/**
 * reads the data in the rx_buffer and increment the tail count in rx_buffer
 */
int Uart_read(void);

/**
 * writes the data to the tx_buffer and increment the head count in tx_buffer
 */
void Uart_write(int c);

/**
 * function to send the string to the uart
 */
void Uart_sendstring(const char *s);

/**
 * Print a number with any base
 * base can be 10, 8 etc
 */
void Uart_printbase (long n, uint8_t base);

/**
 * Checks if the data is available to read in the rx_buffer
 */
int IsDataAvailable(void);


/** Look for a particular string in the given buffer
 * @return 1, if the string is found and -1 if not found
 * @USAGE:: if (Look_for ("some string", buffer)) do something
 */
int Look_for (char *str, char *buffertolookinto);


/** Peek for the data in the Rx Bffer without incrementing the tail count
* Returns the character
* USAGE: if (Uart_peek () == 'M') do something 
*/
int Uart_peek();


/** Copy the data from the Rx buffer into the bufferr, Upto and including the entered string
* This copying will take place in the blocking mode, so you won't be able to perform any other operations
* Returns 1 on success and -1 otherwise
* USAGE: while (!(Copy_Upto ("some string", buffer)));
*/
int Copy_upto (char *string, char *buffertocopyinto);


/** Copies the entered number of characters (blocking mode) from the Rx buffer into the buffer, after some particular string is detected
* Returns 1 on success and -1 otherwise
* USAGE: while (!(Get_after ("some string", 6, buffer)));
*/
int Get_after (char *string, uint8_t numberofchars, char *buffertosave);

/** Wait until a paricular string is detected in the Rx Buffer
* Return 1 on success and -1 otherwise
* USAGE: while (!(Wait_for("some string")));
*/
int Wait_for (char *string);


/** the ISR for the uart. put it in the IRQ handler */
void Uart_isr (UART_HandleTypeDef *huart);

#endif /* UARTRINGBUFFER_H_ */
