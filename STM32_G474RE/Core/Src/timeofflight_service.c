/*
 * timeofflight_service.c
 *
 *  Created on: Apr 3, 2020
 *      Author: aez03
 */

#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "cmsis_os2.h"
#include "usart.h"
#include "FreeRTOS.h"
#include "timeofflight_service.h"
#include "freertos_logger_service.h"
#include "vl53l0x_api.h"

#define TOF_DEFAULT_ADDRESS			0x29U << 1
#define VL53L0X_SENSORS_NUMBER 		1

/* when not customized by application define dummy one */
#ifndef ST53L0A1_GetI2cBus
/**
 * macro that can be overloaded by user to enforce i2c sharing in RTOS context
 */
#define ST53L0A1_GetI2cBus(...) (void)0
#endif

#ifndef ST53L0A1_PutI2cBus
/** macro can be overloaded by user to enforce i2c sharing in RTOS context
 */
#   define ST53L0A1_PutI2cBus(...) (void)0
#endif

/**
 * GPIO monitor pin state register
 * 16 bit register LSB at lowest offset (little endian)
 */
#define GPMR    0x10
/**
 * STMPE1600 GPIO set pin state register
 * 16 bit register LSB at lowest offset (little endian)
 */
#define GPSR    0x12
/**
 * STMPE1600 GPIO set pin direction register
 * 16 bit register LSB at lowest offset
 */
#define GPDR    0x14

typedef enum {
	LONG_RANGE 		= 0, /*!< Long range mode */
	HIGH_SPEED 		= 1, /*!< High speed mode */
	HIGH_ACCURACY	= 2, /*!< High accuracy mode */
} RangingConfig_e;


/**
 * 53L0X Device selector
 *
 * @note Most functions are using a device selector as input. ASCII 'c', 'l' or 'r' are also accepted.
 */
enum ST53L0A1_dev_e{
	ST53L0A1_DEV_LEFT = 0,		/* left satellite device P21 header : 'l' */
	ST53L0A1_DEV_CENTER = 1,	/* center (built-in) vl053 device : 'c" */
	ST53L0A1_DEV_RIGHT = 2		/* Right satellite device P22 header : 'r' */
};

/**
 * cache the full set of expanded GPIO values to avoid i2c reading
 */
static union CurIOVal_u {
    uint8_t bytes[4];   /*!<  4 bytes array i/o view */
    uint32_t u32;       /*!<  single dword i/o view */
} CurIOVal; /** cache the extended IO values */


/* https://learn.adafruit.com/adafruit-vl53l0x-micro-lidar-distance-sensor-breakout/arduino-code */
/* https://github.com/adafruit/Adafruit_VL53L0X */
/* https://github.com/stm32duino/VL53L0X */

static I2C_HandleTypeDef *_hi2cx;

/** leaky factor for filtered range
 *
 * r(n) = averaged_r(n-1)*leaky +r(n)(1-leaky)
 *
 * */
static uint8_t LeakyFactorFix8 = (uint8_t)( 0.6 *256);

/** How many device detect set by @a DetectSensors()*/
static int nDevPresent=0;
/** bit is index in VL53L0XDevs that is not necessary the dev id of the BSP */
static uint8_t nDevMask;

static VL53L0X_Dev_t VL53L0XDevs[];

/* Forward definition of private function */
static uint8_t ST53L0A1_Init(void);
static int _ExpanderRd(int I2cExpAddr, int index, uint8_t *data, int n_data);
static int _ExpanderWR(int I2cExpAddr, int index, uint8_t *data, int n_data);
static int _ExpandersSetAllIO(void);

typedef enum
{
	timeofflightServiceNotInit,
	timeofflightServiceInitOK,
	timeofflightServiceInitError
}  timeofflightServiceStatus_t;
static  timeofflightServiceStatus_t  timeofflightServiceStatus =  timeofflightServiceNotInit;

/**
 * Definitions for lcdServiceTask
 */
