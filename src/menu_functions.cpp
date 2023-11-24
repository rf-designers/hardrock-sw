#include "menu_functions.h"
#include "HR500.h"
#include "HR500V1.h"
#include <EEPROM.h>
#include "hr500_displays.h"
#include "HR500V1.h"

extern TFT Tft;
extern char *workingString;
extern char *workingString2;
extern byte XCVR;
extern byte ACC_Baud;
extern byte USB_Baud;
extern char ATTN_P;
extern char ATTN_ST;
extern byte I_alert, V_alert, F_alert, R_alert, D_alert;
extern byte OI_alert, OV_alert, OF_alert, OR_alert, OD_alert;
extern long M_CORR;
extern byte menuSEL;
extern byte menu_choice;
extern byte oMODE;
extern volatile byte MODE;
extern byte Bias_Meter;
extern int MAX_CUR;
extern volatile byte SR_DATA;

void SET_XCVR(byte s_xcvr);
void Send_RLY(byte data);

void menuFunction(byte item, byte ch_dir) {
    Tft.LCD_SEL = 1;
    Tft.drawString((uint8_t *)item_disp[item], 65, 80, 2, MGRAY);

    switch (item) {
        case mACCbaud:
            if (XCVR == xft817) break;

            if (ch_dir  == 1) ACC_Baud++;
            else ACC_Baud--;

            if (ACC_Baud == 4) ACC_Baud = 0;

            if (ACC_Baud == 255) ACC_Baud = 3;

            EEPROM.write(eeaccbaud, ACC_Baud);
            Set_Ser2(ACC_Baud);
            break;

        case mUSBbaud:
            if (ch_dir  == 1) USB_Baud++;
            else USB_Baud--;

            if (USB_Baud == 6) USB_Baud = 0;

            if (USB_Baud == 255) USB_Baud = 5;

            EEPROM.write(eeusbbaud, USB_Baud);
            Set_Ser(USB_Baud);
            break;

        case mXCVR:
            if (ch_dir  == 1) XCVR++;
            else XCVR--;

            if (XCVR == xcvr_max + 1) XCVR = 0;

            if (XCVR == 255) XCVR = xcvr_max;

            strcpy(item_disp[mXCVR], xcvr_disp[XCVR] );
            EEPROM.write(eexcvr, XCVR);
            SET_XCVR(XCVR);
            break;

        case mATTN:
            if (ATTN_P == 0) {
                item_disp[mATTN] = (char *)" NO ATTENUATOR  ";
            } else {
                if (ATTN_ST != 0) {
                    ATTN_ST = 0;
                    ATTN_ON_LOW;
                    item_disp[mATTN] = (char *)" ATTENUATOR OUT ";
                } else {
                    ATTN_ST = 1;
                    ATTN_ON_HIGH;
                    item_disp[mATTN] = (char *)" ATTENUATOR IN  ";
                }

                EEPROM.write(eeattn, ATTN_ST);
                OD_alert = 5;
            }

            break;

        case mMCAL:
            if (ch_dir == 1) M_CORR++;
            else M_CORR--;

            if (M_CORR > 125) M_CORR = long(125);

            if (M_CORR < 75) M_CORR = long(75);

            byte MCAL = M_CORR;
            EEPROM.write(eemcal, MCAL);
            sprintf(item_disp[mMCAL], "      %3ld       ", M_CORR);
            break;


    }

    Tft.drawString((uint8_t *)item_disp[item], 65, 80, 2, WHITE);
}

