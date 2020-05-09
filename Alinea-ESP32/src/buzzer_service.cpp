/*******************************************************************************************
 * @ Author: Jack Lestrohan
 * @ Create Time: 2020-04-21 14:26:32
 * @ Modified by: Jack Lestrohan
 * @ Modified time: 2020-05-08 15:17:00
 * @ Description:
 *******************************************************************************************/

#include <Arduino.h>
#include "configuration_esp32.h"
#include "buzzer_service.h"
#include "remoteDebug_service.h"

// NB: ALL NOTES DEFINED WITH STANDARD ENGLISH NAMES, EXCEPT FROM "A"
#define note_C0 16.35
#define note_Db0 17.32
#define note_D0 18.35
#define note_Eb0 19.45
#define note_E0 20.60
#define note_F0 21.83
#define note_Gb0 23.12
#define note_G0 24.50
#define note_Ab0 25.96
#define note_A0 27.50
#define note_Bb0 29.14
#define note_B0 30.87
#define note_C1 32.70
#define note_Db1 34.65
#define note_D1 36.71
#define note_Eb1 38.89
#define note_E1 41.20
#define note_F1 43.65
#define note_Gb1 46.25
#define note_G1 49.00
#define note_Ab1 51.91
#define note_A1 55.00
#define note_Bb1 58.27
#define note_B1 61.74
#define note_C2 65.41
#define note_Db2 69.30
#define note_D2 73.42
#define note_Eb2 77.78
#define note_E2 82.41
#define note_F2 87.31
#define note_Gb2 92.50
#define note_G2 98.00
#define note_Ab2 103.83
#define note_A2 110.00
#define note_Bb2 116.54
#define note_B2 123.47
#define note_C3 130.81
#define note_Db3 138.59
#define note_D3 146.83
#define note_Eb3 155.56
#define note_E3 164.81
#define note_F3 174.61
#define note_Gb3 185.00
#define note_G3 196.00
#define note_Ab3 207.65
#define note_A3 220.00
#define note_Bb3 233.08
#define note_B3 246.94
#define note_C4 261.63
#define note_Db4 277.18
#define note_D4 293.66
#define note_Eb4 311.13
#define note_E4 329.63
#define note_F4 349.23
#define note_Gb4 369.99
#define note_G4 392.00
#define note_Ab4 415.30
#define note_A4 440.00
#define note_Bb4 466.16
#define note_B4 493.88
#define note_C5 523.25
#define note_Db5 554.37
#define note_D5 587.33
#define note_Eb5 622.25
#define note_E5 659.26
#define note_F5 698.46
#define note_Gb5 739.99
#define note_G5 783.99
#define note_Ab5 830.61
#define note_A5 880.00
#define note_Bb5 932.33
#define note_B5 987.77
#define note_C6 1046.50
#define note_Db6 1108.73
#define note_D6 1174.66
#define note_Eb6 1244.51
#define note_E6 1318.51
#define note_F6 1396.91
#define note_Gb6 1479.98
#define note_G6 1567.98
#define note_Ab6 1661.22
#define note_A6 1760.00
#define note_Bb6 1864.66
#define note_B6 1975.53
#define note_C7 2093.00
#define note_Db7 2217.46
#define note_D7 2349.32
#define note_Eb7 2489.02
#define note_E7 2637.02
#define note_F7 2793.83
#define note_Gb7 2959.96
#define note_G7 3135.96
#define note_Ab7 3322.44
#define note_A7 3520.01
#define note_Bb7 3729.31
#define note_B7 3951.07
#define note_C8 4186.01
#define note_Db8 4434.92
#define note_D8 4698.64
#define note_Eb8 4978.03

#define BUZZER_OUTPIN 23

#define BUZ_MELODY_MAX_NOTES 20

/**
 * @brief  Main melody struct definition
 * @note   
 * @retval None
 */
struct buzMelody_t
{
    double melody[BUZ_MELODY_MAX_NOTES];     /* the melody itself, array of notes limited in number of notes - todo: make that dynamic! */
    uint8_t durations[BUZ_MELODY_MAX_NOTES]; /* array of durations - todo make that dynamic! */
    size_t notesNumber;                      /* number of notes */
    uint8_t tempo;                           /* melody tempo between 0 and 255 BEWARE the littlest the fastest!! */
};

struct buzMelody_t mld_wifiSuccess = {
    {note_D5, note_E5, note_C5, note_G4, note_G3, note_C4, note_C5, note_G5}, {1, 1, 1, 1, 1, 1, 1, 1}, 8, 130};
struct buzMelody_t mld_cmdReceived = {
    {note_C5, note_G5, note_C6}, {1, 1, 1}, 3, 80};
