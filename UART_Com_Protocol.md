# UART Communication protocol

### Protocol convention for com between STM32 and ESP8266

Custom and simplified Implementation of the HDLC Protocol (High-Level Data Link Control).
The communication between the STM32 and the ESP32 is a full-duplex bi-directional
data exchange "best-effort" mode (for now).

However as the lines between both MCU are very short, fixed, and thus very little prone to errors,
all the coding eforts are currently focused on data transmission to the cloud, and building the IA.

Encapsulated data is serialized JSON data:

#### Commands:

All datas/commands/sensors id etc..  MUST be defined each side and double checked.
For the sake of a lighter payload we just send very little information thru the UART.
Bitwise flags are used to be able to send several commands to several devices in a single
communication process.

Any MCU is able to either send data on its own. In the case of the STM32 we consider
that every data sent out of request is something he asks the ESP32 to relay to the cloud, unless
previously asked by the ESP32.

### General
    ##### Devices infos (variables included in all JSON)
    
    TIMESTAMP       (if no NTP is available at any point, this field will be -1 - both devices share the same clock, stm32 requests the esp anytime to have this updated)
    DEVICE_UUID     32 bits UUID
    UPTIME          in ms ticks

### STM32 
    ##### Addressable Devices: (uint16_t DeviceFlag)
    
        MOTOR_LEFT              =   1 << 0      /* control & speed info */
        MOTOR_RIGHT             =   1 << 1      /* control & speed info */
        FRONT_ULTRASONIC        =   1 << 2      /* control (on/off) & data (beware the fast acquisition vs the slow transmission) */
        BACK_ULTRASONIC         =   1 << 3      /* see above */
        BOTTOM_ULTRASONIC       =   1 << 4      /* see above */
        FRONT_SENSOR_SERVO      =   1 << 5      /* control & position (pwm value) */
        BMP280_SENSOR           =   1 << 6      /* control (on/off) & pressure information */
        MPU6050_SENSOR          =   1 << 7
        SDCARD_DEVICE           =   1 << 8      /* read file, status, control */     
        LED_ONBOARD             =   1 << 9
        LED_UART_EXTERNAL       =   1 << 10
        
        CORE_SYSTEM_STATUS      =   1 << 14     /* when requested infos are temperature, device uuid, active threads, avg load, uptime, etc etc... */
        CORE_SYSTEM_DEEPSLEEP   =   1 << 15     /* special flag, will take precedence over all the rest, when received the device goes into deepsleep mode!! */
        
### ESP32
    ##### Addressable Devices: (uint16_t DeviceFlag)
    
        BUZZER_DEVICE           =   1 << 0      /* play tune command essentially */
        
        CORE_SYSTEM_DATA        =   1 << 14     /* when requested infos are temperature, device uuid, active threads, avg load, uptime, etc etc... */
        CORE_SYSTEM_DEEPSLEEP   =   1 << 15     /* special flag, will take precedence over all the rest, when received the device goes into deepsleep mode!! */
        
    
### Common Actions: 
    ##### Actions(uint16_r cmdActionsFlag)
    
        CONTROL_OFF             =   1 << 0      /* Commands the device to turn on */
        CONTROL_ON              =   1 << 1      /* Commands the device to turn off - Beware, nav control will always have the last word. If you ask the bottom sensor 
                                                    to turn off, the Nav Control will immediately stop the engines to avoid putting the robot in a situation of danger.
                                                    Also, if you command the right motor to turn off, the robot will not hit something left if it detects an obstacle */
        REQ_STATUS              =   1 << 2      /* requests a full status, payload may vary according to the available data */
        REQ_DATA                =   1 << 3      /* requests data acquisition & transmission for the selected device(s)
        SEND_DATA               =   1 << 4      /* Informs the device that arguments are available along with the command.. ie Buzzer, play tune 6
        REQ_READY               =   1 << 5      /* requests the other MCU if it is ready to accept communications - adapt code accordingly */
        ACK_READY               =   1 << 6      /* acknowledge ready status */
        
### JSON Example:

    STM to >ESP32 example:
    {
        "Timestamp":    1588746512,
        "Uptime":   124098,
        deviceFlag: 1,      /* buzzer */
        actionFlags: 4,     /* we send data to the device */
        data:[
            "playtune": 3   /* will command the device to play tune 3 */
        ]
    
  
    to be edited ...
{
    