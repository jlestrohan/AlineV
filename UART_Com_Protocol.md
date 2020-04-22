# UART Communication protocol

### Protocol convention for com between STM32 and ESP8266

Both micro controllers communicate asynchrounously and in a bi-directional way using a dedicated serial port:

	- 	UART0 on the ESP8266
	- 	HLPUART1 on STM32

Each on is able to send either data or commands to the other.

For instance the STM32 may send data to the ESP8266 to be immediately relayed thru Wifi/MQTT, and an orders to play some tune over (as it is dedicated to that eventually), while the ESP8266 may want to send a command to the STM32 to display an informative important message over to the LED display or relay orders coming from MQTT to control motion or other parameters for instance.

This is a real two way communication and every unit can handle RX/TX according to a uniformed protocol described below.

TELNET allows to send orders directly to the ESP8266 as well as having a serial console equivalent for debug or other stuff.