typedef StaticTask_t osStaticThreadDef_t;
static osThreadId_t timeofflightServiceTaHandle;
static uint32_t timeofflightServiceTaBuffer[256];
static osStaticThreadDef_t timeofflightServiceTaControlBlock;
static const osThreadAttr_t timeofflightServiceTa_attributes = {
		.name = "timeofflightServiceTask",
		.stack_mem = &timeofflightServiceTaBuffer[0],
		.stack_size = sizeof(timeofflightServiceTaBuffer),
		.cb_mem = &timeofflightServiceTaControlBlock,
		.cb_size = sizeof(timeofflightServiceTaControlBlock),
		.priority = (osPriority_t) osPriorityLow, };

/**
 * timeofflight main task
 * @param argument
 */
void timeofflightService_task(void *argument)
{
	loggerI("Starting timeofflight task...");
	ST53L0A1_Init();
	for (;;) {


		osDelay(10);
	}
}

/**
 * Reset all sensor then do presence detection
 *
 * All present devices are data initiated and assigned to their final I2C address
 * @return
 */
void detectSensors(int SetDisplay)
{
	uint8_t i;
	uint16_t Id;
	uint8_t status;
	uint8_t FinalAddress;
	char errmsg[50];

	for (i = 0; i < 1; i++) {
		VL53L0X_Dev_t *pDev;
		pDev = &VL53L0XDevs[i];
		pDev->I2cDevAddr = TOF_DEFAULT_ADDRESS; /*0x52;*/
		pDev->Present = 0;
		status = ST53L0A1_ResetId( pDev->Id, 1);
		HAL_Delay(2);
		FinalAddress=0x52+(i+1)*2;

		do {
			/* Set I2C standard mode (400 KHz) before doing the first register access */
			if (status == VL53L0X_ERROR_NONE) {
				status = VL53L0X_WrByte(pDev, 0x88, 0x00);
			}
			/* Try to read one register using default 0x52 address */
			status = VL53L0X_RdWord(pDev, VL53L0X_REG_IDENTIFICATION_MODEL_ID, &Id);
			if (status) {
				sprintf(errmsg, "#%d Read id fail\n", i);
				loggerE(errmsg);
				break;
			}
			if (Id == 0xEEAA) {
				/* Sensor is found => Change its I2C address to final one */
				status = VL53L0X_SetDeviceAddress(pDev,FinalAddress);
				if (status != 0) {
					sprintf(errmsg, "#%d VL53L0X_SetDeviceAddress fail\n", i);
					loggerE(errmsg);
					break;
				}
				pDev->I2cDevAddr = FinalAddress;
				/* Check all is OK with the new I2C address and initialize the sensor */
				status = VL53L0X_RdWord(pDev, VL53L0X_REG_IDENTIFICATION_MODEL_ID, &Id);
				if (status != 0) {
					sprintf(errmsg, "#%d VL53L0X_RdWord fail\n", i);
					loggerE(errmsg);
					break;
				}

				status = VL53L0X_DataInit(pDev);
				if( status == 0 ){
					pDev->Present = 1;
				}
				else{
					sprintf(errmsg, "VL53L0X_DataInit %d fail\n", i);
					loggerE(errmsg);
					break;
				}
				sprintf(errmsg, "VL53L0X %d Present and initiated to final 0x%x\n", pDev->Id, pDev->I2cDevAddr);
				loggerI(errmsg);
				nDevPresent++;
				nDevMask |= 1 << i;
				pDev->Present = 1;
			}
			else {
				sprintf(errmsg, "#%d unknown ID %x\n", i, Id);
				loggerE(errmsg);
				status = 1;
			}
		} while (0);
		/* if fail r can't use for any reason then put the  device back to reset */
		if (status) {
			ST53L0A1_ResetId(i, 0);
		}
	}

}

/**
 *
 */
uint8_t timeofflight_initialize(I2C_HandleTypeDef *hi2cx)
{
	_hi2cx = hi2cx;

	/* creation of timeofflightServiceTask */
	timeofflightServiceTaHandle = osThreadNew(timeofflightService_task, NULL, &timeofflightServiceTa_attributes);
	if (!timeofflightServiceTaHandle) {
		timeofflightServiceStatus = timeofflightServiceInitError;
		loggerE("Initializing timeofflight Service - Failed");
		return (EXIT_FAILURE);
	}

	loggerI("Initializing timeofflight Service - Success!");
	return (EXIT_SUCCESS);
}

