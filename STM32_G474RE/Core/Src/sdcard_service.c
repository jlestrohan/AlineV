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
#include "debug.h"
#include "fatfs_sd.h"
#include "string.h"
#include <FreeRTOS.h>
#include "cmsis_os2.h"
#include <stdio.h>
#include "ff.h"
#include <stdint.h>
#include <stdlib.h>

#define SD_BUFFER_SIZE	1024

FATFS fs; /* file system */
FIL fil; /* file */
FRESULT fresult; /* to store the result */
char buffer[SD_BUFFER_SIZE]; /* to store data */

osTimerId_t osSDTimer1;
volatile uint8_t FatFsCnt = 0;
volatile uint8_t Timer1, Timer2;

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
	while (*buf++ != '\0') {
		i++;
	}
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

void sdcard_write_test()
{
	/* open RW file if it doesn't already exists */
	fresult = f_open(&fil, "file1.txt", FA_OPEN_ALWAYS | FA_READ | FA_WRITE);

	/* writes text */
	fresult = f_puts("This data is from the First FILE\n\n", &fil);

	/* closes file */
	fresult = f_close(&fil);
	dbg_printf("file1.txt created and the data is written");

	/* opens a RO file */
	fresult = f_open(&fil, "file1.txt", FA_READ);

	/* reads string from file */
	f_gets(buffer, sizeof(buffer), &fil);

	dbg_printf(buffer);

	/* closes file */
	f_close(&fil);

	bufclear();
}

/**
 * Check SDCARD Free Space
 */
void sdcard_check_free_space()
{
	f_getfree(NULL, &free_clust, &pfs);
	total = (uint32_t) ((pfs->n_fatent - 2) * pfs->csize * 0.5);
	/* sprintf(buffer, "SD CARD Total Size: \t%lu", total);*/
	dbg_printf(buffer, "SD CARD id: \t%d", pfs->id);

	/* dbg_printf(buffer, "SD CARD sectors size: \t%lu", pfs->csize); */

	bufclear();
	free_space = (uint32_t) (pfs->free_clst * pfs->csize * 0.5);
	dbg_printf(buffer, "SD CARD Free Space: \t%lu", free_space);
}

/**
 * SD Card Timer callback
 */
void sdcardTimer_cb()
{
	FatFsCnt++;
	if (FatFsCnt >= 10) {
		FatFsCnt = 0;
		if (Timer1 > 0) {
			Timer1--;
		}
		if (Timer2 > 0) {
			Timer2--;
		}
	}
}

/**
 * Main SDCARD Service ionitialization function
 */
uint8_t uSdCardServiceInit()
{
	/* make 1ms task to handle SD Card  dedicated timer */
	osSDTimer1 = osTimerNew(sdcardTimer_cb, osTimerPeriodic, NULL, NULL);
	osTimerStart(osSDTimer1, 1U);

	DSTATUS my_status = SD_disk_initialize(0);

	if (my_status == STA_NOINIT) {
		dbg_printf("error in initializing SD CARD...\n");
	}

	if (my_status == STA_NODISK) {
		dbg_printf("No medium in the drive...\n");
	}

	if (my_status == STA_PROTECT) {
		dbg_printf(" Write protected...\n");
	}

	fresult = f_mount(&fs, "", 0);
	if (fresult != FR_OK) {
		dbg_printf("error in mounting SD CARD");
	} else {
		dbg_printf("SD CARD mounted succesfully...");
	}

	sdcard_check_free_space();
	/* sdcard_write_test(); */

	/* todo: error handler here if task is used */
	return (EXIT_SUCCESS);
}

