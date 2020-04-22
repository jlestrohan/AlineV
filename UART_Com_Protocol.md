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

Any UART communication sent from the STM32 that is not encapsulated in the special tags is considered as debug console messages and directly derived to the telnet service of the ESP8266.
On the other way, anything that is sent to the STM32 that is not related to datas or commands is just ignored.

#### Special Flags

HashMD5 is a md5 hash of the content of the data transmitted between the tags. It's worth nothing to recall that it is absolutely mandatory to include a timestamp in any communication message, thus making every md5 checksum hash absolutely unique...



- [CMD:(hashmd5)]CMD[/CMD] = Command with md5 hash, implying an ACK response from the receiver
- [DTA:(hashmd5)]DATA[/DTA] = Data to be sent over (ie MQTT or whatever), can accept JSON formatted string
- [SYN] = First emitted by the sender to check if the other controller is in listen mode. It will reply by an [ACK] if applicable or an [ERR] flag if the communication is not possible
- [ACK:(hashmd5)] = Acknowledge flag (md5 hash is mandatory) is sent back in response to any message containing the same hashcode
- [RST] = Clears any ongoing communication process that could not be over and sets the status of the communication to Ready!
- [ERR:(hashmd5)]CODE[/ERR] = An error occured concerning the hashmd5 message..
  - 001: Last transmission failed
  - 002: Unknown command
  - 003: Missing closure timeout transmission dumped
  - 004: Invalid hashcode
  - 005: Unable to execute command



#### Commands:

All commands are encapsuled between [XXX] and [/XXX] tags, which means that any opened command must be terminated by a closing [/xxx] tag to be taken in account. 

A checksum hash number must be associated to the command code separated by a colon in order to offer a handshake check capability to the communication.

So a [CMD:md5_hash]{data}[/CMD] combination emitted by the sender may eventually require the receiver to send back an [ACK:md5_hash] single unclosed response so the sender can verify that the message has been fully received. Thus the sender can verify the good reception of any data packet sent in any order and close the transaction.