/**
 * Set Reset (XSDN) state of a given "id" device
 * @param  DevNo The device number use  @ref XNUCLEO53L0A1_dev_e. Char 't' 'c' 'r' can also be used
 * @param  state  State of the device reset (xsdn) pin @warning reset  pin is active low
 * @return 0 on success
 */
uint8_t ST53L0A1_ResetId(uint8_t DevNo, uint8_t state)
{
    uint8_t status;
    char errmsg[40];

    switch( DevNo ){
    case ST53L0A1_DEV_CENTER :
    case 'c' :
        CurIOVal.bytes[3]&=~0x80; /* bit 15 expender 1  => byte #3 */
        if( state ) {
            CurIOVal.bytes[3]|=0x80; /* bit 15 expender 1  => byte #3 */
        }
        status= _ExpanderWR(TOF_DEFAULT_ADDRESS, GPSR+1, &CurIOVal.bytes[3], 1);
        break;
    case ST53L0A1_DEV_LEFT :
    case 'l' :
        CurIOVal.bytes[1]&=~0x40; /* bit 14 expender 0 => byte #1*/
        if( state ) {
            CurIOVal.bytes[1]|=0x40; /* bit 14 expender 0 => byte #1*/
        }
        status= _ExpanderWR(TOF_DEFAULT_ADDRESS, GPSR+1, &CurIOVal.bytes[1], 1);
        break;
    case 'r' :
    case ST53L0A1_DEV_RIGHT :
        CurIOVal.bytes[1]&=~0x80; /* bit 15 expender 0  => byte #1 */
        if( state ) {
            CurIOVal.bytes[1]|=0x80; /* bit 15 expender 0 => byte #1*/
        }
        status= _ExpanderWR(TOF_DEFAULT_ADDRESS, GPSR+1, &CurIOVal.bytes[1], 1);
        break;
    default:
    	sprintf(errmsg, "Invalid DevNo %d", DevNo);
    	loggerE(errmsg);
        return EXIT_FAILURE;
    }
/*error with valid id */
    if( status ){
    	sprintf(errmsg, "expander i/o error for DevNo %d state %d ",DevNo, state);
    	loggerE(errmsg);
    }
    return status;
}

/**
 * STMPE1600  i2c Expender register read
 * @param I2cExpAddr Expender address
 * @param index      register index
 * @param data       read data buffer
 * @param n_data     number of byte to read
 * @return           of if ok else i2c I/O operation status
 */
static int _ExpanderRd(int I2cExpAddr, int index, uint8_t *data, int n_data) {

    int status;
    uint8_t RegAddr;
    RegAddr = index;
    ST53L0A1_GetI2cBus();
    do {
        status = HAL_I2C_Master_Transmit(_hi2cx, I2cExpAddr, &RegAddr, 1, 100);
        if (status) {}
            break;
        status = HAL_I2C_Master_Receive(_hi2cx, I2cExpAddr, data, n_data, n_data * 100);
    } while (0);
    ST53L0A1_PutI2cBus();
    return status;
}

/**
 * STMPE1600 i2c Expender register write
 * @param I2cExpAddr Expender address
 * @param index      register index
 * @param data       data buffer
 * @param n_data     number of byte to write
 * @return           of if ok else i2c I/O operation status
 */
static int _ExpanderWR(int I2cExpAddr, int index, uint8_t *data, int n_data) {

    int status;
    uint8_t RegAddr[0x10];
    RegAddr[0] = index;
    memcpy(RegAddr + 1, data, n_data);
    ST53L0A1_GetI2cBus();
    status = HAL_I2C_Master_Transmit(_hi2cx, I2cExpAddr, RegAddr, n_data + 1, 100);
    ST53L0A1_PutI2cBus();
    return status;
}

/**
 * Set all i2c expended gpio in one go
 * @return i/o operation status
 */
static int _ExpandersSetAllIO(void){
    int status;
    status = _ExpanderWR(TOF_DEFAULT_ADDRESS, GPSR, &CurIOVal.bytes[0], 2);
    if( status ){
        goto done_err;
    }
    status = _ExpanderWR(TOF_DEFAULT_ADDRESS, GPSR, &CurIOVal.bytes[2], 2);
done_err:
    return status;
}

