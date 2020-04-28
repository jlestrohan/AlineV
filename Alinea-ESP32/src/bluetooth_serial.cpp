/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-27 13:55:41
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-04-27 16:40:27
 * @ Description:
 *******************************************************************************************/

#include "bluetooth_serial.h"
#include "BluetoothSerial.h"
#include "configuration_esp32.h"
#include <stdlib.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

uint8_t setupBTSerial()
{
    SerialBT.begin(THINGNAME); //Bluetooth device name

    return EXIT_SUCCESS;
}