/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-05-05 17:12:15
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-30 11:38:07
 * @ Description:
 *******************************************************************************************/

#ifndef _IC_HDLC_PROTOCOL_H
#define _IC_HDLC_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

typedef void (*sendchar_type)(uint8_t);
typedef void (*frame_handler_type)(uint8_t *framebuffer, size_t framelength);

class HDLC_Prot
{
public:
    HDLC_Prot(sendchar_type, frame_handler_type, size_t max_frame_length);
    ~HDLC_Prot();
    void charReceiver(uint8_t data);
    void sendFrame(const uint8_t *framebuffer, size_t frame_length);

private:
    // 16bit CRC sum for _crc_ccitt_update
    uint16_t frame_checksum;
    uint16_t max_frame_length;
    uint8_t *receive_frame_buffer;
    uint8_t frame_position;
    /* User must define a function, that sends a 8bit char over the chosen interface, usart, spi, i2c etc. */
    sendchar_type sendchar_function;
    /* User must define a function, that will process the valid received frame */
    /* This function can act like a command router/dispatcher */
    frame_handler_type frame_handler;
    void sendchar(uint8_t data);
    bool escape_character;
};

#endif /* _IC_HDLC_PROTOCOL_H */
