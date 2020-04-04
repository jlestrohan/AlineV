/**
 ******************************************************************************
 * @file    mpu6050_service.c
 * @author  Jack Lestrohan
 * @brief   mpu6050 inertial module service file
 ******************************************************************************
 * @attention
 *
 *
 ******************************************************************************
 */

#include "mpu6050_service.h"
#include "cmsis_os2.h"
#include <FreeRTOS.h>
#include "lcd_service.h"
#include "i2c.h"
#include <stdio.h>
#include <stdlib.h>
#include "freertos_logger_service.h"

/* Default I2C address */
#define MPU6050_I2C_ADDR			0xD0

/* Who I am register value */
#define MPU6050_I_AM				0x68

/* MPU6050 registers */
#define MPU6050_AUX_VDDIO			0x01
#define MPU6050_SMPLRT_DIV			0x19
#define MPU6050_CONFIG				0x1A
#define MPU6050_GYRO_CONFIG			0x1B
#define MPU6050_ACCEL_CONFIG		0x1C
#define MPU6050_MOTION_THRESH		0x1F
#define MPU6050_INT_PIN_CFG			0x37
#define MPU6050_INT_ENABLE			0x38
#define MPU6050_INT_STATUS			0x3A
#define MPU6050_ACCEL_XOUT_H		0x3B
#define MPU6050_ACCEL_XOUT_L		0x3C
#define MPU6050_ACCEL_YOUT_H		0x3D
#define MPU6050_ACCEL_YOUT_L		0x3E
#define MPU6050_ACCEL_ZOUT_H		0x3F
#define MPU6050_ACCEL_ZOUT_L		0x40
#define MPU6050_TEMP_OUT_H			0x41
#define MPU6050_TEMP_OUT_L			0x42
#define MPU6050_GYRO_XOUT_H			0x43
#define MPU6050_GYRO_XOUT_L			0x44
#define MPU6050_GYRO_YOUT_H			0x45
#define MPU6050_GYRO_YOUT_L			0x46
#define MPU6050_GYRO_ZOUT_H			0x47
#define MPU6050_GYRO_ZOUT_L			0x48
#define MPU6050_MOT_DETECT_STATUS	0x61
#define MPU6050_SIGNAL_PATH_RESET	0x68
#define MPU6050_MOT_DETECT_CTRL		0x69
#define MPU6050_USER_CTRL			0x6A
#define MPU6050_PWR_MGMT_1			0x6B
#define MPU6050_PWR_MGMT_2			0x6C
#define MPU6050_FIFO_COUNTH			0x72
#define MPU6050_FIFO_COUNTL			0x73
#define MPU6050_FIFO_R_W			0x74
#define MPU6050_WHO_AM_I			0x75

/* Gyro sensitivities in degrees/s */
#define MPU6050_GYRO_SENS_250		((float) 131)
#define MPU6050_GYRO_SENS_500		((float) 65.5)
#define MPU6050_GYRO_SENS_1000		((float) 32.8)
#define MPU6050_GYRO_SENS_2000		((float) 16.4)

/* Acce sensitivities in g/s */
#define MPU6050_ACCE_SENS_2			((float) 16384)
#define MPU6050_ACCE_SENS_4			((float) 8192)
#define MPU6050_ACCE_SENS_8			((float) 4096)
#define MPU6050_ACCE_SENS_16		((float) 2048)

typedef StaticTask_t osStaticThreadDef_t;

/**
 * set both below accordingly
 */
static I2C_HandleTypeDef *_i2cxHandler;

/**
 * Definitions for mpu6050ServiceTask
 */
static osThreadId_t mpu6050ServiceTaHandle;
static uint32_t mpu6050ServiceTaBuffer[256];
static osStaticThreadDef_t mpu6050ServiceTaControlBlock;
static const osThreadAttr_t mpu6050ServiceTa_attributes = {
        .name = "mpu6050ServiceTask",
        .stack_mem = &mpu6050ServiceTaBuffer[0],
        .stack_size = sizeof(mpu6050ServiceTaBuffer),
        .cb_mem = &mpu6050ServiceTaControlBlock,
        .cb_size = sizeof(mpu6050ServiceTaControlBlock),
        .priority = (osPriority_t) osPriorityLow, };

