# Alinea

### Autonomous Low Impact Networked Exploration Android

Protype built around:

- Caterpillar Chassis: https://www.ebay.fr/itm/292912268306

This device is aimed to map any indoor space, cartography its environment, 
while taking all sort of environmental datas, air quality, temperature, humidity,
etc...
Equipped with a 640x400 camera it can be effective in unaccessible areas as a
forefront explorer.

It is totally autonomous, althought it can be remotely controlled (Wifi/Bluetooth)
and will always try to control its whereabouts so that it can safely return back
to where it started.


#### Microcontrollers:

*  Nucleo STM32G474RE 172Mhz (main controller)
*  ESP32 (Wifi + BT + Camera Module)

#### Sensors:

*  LM393 x2      Independant wheel speed sensors + pwm control
*  HC-SR04 x3    Ultrasonic Distance Sensors
*  IR-08h        Obstacle Avoidance Sensor
*  VL53L0X       Time Of FLight sensor
*  MPU6050       Inertial Sensor
*  Barometric/Pressure sensor
*  Magnetic sensor
*  Buzzer    
*  SD Card Interface
*  

#### Batteries:
*  Lipo 4S/65C Battery for over one hour autonomy  

#### Motors:
*   2x PWM stepper motors controlling each caterpillar.

#### Displays:
*  16x2 LCD I2C Display module
  
#### Cloud/Gateway:
*  Able to send data over to it's gateway and/or MQTT endpoint
*  Video streaming included
