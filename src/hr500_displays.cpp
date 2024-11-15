#include "hr500_displays.h"

#include "amp_state.h"
#include "HR500.h"
#include "HR500V1.h"
#include "display_board.h"
#include <EEPROM.h>

extern amplifier amp;

void draw_home() {
    draw_button(amp.lcd[1], 5, 65, 118, 50);
    draw_button(amp.lcd[1], 197, 65, 58, 50);
    draw_button(amp.lcd[1], 257, 65, 58, 50);
    draw_button(amp.lcd[1], 5, 185, 88, 50);
    draw_button(amp.lcd[1], 227, 185, 88, 50);

    if (amp.atu.is_present()) {
        draw_button(amp.lcd[1], 130, 85, 60, 30);
        draw_button(amp.lcd[1], 116, 185, 88, 50);
        draw_panel(amp.lcd[1], 116, 128, 88, 50);
    } else {
        draw_button(amp.lcd[1], 130, 135, 60, 30);
    }

    draw_panel(amp.lcd[1], 5, 8, 118, 50);
    draw_panel(amp.lcd[1], 197, 8, 118, 50);
    draw_panel(amp.lcd[1], 5, 128, 88, 50);
    draw_panel(amp.lcd[1], 227, 128, 88, 50);
    draw_panel(amp.lcd[1], 140, 8, 40, 20);
    draw_tx_sensor(amp.state.colors.named.tx_sensor_rx);
    draw_rx_buttons(amp.state.colors.named.text_enabled);
    draw_ptt_mode();

    if (amp.state.band != 0)
        draw_band(amp.state.band, amp.state.colors.named.band_text);
    else
        draw_band(amp.state.band, amp.state.colors.named.alarm[3]);

    draw_ant();
    draw_atu();
}

void draw_menu() {
    draw_button(amp.lcd[1], 130, 205, 60, 30);
    amp.lcd[1].draw_string("EXIT", 135, 213, 2, amp.state.colors.named.text_enabled);

    draw_button(amp.lcd[1], 14, 8, 40, 44);
    amp.lcd[1].draw_string("<", 24, 18, 4, amp.state.colors.named.text_enabled);

    draw_button(amp.lcd[1], 266, 8, 40, 44);
    amp.lcd[1].draw_string(">", 274, 18, 4, amp.state.colors.named.text_enabled);

    draw_panel(amp.lcd[1], 60, 8, 200, 44);
    draw_panel(amp.lcd[1], 60, 68, 200, 44);

    amp.lcd[1].draw_string(menu_items[amp.state.menuChoice], 65, 20, 2, WHITE);
    amp.lcd[1].draw_string(item_disp[amp.state.menuChoice], 65, 80, 2, LGRAY);
    draw_button(amp.lcd[1], 120, 125, 80, 30);

    amp.lcd[1].draw_string("SELECT", 124, 132, 2, amp.state.colors.named.text_enabled);

    amp.state.menuSelected = false;

    amp.lcd[1].draw_string("ATU:", 206, 190, 2, LGRAY);
    amp.lcd[1].draw_string(amp.atu.get_version(), 254, 190, 2, LGRAY);
    amp.lcd[1].draw_string("FW:", 206, 213, 2, LGRAY);
    amp.lcd[1].draw_string(VERSION, 244, 213, 2, LGRAY);
}