/**
 * Service Main task
 * @param argument
 */
static void StartMPU6050ServiceTask(void *argument)
{
	loggerI("Starting mpu6050 service task...");
	MPU6050_Result result = {0};
	char res[100] = "";

	for (;;) {

		result = MPU6050_Init(&mpu1, MPU6050_Device_0, MPU6050_Accelerometer_2G, MPU6050_Gyroscope_250s);

		if (result != MPU6050_Result_Ok) {
			loggerI("result NOK");
		} else {
			osDelay(20);

			/*osSemaphoreAcquire(sem_lcdService, 0U); */
			/*MPU6050_ReadTemperature(&mpu1); */
			/*float temper = mpu1.Temperature; */
			/*sprintf(res, "temperature: %d", (int) temper); */
			/*lcd_send_string(res); */
			/*osSemaphoreRelease(sem_lcdService); */

			/*MPU6050_ReadGyroscope(&mpu1);
			 int16_t g_x = mpu1.Gyroscope_X;
			 int16_t g_y = mpu1.Gyroscope_Y;
			 int16_t g_z = mpu1.Gyroscope_Z;*/

			/*MPU6050_ReadAccelerometer(&mpu1); */
			/*int16_t a_x = mpu1.Accelerometer_X; */
			/*int16_t a_y = mpu1.Accelerometer_Y; */
			/*int16_t a_z = mpu1.Accelerometer_Z; */
			//sprintf(res, "%3d %3d %3d", g_x, g_y, g_z);
			/*sprintf(res, "accelX: %d; accelY: %d, accelZ: %d", a_x, a_y, a_z); */
			/*sprintf(res, "temperature: %g", mpu1.Temperature); */
			//loggerI(res);
			/*lcd_send_string(res); */
		}
		osDelay(200);
	}
}

/**
 * Initialize the whole service, tasks and stuff
 * @return
 */
MPU6050_Result MPU6050_Service_Initialize(I2C_HandleTypeDef *i2cxHandler)
{
	_i2cxHandler = i2cxHandler;
	/* creation of LoggerServiceTask */
	mpu6050ServiceTaHandle = osThreadNew(StartMPU6050ServiceTask, NULL, &mpu6050ServiceTa_attributes);
	if (!mpu6050ServiceTaHandle) {
		return (MPU6050_Result_ErrorHandlerNotInitialized);
		loggerI("MPU6050 Initialization failed...");
	}

	loggerI("MPU6050 Initialization completed...");
	return (MPU6050_Result_Ok);
}

/**
 * @brief  Initializes MPU6050 and I2C peripheral
 * @param  *DataStruct: Pointer to empty @ref MPU6050_t structure
 * @param  DeviceNumber: MPU6050 has one pin, AD0 which can be used to set address of device.
 *          This feature allows you to use 2 different sensors on the same board with same library.
 *          If you set AD0 pin to low, then this parameter should be MPU6050_Device_0,
 *          but if AD0 pin is high, then you should use MPU6050_Device_1
 *
 *          Parameter can be a value of @ref MPU6050_Device_t enumeration
 * @param  AccelerometerSensitivity: Set accelerometer sensitivity. This parameter can be a value of @ref MPU6050_Accelerometer_t enumeration
 * @param  GyroscopeSensitivity: Set gyroscope sensitivity. This parameter can be a value of @ref MPU6050_Gyroscope_t enumeration
 * @retval Initialization status:
 *            - MPU6050_Result_t: Everything OK
 *            - Other member: in other cases
 */