struct buzMelody_t mld_cmdReady = {
    {note_C5, note_G5, note_E5, note_C6}, {1, 2, 1, 3}, 4, 120};
struct buzMelody_t mld_cmdFeedback = {
    {note_E6, note_C6, note_G6}, {1, 1, 1}, 3, 80};

struct buzMelody_t *melodyPtr;

/* functions definitions */
void vBuzzerTask(void *pvParameters);

// note, durée
//onst double melody[][3] = {{note_D5, 1}, {note_E5, 1}, {note_C5, 1}, {note_G4, 1}, {note_G3, 1}, {note_C4, 1}, {note_C5, 1}, {note_G5, 1}};

//const int notesNumber = 8;
//const int tempo = 200; /* the littlest the fastest */

xTaskHandle xBuzzerTask_handle = NULL;
QueueHandle_t xBuzzerMelodyQueue = NULL;

/**
 * @brief  Setup routine
 * @note   
 * @retval None
 */
uint8_t uSetupBuzzer()
{
    melodyPtr = &mld_cmdReady;       /* starting melody */
    ledcAttachPin(BUZZER_OUTPIN, 0); //broche 18 associée au canal PWM 0

    DEBUG_SERIAL("xBuzzerMelodyQueue ... creating")
    xBuzzerMelodyQueue = xQueueCreate(10, sizeof(buzMelody_t));
    if (xBuzzerMelodyQueue == NULL)
    {
        DEBUG_SERIAL("xBuzzerMelodyQueue ... Error");
    }
    DEBUG_SERIAL("xBuzzerMelodyQueue ... Success!");

    /* creates buzzer update task */
    xTaskCreate(
        vBuzzerTask,          /* Task function. */
        "vBuzzerTask",        /* String with name of task. */
        10000,                /* Stack size in words. */
        NULL,                 /* Parameter passed as input of the task */
        10,                   /* Priority of the task. */
        &xBuzzerTask_handle); /* Task handle. */

    if (xBuzzerTask_handle == NULL)
    {
        DEBUG_SERIAL("xBuzzerMelodyQueue ... Success!");
        return EXIT_FAILURE;
    }

    DEBUG_SERIAL("BuzzerService created and running..");
    return EXIT_SUCCESS;
}

/**
 * @brief  Buzzer one shot task, takes a melody struct in argument and then goes idle until next melody is requested
 * @note   
 * @param  *pvParameters: 
 * @retval None
 */
void vBuzzerTask(void *pvParameters)
{
    uint8_t idx = 0;
    int frequency;
    buzMelody_t melodyBuffer;

    for (;;)
    {
        /* waits for any melody */
        xQueueReceive(xBuzzerMelodyQueue, &melodyBuffer, portMAX_DELAY);

        for (int i = 0; i < melodyBuffer.notesNumber; i++)
        {
            frequency = melodyBuffer.melody[i];
            ledcSetup(0, frequency, 12);
            ledcWrite(0, 3592); // rapport cyclique 25%
            vTaskDelay(melodyBuffer.tempo * melodyBuffer.durations[i] - 50);
            ledcWrite(0, 0); // rapport cyclique 0% (silence, pour séparer les notes adjacentes)
            vTaskDelay(50);
            idx++;
        }
        vTaskDelay(10);
    }
    /* we never get here but if so... suicides itself */
    vTaskDelete(xBuzzerTask_handle);
}

/**
 * @brief  Melody when Wifi is connected
 * @note   
 * @retval None
 */
void wifiSuccessTune()
{
    /* let's setup ou melody here on purpose then we'll pass it to the main task */
    buzMelody_t wifiSuccessMelody = {
        {note_D5, note_E5, note_C5, note_G4, note_G3, note_C4, note_C5, note_G5}, {1, 1, 1, 1, 1, 1, 1, 1}, 8, 150};
    if (xBuzzerMelodyQueue != NULL)
    {
        xQueueSend(xBuzzerMelodyQueue, &wifiSuccessMelody, portMAX_DELAY);
    }
}

/**
 * @brief  Play Melody according to melody_type argument
 * @note   
 * @param  melody_type: 
 * @retval 
 */
uint8_t vPlayMelody(melodyType_t melody_type)
{
    switch (melody_type)
    {
    case MelodyType_CommandReady:
        melodyPtr = &mld_cmdReady;
        break;
    case MelodyType_CommandReceived:
        melodyPtr = &mld_cmdReceived;
        break;
    case MelodyType_WifiSuccess:
        melodyPtr = &mld_wifiSuccess;
        break;
    case MelodyType_CommandFeedback:
        melodyPtr = &mld_cmdFeedback;
        break;
    default:
        break;
    }

    if (xBuzzerMelodyQueue != NULL)
    {
        xQueueSend(xBuzzerMelodyQueue, melodyPtr, portMAX_DELAY);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