void draw_meter() {
    amp.lcd[0].draw_string("FWD", 14, 8, 2, amp.state.colors.named.text_enabled);
    draw_panel(amp.lcd[0], 18, 32, 29, 14);

    amp.lcd[0].draw_string("RFL", 78, 8, 2, amp.state.colors.named.text_enabled);
    draw_panel(amp.lcd[0], 82, 32, 29, 14);

    amp.lcd[0].draw_string("DRV", 142, 8, 2, amp.state.colors.named.text_enabled);
    draw_panel(amp.lcd[0], 146, 32, 29, 14);

    amp.lcd[0].draw_string("VDD", 206, 8, 2, amp.state.colors.named.text_enabled);
    draw_panel(amp.lcd[0], 210, 32, 29, 14);

    amp.lcd[0].draw_string("IDD", 270, 8, 2, amp.state.colors.named.text_enabled);
    draw_panel(amp.lcd[0], 274, 32, 29, 14);

    draw_panel(amp.lcd[0], 7, 58, 304, 64);

    draw_button(amp.lcd[0], 10, 141, 44, 30);
    amp.lcd[0].draw_string("FWD", 13, 150, 2, amp.state.colors.named.text_enabled);

    draw_button(amp.lcd[0], 74, 141, 44, 30);
    amp.lcd[0].draw_string("RFL", 77, 150, 2, amp.state.colors.named.text_enabled);

    draw_button(amp.lcd[0], 138, 141, 44, 30);
    amp.lcd[0].draw_string("DRV", 141, 150, 2, amp.state.colors.named.text_enabled);

    draw_button(amp.lcd[0], 202, 141, 44, 30);
    amp.lcd[0].draw_string("VDD", 205, 150, 2, amp.state.colors.named.text_enabled);

    draw_button(amp.lcd[0], 266, 141, 44, 30);
    amp.lcd[0].draw_string("IDD", 269, 150, 2, amp.state.colors.named.text_enabled);

    draw_button_down(amp.state.meterSelection);
    amp.lcd[0].draw_string("SWR:", 10, 205, 2, amp.state.colors.named.text_enabled);

    draw_panel(amp.lcd[0], 63, 196, 80, 30);
    amp.lcd[0].draw_string("TEMP:", 165, 205, 2, amp.state.colors.named.text_enabled);
    draw_panel(amp.lcd[0], 230, 196, 80, 30);
}

void draw_rx_buttons(uint16_t color) {
    if (amp.state.is_menu_active) return;

    amp.lcd[1].draw_string("MODE", 26, 79, 3, color);
    amp.lcd[1].draw_string("<", 219, 79, 3, color);
    amp.lcd[1].draw_string(">", 279, 79, 3, color);
    amp.lcd[1].draw_string("ANT", 21, 199, 3, color);

    if (amp.atu.is_present()) {
        amp.lcd[1].draw_string("MENU", 135, 93, 2, color);
        amp.lcd[1].draw_string("TUNE", 122, 199, 3, amp.state.colors.named.text_enabled);
        //Tft.drawString((char*)"----", 122, 142,  3, LBLUE);
    } else {
        amp.lcd[1].draw_string("MENU", 135, 143, 2, color);
    }

    if (!amp.atu.is_present()) color = amp.state.colors.named.text_disabled;
    amp.lcd[1].draw_string("ATU", 244, 199, 3, color);
}

void draw_button(display_board &b, int x, int y, int w, int h) {
    if (h < 8 || w < 8) return;

//    b.draw_v_line(x, y, h, amp.state.colors.named.panel_fg2);
//    b.draw_v_line(x + 1, y + 1, h - 2, amp.state.colors.named.panel_fg2);
//    b.draw_v_line(x + w, y, h, amp.state.colors.named.panel_fg1);
//    b.draw_v_line(x + w - 1, y + 1, h - 2, amp.state.colors.named.panel_fg1);
//    b.draw_h_line(x, y, w, amp.state.colors.named.panel_fg2);
//    b.draw_h_line(x + 1, y + 1, w - 2, amp.state.colors.named.panel_fg2);
//    b.draw_h_line(x, y + h, w, amp.state.colors.named.panel_fg1);
//    b.draw_h_line(x + 1, y + h - 1, w - 2, amp.state.colors.named.panel_fg1);

    b.draw_h_line(x, y + h, w, amp.state.colors.named.button_bg);
    b.fill_rect(x + 2, y + 2, w - 4, h - 4, amp.state.colors.named.button_bg);
}

