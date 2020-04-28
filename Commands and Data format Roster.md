## ESP32 <-> STM32 Commands and Data format Roster



Aside from the proprietary protocol used to let both micro-controllers share data in a full duplex bi-directional way, the following conventions have been setup:



### commands:

All commands are to be included between [CMD:(sha256)] **command here** [/CMD] tags

##### <u>emitted by the STM32 to ESP32:</u>

* **tune** *tunetype* *prioritary*

  *  will command the ESP32 to play the selected tune. 
  * *tunetype*: (string) name of the tune
  * *prioritary*: if true, this flag will order the ESP32 to immediately play the tune even if something else is playing

* **tune** *tunelist* - will command the ESP32 to send back an array of the tunes available

  * tunelist: keyword

  

##### <u>emitted by the ESP32 to STM32:</u>

- **restart**
  - restarts the rover, going thru all initialization process
- **sleep**
  - commands the STM32 to go into deepsleep mode to save battery
  - will be woke up either by a press on the user button, either any event incoming via network still to be defined
  - that command will also be invoked when the LiPo battery sensor detects a weak battery, in order to avoid the destruction of the lipo's cells. In such case a recognizable tune will also be played by the buzzer to inform the driver that the Lipo has to be changed.
- **wakeup**
  - commands the STM32 to exit deepsleep mode and become fully operational again

* **motion** *forward/backward* *milliseconds* *<u>speed</u>*

  * commands the rover to start moving backward or forward for x millis at the optional speed between 1 to 100 (0 will return a bad command error)

* **motion** *left/right millis <u>speed</u>*

  * commands the rover to start turning left or right for x millis at the optional speed 1-100 (0 would return a bad command error)

* **motion** *azimuth milliseconds <u>speed</u>*

  * commands the rover to start moving toward the azimuth given in degrees (magnetic) for x millis at the speed of (1-100, 0 returning a bad command error)

* **motion** *stop*

  * will immediately stop the rover commanding the motors to idle

* **lcd** *clear*

  * clears the LCD display

* **lcd** *cursor X Y*

  * sets LCD cursor to X Y

* **lcd** *print text*

  * prints "text" to the display, trimming anything over 16 chars

  

  





### datas:

All commands are to be included between [DTA:(sha256)] **{json data here}** [/DTA] tags. 

Datas are JSON preformatted and ready to be passed to the ESP32 for direct forwarding to MQTT service thru communication protocols available (BLE, BT, WiFi etc..)

No data is sent from the ESP32 to the STM32, all eventual data needed is included in the commands if applicable.

All SHA256 calculations are made counting every character included between the tags with brackets.

json format for STM general status:

```
{"STATE": {
	"timestamp": 1588046942,
	"free_heap": 2140,
    "ram_used": 10354,
    "ram_free": 595026,
    "sensor_voltage": 5,
    "core_temp_c": 47,
    "firmware_version": "1.12c",
    "last_updated": "2020-04-28T05:13:48",
    "device_uuid": "123e4567-e89b-12d3-a456-426655440000",
    },
 } 
  
    
    

```

​	json format for STM Sensor data (example)

```
{"DATA": {
	"timestamp": 1588046942,
    "device_uuid": "123e4567-e89b-12d3-a456-426655440000",
    "SENSOR": {
    		"sensorType": "gaz",
    		"module_id": "12BDE54",
    		"value": 23,
    		"unit": "C"
    	},
    },
 } 
  
    
    

```

​		json format for STM ACK result:

```
{"RESULT": {
	"timestamp": 1588046942,
    "device_uuid": "123e4567-e89b-12d3-a456-426655440000",
    "RESULT": {
    		"command": "motion forward 15000",
    		"result": "completed",
    		"time_completed_ms": 23254,
    	},
    },
 } 
  
    
    

```

​	



