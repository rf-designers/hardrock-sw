#include "menu_functions.h"

#include <amp_state.h>

#include "HR500.h"
#include "HR500V1.h"
#include <EEPROM.h>
#include "hr500_displays.h"

extern TFT Tft;
extern char* workingString;
extern char* workingString2;
extern char ATTN_P;
extern char ATTN_ST;
extern long M_CORR;
extern byte menuSEL;
extern byte menu_choice;
extern byte Bias_Meter;
extern int MAX_CUR;
extern amp_state state;

void setTransceiver(byte s_xcvr);

void SendLPFRelayData(byte data);

void menuUpdate(byte item, byte changeDirection) {
    Tft.LCD_SEL = 1;
    Tft.drawString((uint8_t *) item_disp[item], 65, 80, 2, MGRAY);

    switch (item) {
        case mACCbaud:
            if (state.trxType == xft817) break;

            if (changeDirection == 1)
                state.accSpeed = nextSerialSpeed(state.accSpeed);
            else
                state.accSpeed = previousSerialSpeed(state.accSpeed);

            if (state.accSpeed == serial_speed::baud_57600)
                state.accSpeed = serial_speed::baud_4800;

            EEPROM.write(eeaccbaud, speedToEEPROM(state.accSpeed));
            SetupAccSerial(state.accSpeed);
            break;

        case mUSBbaud:
            if (changeDirection == 1)
                state.usbSpeed = nextSerialSpeed(state.usbSpeed);
            else
                state.usbSpeed = previousSerialSpeed(state.usbSpeed);

            EEPROM.write(eeusbbaud, speedToEEPROM(state.usbSpeed));
            SetupUSBSerial(state.usbSpeed);
            break;

        case mXCVR:
            if (changeDirection == 1)
                state.trxType++;
            else
                state.trxType--;

            if (state.trxType == xcvr_max + 1)
                state.trxType = 0;

            if (state.trxType == 255)
                state.trxType = xcvr_max;

            strcpy(item_disp[mXCVR], xcvr_disp[state.trxType]);
            EEPROM.write(eexcvr, state.trxType);
            setTransceiver(state.trxType);
            break;

        case mATTN:
            if (ATTN_P == 0) {
                item_disp[mATTN] = (char *) " NO ATTENUATOR  ";
            } else {
                if (ATTN_ST != 0) {
                    ATTN_ST = 0;
                    ATTN_ON_LOW;
                    item_disp[mATTN] = (char *) " ATTENUATOR OUT ";
                } else {
                    ATTN_ST = 1;
                    ATTN_ON_HIGH;
                    item_disp[mATTN] = (char *) " ATTENUATOR IN  ";
                }

                EEPROM.write(eeattn, ATTN_ST);
                state.OD_alert = 5;
            }

            break;

        case mMCAL:
            if (changeDirection == 1) {
                M_CORR++;
            } else {
                M_CORR--;
            }

            if (M_CORR > 125) {
                M_CORR = long(125);
            }

            if (M_CORR < 75) {
                M_CORR = long(75);
            }

            byte MCAL = M_CORR;
            EEPROM.write(eemcal, MCAL);
            sprintf(item_disp[mMCAL], "      %3ld       ", M_CORR);
            break;
    }

    Tft.drawString((uint8_t *) item_disp[item], 65, 80, 2, WHITE);
}