void draw_button_down(int button) {
    int x = (button - 1) * 64 + 10;
    int y = 141;
    int w = 44;
    int h = 30;
    int l_start = 83, l_height = 11, l_color = amp.state.colors.named.meter_ticks;

//    amp.lcd[0].draw_v_line(x, y, h, amp.state.colors.named.panel_fg1);
//    amp.lcd[0].draw_v_line(x + 1, y + 1, h - 2, amp.state.colors.named.panel_fg1);
//    amp.lcd[0].draw_v_line(x + w, y, h, amp.state.colors.named.panel_fg2);
//    amp.lcd[0].draw_v_line(x + w - 1, y + 1, h - 2, amp.state.colors.named.panel_fg2);
//
//    amp.lcd[0].draw_h_line(x, y, w, amp.state.colors.named.panel_fg1);
//    amp.lcd[0].draw_h_line(x + 1, y + 1, w - 2, amp.state.colors.named.panel_fg1);
//    amp.lcd[0].draw_h_line(x, y + h, w, amp.state.colors.named.panel_fg2);
//    amp.lcd[0].draw_h_line(x + 1, y + h - 1, w - 2, amp.state.colors.named.panel_fg2);

    amp.lcd[0].draw_h_line(x, y+h, w, amp.state.colors.named.alarm[2]);


    amp.lcd[0].fill_rect(9, 60, 301, 34, amp.state.colors.named.meter_bg);

    if (button == 1 || button == 2 || button == 3) {
        for (int i = 19; i < 300; i += 56) {
            amp.lcd[0].draw_v_line(i, l_start - 7, l_height + 7, l_color);
        }

        for (int i = 47; i < 300; i += 56) {
            amp.lcd[0].draw_v_line(i, l_start, l_height, l_color);
        }
    }

    l_start = 79;
    l_height = 15;

    if (button == 4 || button == 5) {
        if (button == 4) {
            for (int i = 19; i < 300; i += 28) {
                amp.lcd[0].draw_v_line(i, l_start, l_height, l_color);
            }

            l_start = 77;
            l_height = 12;
//            amp.lcd[0].display_string(16, 59, (uint8_t *) "0", FONT_1608, BLACK);
//            amp.lcd[0].display_string(30, 61, (uint8_t *) "VOLTS", FONT_1206, BLACK);
//            amp.lcd[0].display_string(67, 59, (uint8_t *) "12", FONT_1608, BLACK);
//            amp.lcd[0].display_string(123, 59, (uint8_t *) "24", FONT_1608, BLACK);
//            amp.lcd[0].display_string(179, 59, (uint8_t *) "36", FONT_1608, BLACK);
//            amp.lcd[0].display_string(235, 59, (uint8_t *) "48", FONT_1608, BLACK);
//            amp.lcd[0].display_string(291, 59, (uint8_t *) "60", FONT_1608, BLACK);
        }

        if (button == 5) {
            for (int i = 19; i < 300; i += 14) {
                amp.lcd[0].draw_v_line(i, l_start, l_height, l_color);
            }

//            amp.lcd[0].display_string(16, 59, (uint8_t *) "0", FONT_1608, BLACK);
//            amp.lcd[0].display_string(40, 61, (uint8_t *) "AMPS", FONT_1206, BLACK);
//            amp.lcd[0].display_string(86, 59, (uint8_t *) "5", FONT_1608, BLACK);
//            amp.lcd[0].display_string(151, 59, (uint8_t *) "10", FONT_1608, BLACK);
//            amp.lcd[0].display_string(221, 59, (uint8_t *) "15", FONT_1608, BLACK);
//            amp.lcd[0].display_string(291, 59, (uint8_t *) "20", FONT_1608, BLACK);
        }
    }

    switch (button) {
        case 1:
            amp.lcd[0].display_string(16, 59, (uint8_t *) "0", FONT_1608, amp.state.colors.named.meter_ticks);
            amp.lcd[0].display_string(30, 61, (uint8_t *) "WATTS", FONT_1206, amp.state.colors.named.meter_ticks);
            amp.lcd[0].display_string(64, 59, (uint8_t *) "100", FONT_1608, amp.state.colors.named.meter_ticks);
            amp.lcd[0].display_string(120, 59, (uint8_t *) "200", FONT_1608, amp.state.colors.named.meter_ticks);
            amp.lcd[0].display_string(176, 59, (uint8_t *) "300", FONT_1608, amp.state.colors.named.meter_ticks);
            amp.lcd[0].display_string(232, 59, (uint8_t *) "400", FONT_1608, amp.state.colors.named.meter_ticks);
            amp.lcd[0].display_string(286, 59, (uint8_t *) "500", FONT_1608, amp.state.colors.named.meter_ticks);
            break;
        case 2:
            amp.lcd[0].display_string(16, 59, (uint8_t *) "0", FONT_1608, amp.state.colors.named.meter_ticks);
            amp.lcd[0].display_string(30, 61, (uint8_t *) "WATTS", FONT_1206, amp.state.colors.named.meter_ticks);
            amp.lcd[0].display_string(67, 59, (uint8_t *) "10", FONT_1608, amp.state.colors.named.meter_ticks);
            amp.lcd[0].display_string(123, 59, (uint8_t *) "20", FONT_1608, amp.state.colors.named.meter_ticks);
            amp.lcd[0].display_string(179, 59, (uint8_t *) "30", FONT_1608, amp.state.colors.named.meter_ticks);
            amp.lcd[0].display_string(235, 59, (uint8_t *) "40", FONT_1608, amp.state.colors.named.meter_ticks);
            amp.lcd[0].display_string(291, 59, (uint8_t *) "50", FONT_1608, amp.state.colors.named.meter_ticks);
            break;

        case 3:
            amp.lcd[0].display_string(16, 59, (uint8_t *) "0", FONT_1608, amp.state.colors.named.meter_ticks);
            amp.lcd[0].display_string(30, 61, (uint8_t *) "WATTS", FONT_1206, amp.state.colors.named.meter_ticks);
            amp.lcd[0].display_string(72, 59, (uint8_t *) "2", FONT_1608, amp.state.colors.named.meter_ticks);
            amp.lcd[0].display_string(128, 59, (uint8_t *) "4", FONT_1608, amp.state.colors.named.meter_ticks);
            amp.lcd[0].display_string(184, 59, (uint8_t *) "6", FONT_1608, amp.state.colors.named.meter_ticks);
            amp.lcd[0].display_string(240, 59, (uint8_t *) "8", FONT_1608, amp.state.colors.named.meter_ticks);
            amp.lcd[0].display_string(291, 59, (uint8_t *) "10", FONT_1608, amp.state.colors.named.meter_ticks);
            break;
        case 4:
            amp.lcd[0].display_string(16, 59, (uint8_t *) "0", FONT_1608, amp.state.colors.named.meter_ticks);
            amp.lcd[0].display_string(30, 61, (uint8_t *) "VOLTS", FONT_1206, amp.state.colors.named.meter_ticks);
            amp.lcd[0].display_string(67, 59, (uint8_t *) "12", FONT_1608, amp.state.colors.named.meter_ticks);
            amp.lcd[0].display_string(123, 59, (uint8_t *) "24", FONT_1608, amp.state.colors.named.meter_ticks);
            amp.lcd[0].display_string(179, 59, (uint8_t *) "36", FONT_1608, amp.state.colors.named.meter_ticks);
            amp.lcd[0].display_string(235, 59, (uint8_t *) "48", FONT_1608, amp.state.colors.named.meter_ticks);
            amp.lcd[0].display_string(291, 59, (uint8_t *) "60", FONT_1608, amp.state.colors.named.meter_ticks);
            break;
        case 5:
            amp.lcd[0].display_string(16, 59, (uint8_t *) "0", FONT_1608, amp.state.colors.named.meter_ticks);
            amp.lcd[0].display_string(40, 61, (uint8_t *) "AMPS", FONT_1206, amp.state.colors.named.meter_ticks);
            amp.lcd[0].display_string(86, 59, (uint8_t *) "5", FONT_1608, amp.state.colors.named.meter_ticks);
            amp.lcd[0].display_string(151, 59, (uint8_t *) "10", FONT_1608, amp.state.colors.named.meter_ticks);
            amp.lcd[0].display_string(221, 59, (uint8_t *) "15", FONT_1608, amp.state.colors.named.meter_ticks);
            amp.lcd[0].display_string(291, 59, (uint8_t *) "20", FONT_1608, amp.state.colors.named.meter_ticks);
            break;
        default:
            break;
    }

}