/**
 *  Setup all detected sensors for single shot mode and setup ranging configuration
 */
void SetupSingleShot(RangingConfig_e rangingConfig){
    uint8_t i;
    char errmsg[50];
    uint8_t status;
    uint8_t VhvSettings;
    uint8_t PhaseCal;
    uint32_t refSpadCount;
	uint8_t isApertureSpads;
	FixPoint1616_t signalLimit = (FixPoint1616_t)(0.25*65536);
	FixPoint1616_t sigmaLimit = (FixPoint1616_t)(18*65536);
	uint32_t timingBudget = 33000;
	uint8_t preRangeVcselPeriod = 14;
	uint8_t finalRangeVcselPeriod = 10;

    for( i=0; i<3; i++){
        if( VL53L0XDevs[i].Present){
            status=VL53L0X_StaticInit(&VL53L0XDevs[i]);
            if( status ){
            	sprintf(errmsg, "VL53L0X_StaticInit %d failed\n",i);
            	loggerE(errmsg);
            }

            status = VL53L0X_PerformRefCalibration(&VL53L0XDevs[i], &VhvSettings, &PhaseCal);
			if( status ){
				sprintf(errmsg, "VL53L0X_PerformRefCalibration failed\n");
				loggerE(errmsg);
			}

			status = VL53L0X_PerformRefSpadManagement(&VL53L0XDevs[i], &refSpadCount, &isApertureSpads);
			if( status ){
				sprintf(errmsg, "VL53L0X_PerformRefSpadManagement failed\n");
				loggerE(errmsg);
			}

            status = VL53L0X_SetDeviceMode(&VL53L0XDevs[i], VL53L0X_DEVICEMODE_SINGLE_RANGING); /* Setup in single ranging mode */
            if( status ){
            	sprintf(errmsg, "VL53L0X_SetDeviceMode failed\n");
            	loggerE(errmsg);
            }

            status = VL53L0X_SetLimitCheckEnable(&VL53L0XDevs[i], VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, 1); /* Enable Sigma limit */
			if( status ){
				sprintf(errmsg, "VL53L0X_SetLimitCheckEnable failed\n");
				loggerE(errmsg);
			}

			status = VL53L0X_SetLimitCheckEnable(&VL53L0XDevs[i], VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, 1); /* Enable Signa limit */
			if( status ){
				sprintf(errmsg, "VL53L0X_SetLimitCheckEnable failed\n");
				loggerE(errmsg);
			}
			/* Ranging configuration */
            switch(rangingConfig) {
            case LONG_RANGE:
            	signalLimit = (FixPoint1616_t)(0.1*65536);
            	sigmaLimit = (FixPoint1616_t)(60*65536);
            	timingBudget = 33000;
            	preRangeVcselPeriod = 18;
            	finalRangeVcselPeriod = 14;
            	break;
            case HIGH_ACCURACY:
				signalLimit = (FixPoint1616_t)(0.25*65536);
				sigmaLimit = (FixPoint1616_t)(18*65536);
				timingBudget = 200000;
				preRangeVcselPeriod = 14;
				finalRangeVcselPeriod = 10;
				break;
            case HIGH_SPEED:
				signalLimit = (FixPoint1616_t)(0.25*65536);
				sigmaLimit = (FixPoint1616_t)(32*65536);
				timingBudget = 20000;
				preRangeVcselPeriod = 14;
				finalRangeVcselPeriod = 10;
				break;
            default:
            	loggerE("Not Supported");
            }

            status = VL53L0X_SetLimitCheckValue(&VL53L0XDevs[i],  VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, signalLimit);
			if( status ){
			   loggerE("VL53L0X_SetLimitCheckValue failed\n");
			}

			status = VL53L0X_SetLimitCheckValue(&VL53L0XDevs[i],  VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, sigmaLimit);
			if( status ){
			   loggerE("VL53L0X_SetLimitCheckValue failed\n");
			}

            status = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(&VL53L0XDevs[i],  timingBudget);
            if( status ){
               loggerE("VL53L0X_SetMeasurementTimingBudgetMicroSeconds failed\n");
            }

            status = VL53L0X_SetVcselPulsePeriod(&VL53L0XDevs[i],  VL53L0X_VCSEL_PERIOD_PRE_RANGE, preRangeVcselPeriod);
			if( status ){
			   loggerE("VL53L0X_SetVcselPulsePeriod failed\n");
			}

            status = VL53L0X_SetVcselPulsePeriod(&VL53L0XDevs[i],  VL53L0X_VCSEL_PERIOD_FINAL_RANGE, finalRangeVcselPeriod);
			if( status ){
			   loggerE("VL53L0X_SetVcselPulsePeriod failed\n");
			}

			status = VL53L0X_PerformRefCalibration(&VL53L0XDevs[i], &VhvSettings, &PhaseCal);
			if( status ){
			   loggerE("VL53L0X_PerformRefCalibration failed\n");
			}

            VL53L0XDevs[i].LeakyFirst=1;
        }
    }
}