void menuSelect() {
    Tft.LCD_SEL = 1;

    if (menuSEL == 0) {
        menuSEL = 1;
        Tft.drawString((uint8_t *) menu_items[menu_choice], 65, 20, 2, LGRAY);
        Tft.lcd_fill_rect(14, 8, 41, 45, GRAY);
        Tft.lcd_fill_rect(266, 8, 41, 45, GRAY);

        if (menu_choice != mSETbias && menu_choice != mATWfwl) {
            Tft.drawString((uint8_t *) item_disp[menu_choice], 65, 80, 2, WHITE);
            DrawButton(14, 68, 40, 44);
            Tft.drawString((uint8_t *) "<", 24, 78, 4, GBLUE);
            DrawButton(266, 68, 40, 44);
            Tft.drawString((uint8_t *) ">", 274, 78, 4, GBLUE);
        }

        Tft.drawString((uint8_t *) "SELECT", 124, 132, 2, GRAY);
        Tft.drawString((uint8_t *) "OK", 150, 132, 2, GBLUE);

        if (menu_choice == mSETbias) {
            state.old_mode = state.mode;
            state.mode = mode_type::ptt;
            MAX_CUR = 3;
            Bias_Meter = 1;
            SendLPFRelayData(state.lpfBoardSerialData + 0x02);
            BIAS_ON
        }
    } else {
        Tft.drawString((uint8_t *) "OK", 150, 132, 2, MGRAY);
        menuSEL = 0;
        Tft.drawString((uint8_t *) menu_items[menu_choice], 65, 20, 2, WHITE);
        Tft.drawString((uint8_t *) item_disp[menu_choice], 65, 80, 2, LGRAY);
        Tft.lcd_fill_rect(14, 68, 41, 45, GRAY);
        Tft.lcd_fill_rect(266, 68, 41, 45, GRAY);
        DrawButton(14, 8, 40, 44);
        Tft.drawString((uint8_t *) "<", 24, 18, 4, GBLUE);
        DrawButton(266, 8, 40, 44);
        Tft.drawString((uint8_t *) ">", 274, 18, 4, GBLUE);

        if (menu_choice == mSETbias) {
            BIAS_OFF
            SendLPFRelayData(state.lpfBoardSerialData);
            state.mode = state.old_mode;
            MAX_CUR = 20;
            Bias_Meter = 0;
            Tft.lcd_fill_rect(62, 70, 196, 40, MGRAY);
        }

        if (menu_choice == mATWfwl) {
            Tft.drawString((uint8_t *) "  Updating ATU  ", 65, 80, 2, WHITE);
            Serial.begin(19200);
            Serial3.println("*u");

            while (1) {
                if (Serial.available()) Serial3.write(Serial.read());

                if (Serial3.available()) Serial.write(Serial3.read());
            }
        }

        Tft.drawString((uint8_t *) "SELECT", 124, 132, 2, GBLUE);
    }
}

void SetupAccSerial(const serial_speed speed) {
    Serial2.end();

    switch (speed) {
        case serial_speed::baud_4800:
            Serial2.begin(4800);
            item_disp[mACCbaud] = (char *) "   4800 Baud    ";
            break;
        case serial_speed::baud_9600:
            Serial2.begin(9600);
            item_disp[mACCbaud] = (char *) "   9600 Baud    ";
            break;
        case serial_speed::baud_19200:
            Serial2.begin(19200);
            item_disp[mACCbaud] = (char *) "   19200 Baud   ";
            break;
        case serial_speed::baud_38400:
            Serial2.begin(38400);
            item_disp[mACCbaud] = (char *) "   38400 Baud   ";
            break;
        case serial_speed::baud_57600:
        case serial_speed::baud_115200:
        default:
            // TODO: figure out what to do here
            break;
    }
}

void SetupUSBSerial(const serial_speed speed) {
    Serial.end();

    switch (speed) {
        case serial_speed::baud_4800:
            Serial.begin(4800);
            item_disp[mUSBbaud] = (char *) "   4800 Baud    ";
            break;
        case serial_speed::baud_9600:
            Serial.begin(9600);
            item_disp[mUSBbaud] = (char *) "   9600 Baud    ";
            break;
        case serial_speed::baud_19200:
            Serial.begin(19200);
            item_disp[mUSBbaud] = (char *) "   19200 Baud   ";
            break;
        case serial_speed::baud_38400:
            Serial.begin(38400);
            item_disp[mUSBbaud] = (char *) "   38400 Baud   ";
            break;
        case serial_speed::baud_57600:
            Serial.begin(57600);
            item_disp[mUSBbaud] = (char *) "   57600 Baud   ";
            break;
        case serial_speed::baud_115200:
            Serial.begin(115200);
            item_disp[mUSBbaud] = (char *) "  115200 Baud   ";
            break;
        default:
            // TODO: figure out what to do here
            break;
    }
}