void draw_button_up(const int button) {
    int x = (button - 1) * 64 + 10;
    int y = 141;
    int w = 44;
    int h = 30;

//    amp.lcd[0].draw_v_line(x, y, h, amp.state.colors.named.panel_fg2);
//    amp.lcd[0].draw_v_line(x + 1, y + 1, h - 2, amp.state.colors.named.panel_fg2);
//    amp.lcd[0].draw_v_line(x + w, y, h, amp.state.colors.named.panel_fg1);
//    amp.lcd[0].draw_v_line(x + w - 1, y + 1, h - 2, amp.state.colors.named.panel_fg1);
//    amp.lcd[0].draw_h_line(x, y, w, amp.state.colors.named.panel_fg2);
//    amp.lcd[0].draw_h_line(x + 1, y + 1, w - 2, amp.state.colors.named.panel_fg2);
//    amp.lcd[0].draw_h_line(x, y + h, w, amp.state.colors.named.panel_fg1);
//    amp.lcd[0].draw_h_line(x + 1, y + h - 1, w - 2, amp.state.colors.named.panel_fg1);

    amp.lcd[0].draw_h_line(x, y + h, w, amp.state.colors.named.button_bg);

}

void draw_panel(display_board &b, const int x, const int y, const int w, const int h) {
    if (w < 8 || h < 8) return;

    b.draw_v_line(x, y, h, amp.state.colors.named.panel_fg1);
//    b.draw_v_line(x + 1, y + 1, h - 2, amp.state.colors.named.panel_fg1);
//    b.draw_v_line(x + w, y, h, amp.state.colors.named.panel_fg2);
//    b.draw_v_line(x + w - 1, y + 1, h - 2, amp.state.colors.named.panel_fg2);
//    b.draw_h_line(x, y, w, amp.state.colors.named.panel_fg1);
//    b.draw_h_line(x + 1, y + 1, w - 2, amp.state.colors.named.panel_fg1);
    b.draw_h_line(x, y + h, w, amp.state.colors.named.panel_fg1);
//    b.draw_h_line(x + 1, y + h - 1, w - 2, amp.state.colors.named.panel_fg2);

    b.fill_rect(x + 2, y + 2, w - 3, h - 3, amp.state.colors.named.panel_bg);
}