/*char RangeToLetter(VL53L0X_Dev_t *pDev, VL53L0X_RangingMeasurementData_t *pRange){
    char c;
    if( pRange->RangeStatus == 0 ){
        if( pDev->LeakyRange < RangeLow ){
            c='_';
        }
        else if( pDev->LeakyRange < RangeMedium ){
                c='=';
        }
        else {
            c = '~';
        }

    }
    else{
        c='-';
    }
    return c;
}*/

/* Store new ranging data into the device structure, apply leaky integrator if needed */
void Sensor_SetNewRange(VL53L0X_Dev_t *pDev, VL53L0X_RangingMeasurementData_t *pRange){
    if( pRange->RangeStatus == 0 ){
        if( pDev->LeakyFirst ){
            pDev->LeakyFirst = 0;
            pDev->LeakyRange = pRange->RangeMilliMeter;
        }
        else{
            pDev->LeakyRange = (pDev->LeakyRange*LeakyFactorFix8 + (256-LeakyFactorFix8)*pRange->RangeMilliMeter)>>8;
        }
    }
    else{
        pDev->LeakyFirst = 1;
    }
}

/**
 *
 */
uint8_t ST53L0A1_Init(void) {
	char errmsg[50];
    uint8_t status;
    uint8_t ExpanderData[2];

    status = _ExpanderRd( TOF_DEFAULT_ADDRESS, 0, ExpanderData, 2);
    if (status != 0 || ExpanderData[0] != 0x00 || ExpanderData[1] != 0x16) {
    	sprintf (errmsg, "I2C Expander @0x%02X not detected",(int)TOF_DEFAULT_ADDRESS );
    	loggerE(errmsg);
        goto done_err;

    }
    status = _ExpanderRd( TOF_DEFAULT_ADDRESS, 0, ExpanderData, 2);
    if (status != 0 || ExpanderData[0] != 0x00 || ExpanderData[1] != 0x16) {
    	sprintf(errmsg, "I2C Expander @0x%02X not detected",(int)TOF_DEFAULT_ADDRESS);
    	loggerE(errmsg);
        goto done_err;
    }

    CurIOVal.u32=0x0;
    /* setup expender   i/o direction  all output but exp1 bit 14*/
    ExpanderData[0] = 0xFF;
    ExpanderData[1] = 0xFF;
    status = _ExpanderWR(TOF_DEFAULT_ADDRESS, GPDR, ExpanderData, 2);
    if (status) {
    	sprintf(errmsg, "Set Expander @0x%02X DR", TOF_DEFAULT_ADDRESS);
    	loggerE(errmsg);
        goto done_err;
    }
    ExpanderData[0] = 0xFF;
    ExpanderData[1] = 0xBF; /* all but bit 14-15 that is pb1 and xhurt */
    status = _ExpanderWR(TOF_DEFAULT_ADDRESS, GPDR, ExpanderData, 2);
    if (status) {
    	sprintf(errmsg, "Set Expander @0x%02X DR", TOF_DEFAULT_ADDRESS);
    	loggerE(errmsg);
        goto done_err;
    }
    /* shut down all segment and all device */
    CurIOVal.u32=0x7F + (0x7F<<7) + (0x7F<<16)+(0x7F<<(16+7));
    status= _ExpandersSetAllIO();
    if( status ){
        loggerI("Set initial i/o ");
    }

done_err:
    return status;
}


