#include "atu_functions.h"

#include <amp_state.h>
#include <Arduino.h>
#include "HR500X.h"
#include "HR500.h"
#include "hr500_displays.h"
#include "HR500V1.h"

char ATU_buff[40];
extern char ATU_STAT;
extern amplifier amp;

void on_tune_button_pressed() {
    while (amp.ts2.touched());

    amp.lcd[1].fill_rect(121, 142, 74, 21, MGRAY);
    amp.lcd[1].fill_rect(121, 199, 74, 21, GRAY);

    if (!amp.atu.is_tuning()) {
        if (!amp.atu.is_active()) {
            amp.atu.set_active(true);
            draw_atu();
        }

        amp.lcd[1].draw_string("STOP", 122, 199, 3, GBLUE);
        amp.lcd[1].draw_string("TUNING", 122, 142, 2, LBLUE);
        amp.atu.set_tuning(true);

        // If the TX is on, turn it off
        if (amp.state.tx_is_on) {
            amp.state.pttEnabled = false;
            BIAS_OFF
            amp.state.tx_is_on = false;
            amp.lpf.send_relay_data(amp.lpf.serial_data);
            RF_BYPASS
        }

        ATU_TUNE_LOW // activate the tune line
        delay(5);
    } else {
        on_tune_end();
    }
}

void on_tune_end() {
    amp.lcd[1].fill_rect(121, 142, 74, 21, MGRAY);
    amp.lcd[1].fill_rect(121, 199, 74, 21, GRAY);
    amp.lcd[1].draw_string("TUNE", 122, 199, 3, GBLUE);
    amp.atu.set_tuning(false);
    ATU_TUNE_HIGH
    delay(10);

    amp.atu.query("*S", ATU_buff, 5);
    strcpy(amp.state.RL_TXT, "   ");
    ATU_STAT = ATU_buff[0];

    switch (ATU_STAT) {
        case 'F':
            amp.lcd[1].draw_string("FAILED", 122, 142, 2, RED);
            break;
        case 'E':
            amp.lcd[1].draw_string("HI SWR", 122, 142, 2, RED);
            break;
        case 'H':
            amp.lcd[1].draw_string("HI PWR", 122, 142, 2, YELLOW);
            break;
        case 'L':
            amp.lcd[1].draw_string("LO PWR", 122, 142, 2, YELLOW);
            break;
        case 'A':
            amp.lcd[1].draw_string("CANCEL", 122, 142, 2, GREEN);
            break;
        case 'T':
        case 'S': {
            amp.lcd[1].draw_string("TUNED", 128, 142, 2, GREEN);

            amp.atu.query("*F", ATU_buff, 8);
            unsigned char RL_CH = (ATU_buff[0] - 48) * 100 + (ATU_buff[1] - 48) * 10 + (ATU_buff[2] - 48);
            int SWR = swr[RL_CH] / 10;
            sprintf(amp.state.RL_TXT, "%d.%d", SWR / 10, SWR % 10);

            amp.lcd[0].fill_rect(70, 203, 36, 16, MGRAY);
            amp.lcd[0].draw_string(amp.state.RL_TXT, 70, 203, 2, GRAY);

            strcpy(amp.state.ORL_TXT, amp.state.RL_TXT);
            break;
        }

        default:
            // TODO: figure out what to do
            break;
    }

    amp.switch_to_rx(); // redraw the RX screen
}
