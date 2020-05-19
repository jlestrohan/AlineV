/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-21 14:26:21
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-19 14:25:21
 * @ Description:
 *******************************************************************************************/

#ifndef _IC_BUZMUSIC_H
#define _IC_BUZMUSIC_H

typedef enum
{
    MelodyType_WifiSuccess,
    MelodyType_CommandReceived,
    MelodyType_CommandReady,
    MelodyType_WrongArgument,  /* bad command argument */
    MelodyType_CommandFeedback /* feedback received from STM32 */
} melodyType_t;

uint8_t uSetupBuzzer();
uint8_t vPlayMelody(melodyType_t melody_type);

#endif /* _IC_BUZMUSIC_H */
