/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-05-05 17:12:31
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-05 18:20:17
 * @ Description:
 *******************************************************************************************/

#include "packet_ptcl.h"
#include "remoteDebug_service.h"

void _add_start_frame(uint8_t *buffer);

uint8_t uEncodePacket(char *data)
{
    /* buffer to build the packet */
    uint8_t buffer[2048];

    /* first the 3 bytes for framle header */
    _add_start_frame(buffer);

    /* debug */
    uint8_t out[50];
    char outstr[50]; // for debug only
    for (uint8_t i = 0; i < sizeof(buffer) / sizeof(uint8_t); i++)
    {
        out[i] = buffer[i];
        sprintf(&outstr[i], "%x", buffer[i]);
    }

    debugI("%s", outstr); // for debug only

    return EXIT_SUCCESS;
}

void _add_start_frame(uint8_t *buffer)
{
    *(buffer) = 0xfe;
    *(buffer + 1) = 0xff;
    *(buffer + 2) = 0xfe;
}