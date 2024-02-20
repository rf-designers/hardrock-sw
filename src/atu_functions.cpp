#include "atu_functions.h"

#include <amp_state.h>
#include <Arduino.h>
#include "HR500X.h"
#include "HR500.h"
#include "hr500_displays.h"
#include "HR500V1.h"

char ATU_buff[40];
extern XPT2046_Touchscreen ts1;
extern XPT2046_Touchscreen ts2;
extern TFT Tft;
extern volatile byte PTT;
extern char RL_TXT[];
extern char ORL_TXT[];
extern char ATU_STAT;
extern amp_state state;

void SwitchToRX();
void SendLPFRelayData(byte data);

size_t ATUQuery(const char* command, char* response, const size_t maxLength) {
    Serial3.setTimeout(50);
    Serial3.println(command);
    const size_t receivedBytes = Serial3.readBytesUntil(0x13, response, maxLength);
    response[receivedBytes] = 0;
    return receivedBytes;
}

void DetectATU() {
    Serial3.println(" ");
    strcpy(state.atuVersion, "---");

    if (ATUQuery("*I") > 10) {
        if (strncmp(ATU_buff, "HR500 ATU", 9) == 0) {
            state.isAtuPresent = true;
            const auto c = ATUQuery("*V");
            strncpy(state.atuVersion, ATU_buff, c - 1);
        }
    }
}

size_t ATUQuery(const char* command) {
    return ATUQuery(command, ATU_buff, 28);
}

void TuneButtonPressed() {
    while (ts2.touched());

    Tft.LCD_SEL = 1;
    Tft.lcd_fill_rect(121, 142, 74, 21, MGRAY);
    Tft.lcd_fill_rect(121, 199, 74, 21, GRAY);

    if (!state.isAtuTuning) {
        if (!state.isAtuActive) {
            state.isAtuActive = true;
            DrawATU();
        }

        Tft.drawString((uint8_t *) "STOP", 122, 199, 3, GBLUE);
        Tft.drawString((uint8_t *) "TUNING", 122, 142, 2, LBLUE);
        state.isAtuTuning = true;

        // If the TX is on, turn it off
        if (state.txIsOn) {
            PTT = 0;
            BIAS_OFF
            state.txIsOn = false;
            SendLPFRelayData(state.lpfBoardSerialData);
            RF_BYPASS
        }

        ATU_TUNE_LOW // activate the tune line
        delay(5);
    } else {
        TuneEnd();
    }
}

void TuneEnd() {
    Tft.LCD_SEL = 1;
    Tft.lcd_fill_rect(121, 142, 74, 21, MGRAY);
    Tft.lcd_fill_rect(121, 199, 74, 21, GRAY);
    Tft.drawString((uint8_t *) "TUNE", 122, 199, 3, GBLUE);
    state.isAtuTuning = false;
    ATU_TUNE_HIGH
    delay(10);


    ATUQuery("*S", ATU_buff, 5);
    strcpy(RL_TXT, "   ");
    ATU_STAT = ATU_buff[0];

    switch (ATU_STAT) {
        case 'F':
            Tft.drawString((uint8_t *) "FAILED", 122, 142, 2, RED);
            break;
        case 'E':
            Tft.drawString((uint8_t *) "HI SWR", 122, 142, 2, RED);
            break;
        case 'H':
            Tft.drawString((uint8_t *) "HI PWR", 122, 142, 2, YELLOW);
            break;
        case 'L':
            Tft.drawString((uint8_t *) "LO PWR", 122, 142, 2, YELLOW);
            break;
        case 'A':
            Tft.drawString((uint8_t *) "CANCEL", 122, 142, 2, GREEN);
            break;
        case 'T':
        case 'S': {
            Tft.drawString((uint8_t *) "TUNED", 128, 142, 2, GREEN);

            ATUQuery("*F", ATU_buff, 8);

            unsigned char RL_CH = (ATU_buff[0] - 48) * 100 + (ATU_buff[1] - 48) * 10 + (ATU_buff[2] - 48);
            int SWR = swr[RL_CH] / 10;
            sprintf(RL_TXT, "%d.%d", SWR / 10, SWR % 10);
            Tft.LCD_SEL = 0;
            Tft.lcd_fill_rect(70, 203, 36, 16, MGRAY);
            Tft.drawString((uint8_t *) RL_TXT, 70, 203, 2, GRAY);
            strcpy(ORL_TXT, RL_TXT);
            break;
        }

        default:
            // TODO: figure out what to do
            break;
    }

    SwitchToRX(); //redraw the RX screen
}
