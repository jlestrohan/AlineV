# AlineV - Please check dev branch

### Autonomous Low Impact Networked Eradication Vehicle Android

Protype built around:

- Caterpillar Chassis: https://www.ebay.fr/itm/292912268306


#### Microcontrollers:

*  Nucleo STM32G474RE 172Mhz (main controller)
*  ESP32 (Wifi)
*  Alternate ESP8266 version

#### Sensors:

*  LM393 x2     Independant caterpillar speed sensors
*  SHR-04 x3    Ultrasonic Distance Sensors
*  CMPS12       Telemetry ToF magnetometer sensor
*  Buzzer       Start up music (not avail in esp8266 version))
*  BME280       Temperature/Humidity/Pressure
*  UV Leds simulating huge UV C lamps (200 ~ 280nm)


#### Displays:
*  16x2 LCD I2C Display module


### Main features
#### Autonomous behaviour
    The device is able to recon its environment. Once started it initializes 
    itself and starts mapping the region around him, and records any of it's 
    movement on a AWS Dynamodb. The datas are processed thru a graphical 
    interface (redash) that offers some visuals.
    
It's integrated movement detection device makes it a perfect candidate for 
security patrol device which is then able to patrol along a predefined path or 
totally autonomously.

The motors are intelligently controlled so the device can respond to about every
critical event that can happen:

    * it will detect any derivation from the current heading and adjust its 
        trajectory accordingly until it meets an obstacle.if one of the caterpillar 
        is skidding for some reason and the android is starting to lose its heading,
        it will detect the fault and try to correct its trajectory by regulating 
        the motors power accordingly, guided by its inbuilt CMPS12 mag sensor
    *  it is able to record every single movement it makes so it is easy for it to
        just turn back and come back to a previous location with a relatively good
        accuracy (not implemented yet)
    * Once an obstacle has been detected from the front or rear sensors (bound to
        where it's actually headed) it will compute the best directional decision to 
        take next according to the side sensors data.
    * if it meets some kind of hole or dangerous terrain that could have dire 
        consequences upon the mission, it will try to avoid the obstacle and keep
        the previous heading once the obstacle have been overtaken, thanks to a 
        couple of obstacle sensors located underneath the chassis that will guard
        the device from falling from too height for instance. Thus it will also 
        deactivate if it detects any lift off or any foreign action taken against
        it, while keeping the video monitor active (if previously setup)
    * To preserve a constant speed and guarantee an accurate mapping of the place it
        is operating in, the device is equipped with a gyroscope/accelerometer that
        will let it adjust its speed either on flat or upward/downward sloping 
        terrains for instance. It will also detect it's current levelling and adapt
        the datas accordingly.
    * according to what has been defined when the device was setup and started, it can adopt
        different behaviours according to any event that may occur:
        
        ie:
            1.  one of the environmental sensor gets some critical values, is offline...
            2.  battery getting low, shall it return or stop ?
            3.  mapping completed, no known unmapped surface left, mapping lost...
            4.  control command received from the remote operator
            5.  data processing compromised (SD card full, no connectivity..)
            6.  movement or dire condition detected that requires human intervention
            7.  in the case of patrol mode, detection of some unattended event...
            etc...
            
    To be continued, edited.. and coded.... this project is far from over but 
    still in a very presentable way.
        
        Beside robotics, it is a real connected thing!
    
 
        
