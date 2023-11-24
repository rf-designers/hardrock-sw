#include "atu_functions.h"
#include <Arduino.h>
#include "HR500X.h"
#include "HR500.h"
#include "hr500_displays.h"
#include "HR500V1.h"

extern char ATU_buff[40], ATU_cmd[8], ATU_cbuf[32], ATU_ver[8];
extern XPT2046_Touchscreen ts1;
extern XPT2046_Touchscreen ts2;
extern TFT Tft;
extern byte TUNING;
extern byte ATU;
extern byte TX;
extern volatile byte PTT;
extern volatile byte SR_DATA;
extern char RL_TXT[];
extern char ORL_TXT[];
extern char ATU_STAT;

void Switch_to_RX();

int ATU_exch(void) {
    size_t b_len;

    Serial3.setTimeout(50);
    Serial3.println(ATU_cmd);
    b_len = Serial3.readBytesUntil(0x13, ATU_buff, 28);
    ATU_buff[b_len] = 0;
    return b_len;
}

void Tune_button(void) {
    while (ts2.touched());

    Tft.LCD_SEL = 1;
    Tft.lcd_fill_rect(121, 142, 74, 21, MGRAY);
    Tft.lcd_fill_rect(121, 199, 74, 21, GRAY);


    if (TUNING == 0) {
        if (ATU == 0) {
            ATU = 1;
            DrawATU();
        }

        Tft.drawString((uint8_t *)"STOP", 122, 199,  3, GBLUE);
        Tft.drawString((uint8_t *)"TUNING", 122, 142,  2, LBLUE);
        TUNING = 1;

        if (TX == 1) { //If the TX is on, turn it off
            PTT = 0;
            BIAS_OFF
            TX = 0;
            byte SRSend_RLY = SR_DATA;
            RELAY_CS_LOW
            SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE3));
            SPI.transfer(SRSend_RLY);
            SPI.endTransaction();
            RELAY_CS_HIGH
            RF_BYPASS
        }

        ATU_TUNE_LOW // activate the tune line
        delay(5);
    } else Tune_End();
}

void Tune_End(void) {
    Tft.LCD_SEL = 1;
    Tft.lcd_fill_rect(121, 142, 74, 21, MGRAY);
    Tft.lcd_fill_rect(121, 199, 74, 21, GRAY);
    Tft.drawString((uint8_t *)"TUNE", 122, 199,  3, GBLUE);
    TUNING = 0;
    ATU_TUNE_HIGH
    delay(10);
    ATU_buff[0] = 0;
    Serial3.setTimeout(50);
    Serial3.println("*S");
    Serial3.readBytesUntil(0x13, ATU_buff, 5);

    strcpy(RL_TXT,"   ");
    ATU_STAT = ATU_buff[0];

    switch (ATU_STAT) {
        case 'F':
            Tft.drawString((uint8_t *)"FAILED", 122, 142,  2, RED);
            break;

        case 'E':
            Tft.drawString((uint8_t *)"HI SWR", 122, 142,  2, RED);
            break;

        case 'H':
            Tft.drawString((uint8_t *)"HI PWR", 122, 142,  2, YELLOW);
            break;

        case 'L':
            Tft.drawString((uint8_t *)"LO PWR", 122, 142,  2, YELLOW);
            break;

        case 'A':
            Tft.drawString((uint8_t *)"CANCEL", 122, 142,  2, GREEN);
            break;

        case 'T':
        case 'S':
            Tft.drawString((uint8_t *)"TUNED", 128, 142,  2, GREEN);
            Serial3.setTimeout(50);
            Serial3.println("*F");
            Serial3.readBytesUntil(0x13, ATU_buff, 8);
            unsigned char RL_CH = (ATU_buff[0] - 48) * 100 + (ATU_buff[1] - 48) * 10 + (ATU_buff[2] - 48);
            int SWR = swr[RL_CH]/10;
            sprintf(RL_TXT, "%d.%d", SWR/10, SWR % 10);
            Tft.LCD_SEL = 0;
            Tft.lcd_fill_rect(70, 203, 36, 16, MGRAY);
            Tft.drawString((uint8_t *)RL_TXT, 70, 203, 2, GRAY);
            strcpy(ORL_TXT, RL_TXT);
            break;
    }

    Switch_to_RX(); //redraw the RX screen

}
