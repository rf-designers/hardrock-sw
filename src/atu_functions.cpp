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
extern display_board lcd[2];

void TuneButtonPressed() {
    while (amp.ts2.touched());

    lcd[1].fill_rect(121, 142, 74, 21, MGRAY);
    lcd[1].fill_rect(121, 199, 74, 21, GRAY);

    if (!amp.atu.isTuning()) {
        if (!amp.atu.isActive()) {
            amp.atu.setActive(true);
            draw_atu();
        }

        lcd[1].draw_string("STOP", 122, 199, 3, GBLUE);
        lcd[1].draw_string("TUNING", 122, 142, 2, LBLUE);
        amp.atu.setTuning(true);

        // If the TX is on, turn it off
        if (amp.state.txIsOn) {
            amp.state.pttEnabled = false;
            BIAS_OFF
            amp.state.txIsOn = false;
            amp.lpf.sendRelayData(amp.lpf.serialData);
            RF_BYPASS
        }

        ATU_TUNE_LOW // activate the tune line
        delay(5);
    } else {
        TuneEnd();
    }
}

void TuneEnd() {
    lcd[1].fill_rect(121, 142, 74, 21, MGRAY);
    lcd[1].fill_rect(121, 199, 74, 21, GRAY);
    lcd[1].draw_string("TUNE", 122, 199, 3, GBLUE);
    amp.atu.setTuning(false);
    ATU_TUNE_HIGH
    delay(10);

    amp.atu.query("*S", ATU_buff, 5);
    strcpy(amp.state.RL_TXT, "   ");
    ATU_STAT = ATU_buff[0];

    switch (ATU_STAT) {
        case 'F':
            lcd[1].draw_string("FAILED", 122, 142, 2, RED);
            break;
        case 'E':
            lcd[1].draw_string("HI SWR", 122, 142, 2, RED);
            break;
        case 'H':
            lcd[1].draw_string("HI PWR", 122, 142, 2, YELLOW);
            break;
        case 'L':
            lcd[1].draw_string("LO PWR", 122, 142, 2, YELLOW);
            break;
        case 'A':
            lcd[1].draw_string("CANCEL", 122, 142, 2, GREEN);
            break;
        case 'T':
        case 'S': {
            lcd[1].draw_string("TUNED", 128, 142, 2, GREEN);

            amp.atu.query("*F", ATU_buff, 8);

            unsigned char RL_CH = (ATU_buff[0] - 48) * 100 + (ATU_buff[1] - 48) * 10 + (ATU_buff[2] - 48);
            int SWR = swr[RL_CH] / 10;
            sprintf(amp.state.RL_TXT, "%d.%d", SWR / 10, SWR % 10);

            lcd[0].fill_rect(70, 203, 36, 16, MGRAY);
            lcd[0].draw_string(amp.state.RL_TXT, 70, 203, 2, GRAY);

            strcpy(amp.state.ORL_TXT, amp.state.RL_TXT);
            break;
        }

        default:
            // TODO: figure out what to do
            break;
    }

    amp.switchToRX(); // redraw the RX screen
}
