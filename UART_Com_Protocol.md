# UART Communication protocol

### Protocol convention for com between STM32 and ESP8266

This protocol has been created by Jack Lestrohan, on April 15th 2020 and is aimed at structuring the communications between the core of an exploration Android (Alinea) and it's external com chip. It is only based of a personal need and only serves the purpose of making the whole thing just work! It is vaguely inspired by the TCP protocol except that we're not working at a bit level but a totally human readable exchange protocol for now.

Both micro controllers communicate asynchrounously and in a bi-directional way using a dedicated serial port:

	- 	UART0 on the ESP8266
	- 	HLPUART1 on STM32

Each one is able to send either data or commands to the other.

For instance the STM32 may send data to the ESP8266 to be immediately relayed thru Wifi/MQTT, and an orders to play some tune over (as it is dedicated to that eventually), while the ESP8266 may want to send a command to the STM32 to display an informative important message over to the LED display or relay orders coming from MQTT to control motion or other parameters for instance.

This is a real two way communication and every unit can handle RX/TX according to a uniformed protocol described below.

TELNET allows to send orders directly to the ESP8266 as well as having a serial console equivalent for debug or other stuff.

The protocol encapsulates a binary json encoded frame inside the following packets structure:

    -   3 start bytes pattern that indicate the start of a packet. This is the trigger telling the UART on the other side that a packet is incoming
    - -[0xFE][0xFF][0xFE]
    -   ID byte identifying the emmiting unit
    -   - [0xnn]
    -   16 bits encoded command ID that will identify the type of data. It is used to further decode the right type of data
    - -[0xmsb][0xlsb]
    -   the data itself, which is a binary serialization of a JSON document
    -   - [..................]
    -   16 bits size of the encoded json document that will make the first level of validation that we received the right packet
    -   - [0xmsb][0xlsb]
    -   32 bits (4 bytes) SHA256 of the json encoded document to avoid any corruption. 
    -   [0xnn][0xnn][0xnn][0xnn]
    -   termination pattern that indicates the end of the constructed packet
    -   - [0xEE][0xEF][0xEE]
    
    