/**
 ******************************************************************************
 * @file    sdcard_service.c
 * @author  Jack Lestrohan
 * @brief   SD Card initial handler service file
 ******************************************************************************
 * @attention
 *
 *
 ******************************************************************************
 */

#include "sdcard_service.h"
#include "freertos_logger_service.h"
#include "fatfs_sd.h"
#include "string.h"
#include "ff.h"
#include <stdint.h>

#define SD_BUFFER_SIZE	1024

FATFS fs; /* file system */
FIL fil; /* file */
FRESULT fresult; /* to store the result */
char buffer[SD_BUFFER_SIZE]; /* to store data */

UINT br, bw; /* file read/write count */

/* capacity related variables */
FATFS *pfs;
DWORD free_clust;
uint32_t total, free_space;

/**
 * Calculates the data size in the buffer
 * @param buf
 * @return
 */
uint8_t bufsize(char *buf)
{
	int i = 0;
	while (*buf++ != '\0')
		i++;
	return i;
}

/**
 * clears the buffer
 */
void bufclear(void)
{
	/*for (uint8_t i=0; i<SD_BUFFER_SIZE; i++)
	 {
	 buffer[i] = '\0';
	 }*/
	memset(buffer, '\0', SD_BUFFER_SIZE);
}

/**
 * Main SDCARD Service ionitialization function
 */
void sdcardService_initialize()
{
	fresult = f_mount(&fs, "", 0);
	if (fresult != FR_OK)
		loggerE("error in mounting SD CARD\n");
	else
		loggerI("SD CARD mounted succesfully...\n");
}
