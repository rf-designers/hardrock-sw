#include "menu_functions.h"

#include <amp_state.h>

#include "HR500.h"
#include "HR500V1.h"
#include <EEPROM.h>
#include "hr500_displays.h"

extern char *workingString;
extern char *workingString2;
extern char ATTN_P;
extern char ATTN_ST;
extern long M_CORR;
extern amplifier amp;

void set_transceiver(byte s_xcvr);

void menu_update(byte item, byte ch_dir) {
    amp.lcd[1].draw_string(item_disp[item], 65, 80, 2, MGRAY);

    switch (item) {
        case mACCbaud:
            if (amp.state.trx_type == xft817) break;

            if (ch_dir == 1)
                amp.state.accSpeed = next_serial_speed(amp.state.accSpeed);
            else
                amp.state.accSpeed = prev_serial_speed(amp.state.accSpeed);

            if (amp.state.accSpeed == serial_speed::baud_57600)
                amp.state.accSpeed = serial_speed::baud_4800;

            EEPROM.write(eeaccbaud, speed_to_eeprom(amp.state.accSpeed));
            SetupAccSerial(amp.state.accSpeed);
            break;

        case mUSBbaud:
            if (ch_dir == 1)
                amp.state.usbSpeed = next_serial_speed(amp.state.usbSpeed);
            else
                amp.state.usbSpeed = prev_serial_speed(amp.state.usbSpeed);

            EEPROM.write(eeusbbaud, speed_to_eeprom(amp.state.usbSpeed));
            SetupUSBSerial(amp.state.usbSpeed);
            break;

        case mXCVR:
            if (ch_dir == 1)
                amp.state.trx_type++;
            else
                amp.state.trx_type--;

            if (amp.state.trx_type == xcvr_max + 1)
                amp.state.trx_type = 0;

            if (amp.state.trx_type == 255)
                amp.state.trx_type = xcvr_max;

            strcpy(item_disp[mXCVR], xcvr_disp[amp.state.trx_type]);
            EEPROM.write(eexcvr, amp.state.trx_type);
            set_transceiver(amp.state.trx_type);
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
                amp.state.OD_alert = 5;
            }

            break;

        case mMCAL:
            if (ch_dir == 1) {
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

    amp.lcd[1].draw_string(item_disp[item], 65, 80, 2, WHITE);
}

void menuSelect() {
    if (!amp.state.menuSelected) {
        amp.state.menuSelected = true;
        amp.lcd[1].draw_string(menu_items[amp.state.menuChoice], 65, 20, 2, LGRAY);
        amp.lcd[1].fill_rect(14, 8, 41, 45, GRAY);
        amp.lcd[1].fill_rect(266, 8, 41, 45, GRAY);

        if (amp.state.menuChoice != mSETbias && amp.state.menuChoice != mATWfwl) {
            amp.lcd[1].draw_string(item_disp[amp.state.menuChoice], 65, 80, 2, WHITE);
            draw_button(amp.lcd[1], 14, 68, 40, 44);
            amp.lcd[1].draw_string("<", 24, 78, 4, GBLUE);
            draw_button(amp.lcd[1], 266, 68, 40, 44);
            amp.lcd[1].draw_string(">", 274, 78, 4, GBLUE);
        }

        amp.lcd[1].draw_string("SELECT", 124, 132, 2, GRAY);
        amp.lcd[1].draw_string("OK", 150, 132, 2, GBLUE);

        if (amp.state.menuChoice == mSETbias) {
            amp.state.old_mode = amp.state.mode;
            amp.state.mode = mode_type::ptt;
            amp.state.MAX_CUR = 3;
            amp.state.biasMeter = true;
            amp.lpf.send_relay_data(amp.lpf.serial_data + 0x02);
            BIAS_ON
        }
    } else {
        amp.lcd[1].draw_string("OK", 150, 132, 2, MGRAY);
        amp.state.menuSelected = 0;
        amp.lcd[1].draw_string(menu_items[amp.state.menuChoice], 65, 20, 2, WHITE);
        amp.lcd[1].draw_string(item_disp[amp.state.menuChoice], 65, 80, 2, LGRAY);
        amp.lcd[1].fill_rect(14, 68, 41, 45, GRAY);
        amp.lcd[1].fill_rect(266, 68, 41, 45, GRAY);
        draw_button(amp.lcd[1], 14, 8, 40, 44);
        amp.lcd[1].draw_string("<", 24, 18, 4, GBLUE);
        draw_button(amp.lcd[1], 266, 8, 40, 44);
        amp.lcd[1].draw_string(">", 274, 18, 4, GBLUE);

        if (amp.state.menuChoice == mSETbias) {
            BIAS_OFF
            amp.lpf.send_relay_data(amp.lpf.serial_data);
            amp.state.mode = amp.state.old_mode;
            amp.state.MAX_CUR = 20;
            amp.state.biasMeter = false;
            amp.lcd[1].fill_rect(62, 70, 196, 40, MGRAY);
        }

        if (amp.state.menuChoice == mATWfwl) {
            amp.lcd[1].draw_string("  Updating ATU  ", 65, 80, 2, WHITE);
            Serial.begin(19200);
            Serial3.println("*u");

            while (true) {
                if (Serial.available()) Serial3.write(Serial.read());
                if (Serial3.available()) Serial.write(Serial3.read());
            }
        }

        amp.lcd[1].draw_string("SELECT", 124, 132, 2, GBLUE);
    }
}

void SetupAccSerial(const serial_speed speed) {
    Serial2.end();

    switch (speed) {
        case serial_speed::baud_4800:
            Serial2.begin(4800);
            strcpy(item_disp[mACCbaud], "   4800 Baud    ");
            break;
        case serial_speed::baud_9600:
            Serial2.begin(9600);
            strcpy(item_disp[mACCbaud], "   9600 Baud    ");
            break;
        case serial_speed::baud_19200:
            Serial2.begin(19200);
            strcpy(item_disp[mACCbaud], "   19200 Baud   ");
            break;
        case serial_speed::baud_38400:
            Serial2.begin(38400);
            strcpy(item_disp[mACCbaud], "   38400 Baud   ");
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
            strcpy(item_disp[mUSBbaud], "   4800 Baud    ");
            break;
        case serial_speed::baud_9600:
            Serial.begin(9600);
            strcpy(item_disp[mUSBbaud], "   9600 Baud    ");
            break;
        case serial_speed::baud_19200:
            Serial.begin(19200);
            strcpy(item_disp[mUSBbaud], "   19200 Baud   ");
            break;
        case serial_speed::baud_38400:
            Serial.begin(38400);
            strcpy(item_disp[mUSBbaud], "   38400 Baud   ");
            break;
        case serial_speed::baud_57600:
            Serial.begin(57600);
            strcpy(item_disp[mUSBbaud], "   57600 Baud   ");
            break;
        case serial_speed::baud_115200:
            Serial.begin(115200);
            strcpy(item_disp[mUSBbaud], "  115200 Baud   ");
            break;
        default:
            // TODO: figure out what to do here
            break;
    }
}
