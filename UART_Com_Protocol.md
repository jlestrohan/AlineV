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

#### Protocol definition
The protocol encapsulates a binary json encoded frame inside the following packets structure:

    -   3 start bytes pattern that indicate the start of a packet. This is the trigger telling the UART on the other side that a packet is incoming
    - -[0xFE][0xFF][0xFE]
    -   ID byte identifying the emmiting unit
    -   - [0xnn]
    -   8 bits encoded command ID that will identify the type of data. It is used to further decode the right type of data
    - -[0xnn]
    -   the data itself, which is a binary serialization of a JSON document
    -   - [..................]
    -   8 bits size crc
    -   - [0xnn]
    -   timeout expected by the sending unit for the acknowledgement. This int16 (2 bytes) is included so the receiving unit knows how much time it has been given within which he must respond to 
        the sending unit. Past this timeout, the receiver unit should respond with a "packet timeout error" special message
    -   - [0xmsb][0xlsb]
    -   termination pattern that indicates the end of the constructed packet
    -   - [0xEE][0xEF][0xEE]
    
    Example of a final constructed packet would be:
    [0xFE][0xFF][0xFE][ID][datatype1]datatype2][binary serialized json][SIZEmsb][SIZElsb][SHA256_1][SHA256_2]
[SHA256_3][SHA256_4][timeout][0xEE][0xEF][0xEE]     

#### Protocol Handshaking
Any received packet should be acknowledged by the receiving unit as follows:
    
    -   3 start bytes pattern that indicate the start of a packet. This is the trigger telling the UART on the other side that a packet is incoming
    - -[0xFE][0xFF][0xFE]
    -   ID byte identifying the emitting unit
    -   - [0xnn]
    -   16 bits encoded special command ID (ACK = 30300) or (ERR = 31311) that will identify the type of response
    - - [0xmsb][0xlsb]
    -   at this point the receiving parser should expect to receive a 32 bits (4 bytes) encoded SHA256 of the previous message to identify what message the acknowledgement/error relates to.
    -   - [0xnn][0xnn][0xnn][0xnn]
    termination pattern that indicates the end of the constructed packet
    -   - [0xEE][0xEF][0xEE]
    
    Once the acknowledgement has been received the sending unit may trash any trace of the sent message (or handle it the way it suits it best) or handle the error.
    