void menuSelect(void) {
    Tft.LCD_SEL = 1;

    if (menuSEL == 0) {
        menuSEL = 1;
        Tft.drawString((uint8_t *)menu_items[menu_choice], 65, 20, 2, LGRAY);
        Tft.lcd_fill_rect(14, 8, 41, 45, GRAY);
        Tft.lcd_fill_rect(266, 8, 41, 45, GRAY);

        if (menu_choice != mSETbias && menu_choice != mATWfwl) {
            Tft.drawString((uint8_t *)item_disp[menu_choice], 65, 80, 2, WHITE);
            DrawButton(14, 68, 40, 44);
            Tft.drawString((uint8_t *)"<", 24, 78,  4, GBLUE);
            DrawButton(266, 68, 40, 44);
            Tft.drawString((uint8_t *)">", 274, 78,  4, GBLUE);
        }

        Tft.drawString((uint8_t *)"SELECT", 124, 132, 2, GRAY);
        Tft.drawString((uint8_t *)"OK", 150, 132, 2, GBLUE);

        if (menu_choice == mSETbias) {
            oMODE = MODE;
            MODE = 1;
            MAX_CUR = 3;
            Bias_Meter = 1;
            Send_RLY(SR_DATA + 0x02);
            BIAS_ON
        }
    } else {
        Tft.drawString((uint8_t *)"OK", 150, 132, 2, MGRAY);
        menuSEL = 0;
        Tft.drawString((uint8_t *)menu_items[menu_choice], 65, 20, 2, WHITE);
        Tft.drawString((uint8_t *)item_disp[menu_choice], 65, 80, 2, LGRAY);
        Tft.lcd_fill_rect(14, 68, 41, 45, GRAY);
        Tft.lcd_fill_rect(266, 68, 41, 45, GRAY);
        DrawButton(14, 8, 40, 44);
        Tft.drawString((uint8_t *)"<", 24, 18,  4, GBLUE);
        DrawButton(266, 8, 40, 44);
        Tft.drawString((uint8_t *)">", 274, 18,  4, GBLUE);

        if (menu_choice == mSETbias) {
            BIAS_OFF
            Send_RLY(SR_DATA);
            MODE = oMODE;
            MAX_CUR = 20;
            Bias_Meter = 0;
            Tft.lcd_fill_rect(62, 70, 196, 40, MGRAY);
        }

        if (menu_choice == mATWfwl) {
            Tft.drawString((uint8_t *)"  Updating ATU  ", 65, 80, 2, WHITE);
            Serial.begin(19200);
            Serial3.println("*u");

            while (1) {
                if (Serial.available()) Serial3.write(Serial.read());

                if (Serial3.available()) Serial.write(Serial3.read());
            }
        }

        Tft.drawString((uint8_t *)"SELECT", 124, 132, 2, GBLUE);
    }

}

void Set_Ser2(byte BR) {
    Serial2.end();

    switch (BR) {
        case 0:
            Serial2.begin(4800);
            item_disp[mACCbaud] = (char *)"   4800 Baud    ";
            break;

        case 1:
            Serial2.begin(9600);
            item_disp[mACCbaud] = (char *)"   9600 Baud    ";
            break;

        case 2:
            Serial2.begin(19200);
            item_disp[mACCbaud] = (char *)"   19200 Baud   ";
            break;

        case 3:
            Serial2.begin(38400);
            item_disp[mACCbaud] = (char *)"   38400 Baud   ";
            break;
    }
}

void Set_Ser(byte BR) {
    Serial.end();

    switch (BR) {
        case 0:
            Serial.begin(4800);
            item_disp[mUSBbaud] = (char *)"   4800 Baud    ";
            break;

        case 1:
            Serial.begin(9600);
            item_disp[mUSBbaud] = (char *)"   9600 Baud    ";
            break;

        case 2:
            Serial.begin(19200);
            item_disp[mUSBbaud] = (char *)"   19200 Baud   ";
            break;

        case 3:
            Serial.begin(38400);
            item_disp[mUSBbaud] = (char *)"   38400 Baud   ";
            break;

        case 4:
            Serial.begin(57600);
            item_disp[mUSBbaud] = (char *)"   57600 Baud   ";
            break;

        case 5:
            Serial.begin(115200);
            item_disp[mUSBbaud] = (char *)"  115200 Baud   ";
            break;
    }
}
