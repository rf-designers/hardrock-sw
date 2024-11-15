#include "menu_functions.h"

#include <amp_state.h>

#include "HR500.h"
#include "HR500V1.h"
#include <EEPROM.h>
#include <Wire.h>

#include "hr500_displays.h"

extern char *workingString;
extern char *workingString2;
extern amplifier amp;

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
            setup_acc_serial(amp.state.accSpeed);
            break;

        case mUSBbaud:
            if (ch_dir == 1)
                amp.state.usbSpeed = next_serial_speed(amp.state.usbSpeed);
            else
                amp.state.usbSpeed = prev_serial_speed(amp.state.usbSpeed);

            EEPROM.write(eeusbbaud, speed_to_eeprom(amp.state.usbSpeed));
            setup_usb_serial(amp.state.usbSpeed);
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
            amp.set_transceiver(amp.state.trx_type);
            break;

        case mATTN:
            if (amp.state.attenuator_present == 0) {
                item_disp[mATTN] = (char *) " NO ATTENUATOR  ";
            } else {
                if (amp.state.attenuator_enabled) {
                    amp.state.attenuator_enabled = false;
                    ATTN_ON_LOW;
                    item_disp[mATTN] = (char *) " ATTENUATOR OUT ";
                } else {
                    amp.state.attenuator_enabled = true;
                    ATTN_ON_HIGH;
                    item_disp[mATTN] = (char *) " ATTENUATOR IN  ";
                }

                EEPROM.write(eeattn, amp.state.attenuator_enabled);
                amp.state.old_alerts[alert_drive_pwr] = 5;
            }

            break;

        case mMCAL:
            if (ch_dir == 1) {
                amp.state.M_CORR++;
            } else {
                amp.state.M_CORR--;
            }

            if (amp.state.M_CORR > 125) {
                amp.state.M_CORR = long(125);
            }

            if (amp.state.M_CORR < 75) {
                amp.state.M_CORR = long(75);
            }

            byte MCAL = amp.state.M_CORR;
            EEPROM.write(eemcal, MCAL);
            sprintf(item_disp[mMCAL], "      %3ld       ", amp.state.M_CORR);
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

void setup_acc_serial(serial_speed speed) {
    Wire.beginTransmission(0x55); // arduino nano address
    Wire.write(0x03); // CMD_SET_BAUD
    Wire.write(static_cast<uint8_t>(speed));
    Wire.endTransmission();

    // Serial2.end();
    //
    // switch (speed) {
    //     case serial_speed::baud_4800:
    //         Serial2.begin(4800);
    //         strcpy(item_disp[mACCbaud], "   4800 Baud    ");
    //         break;
    //     case serial_speed::baud_9600:
    //         Serial2.begin(9600);
    //         strcpy(item_disp[mACCbaud], "   9600 Baud    ");
    //         break;
    //     case serial_speed::baud_19200:
    //         Serial2.begin(19200);
    //         strcpy(item_disp[mACCbaud], "   19200 Baud   ");
    //         break;
    //     case serial_speed::baud_38400:
    //         Serial2.begin(38400);
    //         strcpy(item_disp[mACCbaud], "   38400 Baud   ");
    //         break;
    //     case serial_speed::baud_57600:
    //     case serial_speed::baud_115200:
    //     default:
    //         // TODO: figure out what to do here
    //         break;
    // }
}

void setup_usb_serial(serial_speed speed) {
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