MPU6050_Result MPU6050_Init(MPU6050 *DataStruct, MPU6050_Device *DeviceNumber, MPU6050_Accelerometer *AccelerometerSensitivity, MPU6050_Gyroscope *GyroscopeSensitivity)
{

	uint8_t WHO_AM_I = (uint8_t) MPU6050_WHO_AM_I;
	uint8_t temp = 0;
	uint8_t d[2] = {0};

	/* Format I2C address */
	DataStruct->Address = MPU6050_I2C_ADDR | (uint8_t) *DeviceNumber;
	uint8_t address = DataStruct->Address;

	/* Check if device is connected */
	if (HAL_I2C_IsDeviceReady(_i2cxHandler, address, 2, 5) != HAL_OK) {
		return (MPU6050_Result_Error);
	}
	/* Check who am I */
	/* Send address */
	if (HAL_I2C_Master_Transmit(_i2cxHandler, address, &WHO_AM_I, 1, 1000) != HAL_OK) {
		return (MPU6050_Result_Error);
	}

	/* Receive multiple byte */
	if (HAL_I2C_Master_Receive(_i2cxHandler, address, &temp, 1, 1000) != HAL_OK) {
		return (MPU6050_Result_Error);
	}

	/* Checking */
	while (temp != MPU6050_I_AM) {
		/* Return error */
		return (MPU6050_Result_DeviceInvalid);
	}

	/* Wakeup MPU6050 */
	/* Format array to send */
	d[0] = MPU6050_PWR_MGMT_1;
	d[1] = 0x00;

	/* Try to transmit via I2C */
	if (HAL_I2C_Master_Transmit(_i2cxHandler, (uint16_t) address, (uint8_t*) d, 2, 1000) != HAL_OK) {
		return (MPU6050_Result_Error);
	}
	/* ------------------ */

	/* Set sample rate to 1kHz */
	MPU6050_SetDataRate(DataStruct, MPU6050_DataRate_1KHz);

	/* Config accelerometer */
	MPU6050_SetAccelerometer(DataStruct, *AccelerometerSensitivity);

	/* Config Gyroscope */
	MPU6050_SetGyroscope(DataStruct, *GyroscopeSensitivity);

	/* Return OK */
	return (MPU6050_Result_Ok);
}

/**
 * @brief  Sets output data rate
 * @param  *DataStruct: Pointer to @ref MPU6050_t structure indicating MPU6050 device
 * @param  rate: Data rate value. An 8-bit value for prescaler value
 * @retval Member of @ref MPU6050_Result_t enumeration
 */
MPU6050_Result MPU6050_SetDataRate(MPU6050 *DataStruct, uint8_t rate)
{

	uint8_t d[2] = {0};
	uint8_t address = DataStruct->Address;
	/* Format array to send */
	d[0] = MPU6050_SMPLRT_DIV;
	d[1] = rate;

	/* Set data sample rate */
	while (HAL_I2C_Master_Transmit(_i2cxHandler, (uint16_t) address, (uint8_t*) d, 2, 1000) != HAL_OK) {};
	/*{
	 return SD_MPU6050_Result_Error;
	 }*/

	/* Return OK */
	return (MPU6050_Result_Ok);
}

/**
 * @brief  Sets accelerometer sensitivity
 * @param  *DataStruct: Pointer to @ref MPU6050_t structure indicating MPU6050 device
 * @param  AccelerometerSensitivity: Gyro sensitivity value. This parameter can be a value of @ref MPU6050_Accelerometer_t enumeration
 * @retval Member of @ref MPU6050_Result_t enumeration
 */