void draw_tx_sensor(const uint16_t pcolor) {
    if (amp.state.is_menu_active)
        return;

    amp.lcd[1].fill_rect(142, 10, 36, 16, pcolor);
}

void draw_ptt_mode() {
    if (amp.state.is_menu_active) return;

    amp.lcd[1].fill_rect(38, 21, 54, 21, amp.state.colors.named.panel_bg);

    switch (amp.state.mode) {
        case mode_type::standby:
            amp.lcd[1].draw_string("OFF", 36, 21, 3, amp.state.colors.named.alarm[2]);
            break;
        case mode_type::ptt:
            amp.lcd[1].draw_string("PTT", 36, 21, 3, amp.state.colors.named.alarm[1]);
            break;
        case mode_type::qrp:
            amp.lcd[1].draw_string("QRP", 36, 21, 3, amp.state.colors.named.alarm[1]);
            break;
    }
}

void draw_band(const byte band, const uint16_t color) {
    if (amp.state.is_menu_active) return;

    if (band == 0) amp.lcd[1].draw_string("UNK", 228, 21, 3, color);
    else if (band == 1) amp.lcd[1].draw_string("10M", 228, 21, 3, color);
    else if (band == 2) amp.lcd[1].draw_string("12M", 228, 21, 3, color);
    else if (band == 3) amp.lcd[1].draw_string("15M", 228, 21, 3, color);
    else if (band == 4) amp.lcd[1].draw_string("17M", 228, 21, 3, color);
    else if (band == 5) amp.lcd[1].draw_string("20M", 228, 21, 3, color);
    else if (band == 6) amp.lcd[1].draw_string("30M", 228, 21, 3, color);
    else if (band == 7) amp.lcd[1].draw_string("40M", 228, 21, 3, color);
    else if (band == 8) amp.lcd[1].draw_string("60M", 228, 21, 3, color);
    else if (band == 9) amp.lcd[1].draw_string("80M", 228, 21, 3, color);
    else if (band == 10) amp.lcd[1].draw_string("160M", 228, 21, 3, color);
}

void draw_ant() {
    if (amp.state.is_menu_active) return;

    amp.lcd[1].fill_rect(43, 142, 16, 21, amp.state.colors.named.panel_bg);

    if (amp.state.antForBand[amp.state.band] == 1) {
        amp.lcd[1].draw_string("1", 41, 142, 3, amp.state.colors.named.text_ant_atu);
        Serial3.println("*N1");
        SEL_ANT1;
    }

    if (amp.state.antForBand[amp.state.band] == 2) {
        amp.lcd[1].draw_string("2", 41, 142, 3, amp.state.colors.named.text_ant_atu);
        Serial3.println("*N2");
        SEL_ANT2;
    }

    if (amp.atu.is_present()) {
        amp.lcd[1].fill_rect(121, 142, 74, 21, amp.state.colors.named.text_ant_atu);
    }
}

void draw_atu() {
    if (amp.state.is_menu_active) return;

    amp.lcd[1].fill_rect(244, 142, 54, 21, amp.state.colors.named.panel_bg);

    if (amp.atu.is_present()) {
        if (amp.atu.is_active()) {
            amp.lcd[1].draw_string("ON", 244, 142, 3, amp.state.colors.named.text_ant_atu);
            Serial3.println("*Y0");
        } else {
            amp.lcd[1].draw_string("BYP", 244, 142, 3, amp.state.colors.named.text_ant_atu);
            Serial3.println("*Y1");
        }

        EEPROM.write(eeatub, amp.atu.is_active());
    } else {
        amp.lcd[1].draw_string("---", 244, 142, 3, amp.state.colors.named.text_ant_atu);
    }
}
