/*
 * hdlc_protocol.c
 *
 *  Created on: May 6, 2020
 *      Author: Jack Lestrohan
 */

#include "hdlc_protocol.h"
#include <stdbool.h>
#include <stdlib.h>

/** HDLC Asynchronous framing */
/** The frame boundary octet is 01111110, (7E in hexadecimal notation) */
#define FRAME_BOUNDARY_OCTET 0x7E

/** A "control escape octet", has the bit sequence '01111101', (7D hexadecimal) */
#define CONTROL_ESCAPE_OCTET 0x7D

/** If either of these two octets appears in the transmitted data, an escape octet is sent, */
/** followed by the original data octet with bit 5 inverted */
#define INVERT_OCTET 0x20

/** The frame check sequence (FCS) is a 16-bit CRC-CCITT */
/** AVR Libc CRC function is _crc_ccitt_update() */
/** Corresponding CRC function in Qt (www.qt.io) is qChecksum() */
#define CRC16_CCITT_INIT_VAL 0xFFFF

/** 16bit low and high bytes copier */
#define low(x) ((x)&0xFF)
#define high(x) (((x) >> 8) & 0xFF)

#define lo8(x) ((x)&0xff)
#define hi8(x) ((x) >> 8)


/** User must define a function, that sends a 8bit char over the chosen interface, usart, spi, i2c etc. */
sendchar_type sendchar_function;

/** User must define a function, that will process the valid received frame */
/** This function can act like a command router/dispatcher */
frame_handler_type frame_handler;

void vSendchar(uint8_t data);

static bool _escape_character;
static uint8_t *_receive_frame_buffer;
static uint8_t _frame_position = 0;

/* 16bit CRC sum for _crc_ccitt_update */
static uint16_t _frame_checksum = CRC16_CCITT_INIT_VAL;
static uint16_t _max_frame_length = 0;


static uint16_t crc16_update(uint16_t crc, uint8_t a)
{
	int i;

	crc ^= a;
	for (i = 0; i < 8; ++i)
	{
		if (crc & 1)
			crc = (crc >> 1) ^ 0xA001;
		else
			crc = (crc >> 1);
	}

	return crc;
}

static uint16_t crc_xmodem_update(uint16_t crc, uint8_t data)
{
	int i;

	crc = crc ^ ((uint16_t)data << 8);
	for (i = 0; i < 8; i++)
	{
		if (crc & 0x8000)
			crc = (crc << 1) ^ 0x1021;
		else
			crc <<= 1;
	}

	return crc;
}
static uint16_t _crc_ccitt_update(uint16_t crc, uint8_t data)
{
	data ^= lo8(crc);
	data ^= data << 4;

	return ((((uint16_t)data << 8) | hi8(crc)) ^ (uint8_t)(data >> 4) ^ ((uint16_t)data << 3));
}

static uint8_t _crc_ibutton_update(uint8_t crc, uint8_t data)
{
	uint8_t i;

	crc = crc ^ data;
	for (i = 0; i < 8; i++)
	{
		if (crc & 0x01)
			crc = (crc >> 1) ^ 0x8C;
		else
			crc >>= 1;
	}

	return crc;
}

void uHdlcProtInit(sendchar_type put_char, frame_handler_type hdlc_command_router, uint8_t max_frame_length)
{
	sendchar_function = put_char;
	frame_handler = hdlc_command_router;
	_max_frame_length = max_frame_length;
	_receive_frame_buffer = (uint8_t *)malloc(max_frame_length + 1); // char *ab = (char*)malloc(12);
}

/* Function to send a byte throug USART, I2C, SPI etc.*/
void vSendchar(uint8_t data)
{
	(*sendchar_function)(data);
}

/* Function to find valid HDLC frame from incoming data */
void vCharReceiver(uint8_t data)
{
	/* FRAME FLAG */
	if (data == FRAME_BOUNDARY_OCTET)
	{
		if (_escape_character == true)
		{
			_escape_character = false;
		}
		/* If a valid frame is detected */
		else if ((_frame_position >= 2) &&
				(_frame_checksum == ((_receive_frame_buffer[_frame_position - 1] << 8) | (_receive_frame_buffer[_frame_position - 2] & 0xff)))) // (msb << 8 ) | (lsb & 0xff)
		{
			/* Call the user defined function and pass frame to it */
			(*frame_handler)(_receive_frame_buffer, (uint8_t)(_frame_position - 2));
		}
		_frame_position = 0;
		_frame_checksum = CRC16_CCITT_INIT_VAL;
		return;
	}

	if (_escape_character)
	{
		_escape_character = false;
		data ^= INVERT_OCTET;
	}
	else if (data == CONTROL_ESCAPE_OCTET)
	{
		_escape_character = true;
		return;
	}

	_receive_frame_buffer[_frame_position] = data;

	if (_frame_position - 2 >= 0)
	{
		_frame_checksum = _crc_ccitt_update(_frame_checksum, _receive_frame_buffer[_frame_position - 2]);
	}

	_frame_position++;

	if (_frame_position == _max_frame_length)
	{
		_frame_position = 0;
		_frame_checksum = CRC16_CCITT_INIT_VAL;
	}
}

/* Wrap given data in HDLC frame and send it out byte at a time*/
void vSendFrame(const uint8_t *framebuffer, uint8_t frame_length)
{
	uint8_t data;
	uint16_t fcs = CRC16_CCITT_INIT_VAL;

	vSendchar((uint8_t)FRAME_BOUNDARY_OCTET);

	while (frame_length)
	{
		data = *framebuffer++;
		fcs = _crc_ccitt_update(fcs, data);
		if ((data == CONTROL_ESCAPE_OCTET) || (data == FRAME_BOUNDARY_OCTET))
		{
			vSendchar((uint8_t)CONTROL_ESCAPE_OCTET);
			data ^= INVERT_OCTET;
		}
		vSendchar((uint8_t)data);
		frame_length--;
	}
	data = low(fcs);
	if ((data == CONTROL_ESCAPE_OCTET) || (data == FRAME_BOUNDARY_OCTET))
	{
		vSendchar((uint8_t)CONTROL_ESCAPE_OCTET);
		data ^= (uint8_t)INVERT_OCTET;
	}
	vSendchar((uint8_t)data);
	data = high(fcs);
	if ((data == CONTROL_ESCAPE_OCTET) || (data == FRAME_BOUNDARY_OCTET))
	{
		vSendchar(CONTROL_ESCAPE_OCTET);
		data ^= INVERT_OCTET;
	}
	vSendchar(data);
	vSendchar(FRAME_BOUNDARY_OCTET);
}