MPU6050_Result MPU6050_SetAccelerometer(MPU6050 *DataStruct, MPU6050_Accelerometer AccelerometerSensitivity)
{

	uint8_t temp = 0;
	uint8_t address = DataStruct->Address;
	uint8_t regAdd = (uint8_t) MPU6050_ACCEL_CONFIG;

	/* Config accelerometer */
	while (HAL_I2C_Master_Transmit(_i2cxHandler, (uint16_t) address, &regAdd, 1, 1000) != HAL_OK) {};
	/*{
	 return SD_MPU6050_Result_Error;
	 }*/
	while (HAL_I2C_Master_Receive(_i2cxHandler, (uint16_t) address, &temp, 1, 1000) != HAL_OK) {};
	/*{
	 return SD_MPU6050_Result_Error;
	 }*/
	temp = (temp & 0xE7) | (uint8_t) AccelerometerSensitivity << 3;
	while (HAL_I2C_Master_Transmit(_i2cxHandler, (uint16_t) address, &temp, 1, 1000) != HAL_OK) {};
	/*{
	 return SD_MPU6050_Result_Error;
	 }*/

	/* Set sensitivities for multiplying gyro and accelerometer data */
	switch (AccelerometerSensitivity)
	{
		case MPU6050_Accelerometer_2G:
			DataStruct->Acce_Mult = (float) 1 / MPU6050_ACCE_SENS_2;
			break;
		case MPU6050_Accelerometer_4G:
			DataStruct->Acce_Mult = (float) 1 / MPU6050_ACCE_SENS_4;
			break;
		case MPU6050_Accelerometer_8G:
			DataStruct->Acce_Mult = (float) 1 / MPU6050_ACCE_SENS_8;
			break;
		case MPU6050_Accelerometer_16G:
			DataStruct->Acce_Mult = (float) 1 / MPU6050_ACCE_SENS_16;
			break;
		default:
			break;
	}

	/* Return OK */
	return (MPU6050_Result_Ok);
}

/**
 * @brief  Sets gyroscope sensitivity
 * @param  *DataStruct: Pointer to @ref MPU6050_t structure indicating MPU6050 device
 * @param  GyroscopeSensitivity: Gyro sensitivity value. This parameter can be a value of @ref MPU6050_Gyroscope_t enumeration
 * @retval Member of @ref MPU6050_Result_t enumeration
 */
MPU6050_Result MPU6050_SetGyroscope(MPU6050 *DataStruct, MPU6050_Gyroscope GyroscopeSensitivity)
{

	uint8_t temp = 0;
	uint8_t address = DataStruct->Address;
	uint8_t regAdd = (uint8_t) MPU6050_GYRO_CONFIG;

	/* Config gyroscope */
	while (HAL_I2C_Master_Transmit(_i2cxHandler, (uint16_t) address, &regAdd, 1, 1000) != HAL_OK) {};
	/*{
	 return MPU6050_Result_Error;
	 }*/
	while (HAL_I2C_Master_Receive(_i2cxHandler, (uint16_t) address, &temp, 1, 1000) != HAL_OK) {};
	/*{
	 return MPU6050_Result_Error;
	 }*/
	temp = (temp & 0xE7) | (uint8_t) GyroscopeSensitivity << 3;
	while (HAL_I2C_Master_Transmit(_i2cxHandler, (uint16_t) address, &temp, 1, 1000) != HAL_OK) {};
	/*{
	 return MPU6050_Result_Error;
	 }*/

	switch (GyroscopeSensitivity)
	{
		case MPU6050_Gyroscope_250s:
			DataStruct->Gyro_Mult = (float) 1 / MPU6050_GYRO_SENS_250;
			break;
		case MPU6050_Gyroscope_500s:
			DataStruct->Gyro_Mult = (float) 1 / MPU6050_GYRO_SENS_500;
			break;
		case MPU6050_Gyroscope_1000s:
			DataStruct->Gyro_Mult = (float) 1 / MPU6050_GYRO_SENS_1000;
			break;
		case MPU6050_Gyroscope_2000s:
			DataStruct->Gyro_Mult = (float) 1 / MPU6050_GYRO_SENS_2000;
			break;
		default:
			break;
	}
	/* Return OK */
	return (MPU6050_Result_Ok);
}

/**
 * @brief  Reads accelerometer data from sensor
 * @param  *DataStruct: Pointer to @ref MPU6050_t structure to store data to
 * @retval Member of @ref MPU6050_Result_t:
 *            - MPU6050_Result_Ok: everything is OK
 *            - Other: in other cases
 */
MPU6050_Result MPU6050_ReadAccelerometer(MPU6050 *DataStruct)
{

	uint8_t data[6] = {0};
	uint8_t reg = MPU6050_ACCEL_XOUT_H;
	uint8_t address = DataStruct->Address;

	/* Read accelerometer data */
	while (HAL_I2C_Master_Transmit(_i2cxHandler, (uint16_t) address, &reg, 1, 1000) != HAL_OK) {};

	while (HAL_I2C_Master_Receive(_i2cxHandler, (uint16_t) address, data, 6, 1000) != HAL_OK) {};

	/* Format */
	DataStruct->Accelerometer_X = (int16_t) (data[0] << 8 | data[1]);
	DataStruct->Accelerometer_Y = (int16_t) (data[2] << 8 | data[3]);
	DataStruct->Accelerometer_Z = (int16_t) (data[4] << 8 | data[5]);

	/* Return OK */
	return (MPU6050_Result_Ok);
}

/**
 * @brief  Reads gyroscope data from sensor
 * @param  *DataStruct: Pointer to @ref MPU6050_t structure to store data to
 * @retval Member of @ref MPU6050_Result_t:
 *            - MPU6050_Result_Ok: everything is OK
 *            - Other: in other cases
 */
MPU6050_Result MPU6050_ReadGyroscope(MPU6050 *DataStruct)
{
	uint8_t data[6] = {0};
	uint8_t reg = MPU6050_GYRO_XOUT_H;
	uint8_t address = DataStruct->Address;

	/* Read gyroscope data */
	while (HAL_I2C_Master_Transmit(_i2cxHandler, (uint16_t) address, &reg, 1, 1000) != HAL_OK) {};

	while (HAL_I2C_Master_Receive(_i2cxHandler, (uint16_t) address, data, 6, 1000) != HAL_OK) {};

	/* Format */
	DataStruct->Gyroscope_X = (int16_t) (data[0] << 8 | data[1]);
	DataStruct->Gyroscope_Y = (int16_t) (data[2] << 8 | data[3]);
	DataStruct->Gyroscope_Z = (int16_t) (data[4] << 8 | data[5]);

	/* Return OK */
	return (MPU6050_Result_Ok);
}

/**
 * @brief  Reads temperature data from sensor
 * @param  *DataStruct: Pointer to @ref MPU6050_t structure to store data to
 * @retval Member of @ref MPU6050_Result_t:
 *            - MPU6050_Result_Ok: everything is OK
 *            - Other: in other cases
 */
MPU6050_Result MPU6050_ReadTemperature(MPU6050 *DataStruct)
{
	if (!_i2cxHandler) { exit(-1);}

	uint8_t data[2] = {0};
	int16_t temp = 0;
	uint8_t reg = MPU6050_TEMP_OUT_H;
	uint8_t address = DataStruct->Address;

	/* Read temperature */
	while (HAL_I2C_Master_Transmit(_i2cxHandler, (uint16_t) address, &reg, 1, 1000) != HAL_OK) {};

	while (HAL_I2C_Master_Receive(_i2cxHandler, (uint16_t) address, data, 2, 1000) != HAL_OK) {};

	/* Format temperature */
	temp = (data[0] << 8 | data[1]);
	DataStruct->Temperature = (float) ((int16_t) temp / (float) 340.0 + (float) 36.53);

	/* Return OK */
	return (MPU6050_Result_Ok);
}

/**
 * @brief  Reads accelerometer, gyroscope and temperature data from sensor
 * @param  *DataStruct: Pointer to @ref MPU6050_t structure to store data to
 * @retval Member of @ref MPU6050_Result_t:
 *            - MPU6050_Result_Ok: everything is OK
 *            - Other: in other cases
 */
MPU6050_Result MPU6050_ReadAll(MPU6050 *DataStruct)
{

	uint8_t data[14] = {0};
	int16_t temp = 0;
	uint8_t reg = MPU6050_ACCEL_XOUT_H;
	uint8_t address = DataStruct->Address;

	/* Read full raw data, 14bytes */
	while (HAL_I2C_Master_Transmit(_i2cxHandler, (uint16_t) address, &reg, 1, 1000) != HAL_OK) {};

	while (HAL_I2C_Master_Receive(_i2cxHandler, (uint16_t) address, data, 14, 1000) != HAL_OK) {};

	/* Format accelerometer data */
	DataStruct->Accelerometer_X = (int16_t) (data[0] << 8 | data[1]);
	DataStruct->Accelerometer_Y = (int16_t) (data[2] << 8 | data[3]);
	DataStruct->Accelerometer_Z = (int16_t) (data[4] << 8 | data[5]);

	/* Format temperature */
	temp = (data[6] << 8 | data[7]);
	DataStruct->Temperature = (float) ((float) ((int16_t) temp) / (float) 340.0 + (float) 36.53);

	/* Format gyroscope data */
	DataStruct->Gyroscope_X = (int16_t) (data[8] << 8 | data[9]);
	DataStruct->Gyroscope_Y = (int16_t) (data[10] << 8 | data[11]);
	DataStruct->Gyroscope_Z = (int16_t) (data[12] << 8 | data[13]);

	/* Return OK */
	return (MPU6050_Result_Ok);
}

/**
 * @brief  Enables interrupts
 * @param  *DataStruct: Pointer to @ref MPU6050_t structure indicating MPU6050 device
 * @retval Member of @ref MPU6050_Result_t enumeration
 */
MPU6050_Result MPU6050_EnableInterrupts(MPU6050 *DataStruct)
{

	uint8_t temp = 0;
	uint8_t reg[2] = { MPU6050_INT_ENABLE, 0x21 };
	uint8_t address = DataStruct->Address;

	/* Enable interrupts for data ready and motion detect */
	while (HAL_I2C_Master_Transmit(_i2cxHandler, (uint16_t) address, reg, 2, 1000) != HAL_OK) {};

	uint8_t mpu_reg = MPU6050_INT_PIN_CFG;
	/* Clear IRQ flag on any read operation */
	while (HAL_I2C_Master_Transmit(_i2cxHandler, (uint16_t) address, &mpu_reg, 1, 1000) != HAL_OK) {};

	while (HAL_I2C_Master_Receive(_i2cxHandler, (uint16_t) address, &temp, 14, 1000) != HAL_OK) {};
	temp |= 0x10;
	reg[0] = MPU6050_INT_PIN_CFG;
	reg[1] = temp;
	while (HAL_I2C_Master_Transmit(_i2cxHandler, (uint16_t) address, reg, 2, 1000) != HAL_OK) {};

	/* Return OK */
	return (MPU6050_Result_Ok);
}

/**
 * @brief  Disables interrupts
 * @param  *DataStruct: Pointer to @ref MPU6050_t structure indicating MPU6050 device
 * @retval Member of @ref MPU6050_Result_t enumeration
 */
MPU6050_Result MPU6050_DisableInterrupts(MPU6050 *DataStruct)
{

	uint8_t reg[2] = { MPU6050_INT_ENABLE, 0x00 };
	uint8_t address = DataStruct->Address;

	/* Disable interrupts */
	while (HAL_I2C_Master_Transmit(_i2cxHandler, (uint16_t) address, reg, 2, 1000) != HAL_OK) {};
	/* Return OK */
	return (MPU6050_Result_Ok);
}

/**
 * @brief  Reads and clears interrupts
 * @param  *DataStruct: Pointer to @ref MPU6050_t structure indicating MPU6050 device
 * @param  *InterruptsStruct: Pointer to @ref MPU6050_Interrupt_t structure to store status in
 * @retval Member of @ref MPU6050_Result_t enumeration
 */
MPU6050_Result MPU6050_ReadInterrupts(MPU6050 *DataStruct, MPU6050_Interrupt *InterruptsStruct)
{

	uint8_t read = 0;

	/* Reset structure */
	InterruptsStruct->Status = 0;
	uint8_t reg = MPU6050_INT_STATUS;
	uint8_t address = DataStruct->Address;

	while (HAL_I2C_Master_Transmit(_i2cxHandler, (uint16_t) address, &reg, 1, 1000) != HAL_OK) {};

	while (HAL_I2C_Master_Receive(_i2cxHandler, (uint16_t) address, &read, 14, 1000) != HAL_OK) {};

	/* Fill value */
	InterruptsStruct->Status = read;
	/* Return OK */
	return (MPU6050_Result_Ok);
}

