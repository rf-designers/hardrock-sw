#include "hr500_displays.h"

#include <amp_state.h>

#include "HR500.h"
#include "HR500V1.h"
#include "display_board.h"
#include <EEPROM.h>

extern amplifier amp;
extern display_board lcd[2];

void draw_home() {
    draw_button(lcd[1], 5, 65, 118, 50);
    draw_button(lcd[1], 197, 65, 58, 50);
    draw_button(lcd[1], 257, 65, 58, 50);
    draw_button(lcd[1], 5, 185, 88, 50);
    draw_button(lcd[1], 227, 185, 88, 50);

    if (amp.atu.isPresent()) {
        draw_button(lcd[1], 130, 85, 60, 30);
        draw_button(lcd[1], 116, 185, 88, 50);
        draw_panel(lcd[1], 116, 128, 88, 50);
    } else {
        draw_button(lcd[1], 130, 135, 60, 30);
    }

    draw_panel(lcd[1], 5, 8, 118, 50);
    draw_panel(lcd[1], 197, 8, 118, 50);
    draw_panel(lcd[1], 5, 128, 88, 50);
    draw_panel(lcd[1], 227, 128, 88, 50);
    draw_panel(lcd[1], 140, 8, 40, 20);
    draw_tx_panel(GREEN);
    draw_rx_buttons(GBLUE);
    draw_mode();

    if (amp.state.band != 0)
        draw_band(amp.state.band, ORANGE);
    else
        draw_band(amp.state.band, RED);

    draw_ant();
    draw_atu();
}

void draw_menu() {
    draw_button(lcd[1], 130, 205, 60, 30);
    lcd[1].draw_string("EXIT", 135, 213, 2, GBLUE);

    draw_button(lcd[1], 14, 8, 40, 44);
    lcd[1].draw_string("<", 24, 18, 4, GBLUE);

    draw_button(lcd[1], 266, 8, 40, 44);
    lcd[1].draw_string(">", 274, 18, 4, GBLUE);

    draw_panel(lcd[1], 60, 8, 200, 44);
    draw_panel(lcd[1], 60, 68, 200, 44);

    lcd[1].draw_string(menu_items[amp.state.menuChoice], 65, 20, 2, WHITE);
    lcd[1].draw_string(item_disp[amp.state.menuChoice], 65, 80, 2, LGRAY);
    draw_button(lcd[1], 120, 125, 80, 30);

    lcd[1].draw_string("SELECT", 124, 132, 2, GBLUE);

    amp.state.menuSelected = false;

    lcd[1].draw_string("ATU:", 206, 190, 2, LGRAY);
    lcd[1].draw_string(amp.atu.getVersion(), 254, 190, 2, LGRAY);
    lcd[1].draw_string("FW:", 206, 213, 2, LGRAY);
    lcd[1].draw_string(VERSION, 244, 213, 2, LGRAY);
}

void draw_meter() {
    lcd[0].draw_string("FWD", 14, 8, 2, GBLUE);
    draw_panel(lcd[0], 18, 32, 29, 14);

    lcd[0].draw_string("RFL", 78, 8, 2, GBLUE);
    draw_panel(lcd[0], 82, 32, 29, 14);

    lcd[0].draw_string("DRV", 142, 8, 2, GBLUE);
    draw_panel(lcd[0], 146, 32, 29, 14);

    lcd[0].draw_string("VDD", 206, 8, 2, GBLUE);
    draw_panel(lcd[0], 210, 32, 29, 14);

    lcd[0].draw_string("IDD", 270, 8, 2, GBLUE);
    draw_panel(lcd[0], 274, 32, 29, 14);

    draw_panel(lcd[0], 7, 58, 304, 64);

    draw_button(lcd[0], 10, 141, 44, 30);
    lcd[0].draw_string("FWD", 13, 150, 2, GBLUE);

    draw_button(lcd[0], 74, 141, 44, 30);
    lcd[0].draw_string("RFL", 77, 150, 2, GBLUE);

    draw_button(lcd[0], 138, 141, 44, 30);
    lcd[0].draw_string("DRV", 141, 150, 2, GBLUE);

    draw_button(lcd[0], 202, 141, 44, 30);
    lcd[0].draw_string("VDD", 205, 150, 2, GBLUE);

    draw_button(lcd[0], 266, 141, 44, 30);
    lcd[0].draw_string("IDD", 269, 150, 2, GBLUE);

    draw_button_down(amp.state.meterSelection);
    lcd[0].draw_string("SWR:", 10, 205, 2, GBLUE);

    draw_panel(lcd[0], 63, 196, 80, 30);
    lcd[0].draw_string("TEMP:", 165, 205, 2, GBLUE);
    draw_panel(lcd[0], 230, 196, 80, 30);
}

void draw_rx_buttons(uint16_t bcolor) {
    if (amp.state.isMenuActive) return;

    lcd[1].draw_string("MODE", 26, 79, 3, bcolor);
    lcd[1].draw_string("<", 219, 79, 3, bcolor);
    lcd[1].draw_string(">", 279, 79, 3, bcolor);
    lcd[1].draw_string("ANT", 21, 199, 3, bcolor);

    if (amp.atu.isPresent()) {
        lcd[1].draw_string("MENU", 135, 93, 2, bcolor);
        lcd[1].draw_string("TUNE", 122, 199, 3, GBLUE);
        //Tft.drawString((char*)"----", 122, 142,  3, LBLUE);
    } else {
        lcd[1].draw_string("MENU", 135, 143, 2, bcolor);
    }

    if (!amp.atu.isPresent()) bcolor = DGRAY;

    lcd[1].draw_string("ATU", 244, 199, 3, bcolor);
}

void draw_button(display_board& b, int x, int y, int w, int h) {
    if (h < 8 || w < 8) return;

    b.draw_v_line(x, y, h, LGRAY);
    b.draw_v_line(x + 1, y + 1, h - 2, LGRAY);
    b.draw_v_line(x + w, y, h, DGRAY);
    b.draw_v_line(x + w - 1, y + 1, h - 2, DGRAY);
    b.draw_h_line(x, y, w, LGRAY);
    b.draw_h_line(x + 1, y + 1, w - 2, LGRAY);
    b.draw_h_line(x, y + h, w, DGRAY);
    b.draw_h_line(x + 1, y + h - 1, w - 2, DGRAY);
    b.fill_rect(x + 2, y + 2, w - 4, h - 4, GRAY);
}

void draw_button_down(int button) {
    int x = (button - 1) * 64 + 10;
    int y = 141;
    int w = 44;
    int h = 30;
    int l_start = 83, l_height = 11, l_color = BLACK;

    lcd[0].draw_v_line(x, y, h, DGRAY);
    lcd[0].draw_v_line(x + 1, y + 1, h - 2, DGRAY);
    lcd[0].draw_v_line(x + w, y, h, LGRAY);
    lcd[0].draw_v_line(x + w - 1, y + 1, h - 2, LGRAY);
    lcd[0].draw_h_line(x, y, w, DGRAY);
    lcd[0].draw_h_line(x + 1, y + 1, w - 2, DGRAY);
    lcd[0].draw_h_line(x, y + h, w, LGRAY);
    lcd[0].draw_h_line(x + 1, y + h - 1, w - 2, LGRAY);
    lcd[0].fill_rect(9, 60, 301, 34, 0xFFDC);

    if (button == 1 || button == 2 || button == 3) {
        for (int i = 19; i < 300; i += 56) {
            lcd[0].draw_v_line(i, l_start - 7, l_height + 7, l_color);
        }

        for (int i = 47; i < 300; i += 56) {
            lcd[0].draw_v_line(i, l_start, l_height, l_color);
        }

        if (button == 1) {
            lcd[0].display_string(16, 59, (uint8_t *) "0", FONT_1608, BLACK);
            lcd[0].display_string(30, 61, (uint8_t *) "WATTS", FONT_1206, BLACK);
            lcd[0].display_string(64, 59, (uint8_t *) "100", FONT_1608, BLACK);
            lcd[0].display_string(120, 59, (uint8_t *) "200", FONT_1608, BLACK);
            lcd[0].display_string(176, 59, (uint8_t *) "300", FONT_1608, BLACK);
            lcd[0].display_string(232, 59, (uint8_t *) "400", FONT_1608, BLACK);
            lcd[0].display_string(286, 59, (uint8_t *) "500", FONT_1608, BLACK);
        }

        if (button == 2) {
            lcd[0].display_string(16, 59, (uint8_t *) "0", FONT_1608, BLACK);
            lcd[0].display_string(30, 61, (uint8_t *) "WATTS", FONT_1206, BLACK);
            lcd[0].display_string(67, 59, (uint8_t *) "10", FONT_1608, BLACK);
            lcd[0].display_string(123, 59, (uint8_t *) "20", FONT_1608, BLACK);
            lcd[0].display_string(179, 59, (uint8_t *) "30", FONT_1608, BLACK);
            lcd[0].display_string(235, 59, (uint8_t *) "40", FONT_1608, BLACK);
            lcd[0].display_string(291, 59, (uint8_t *) "50", FONT_1608, BLACK);
        }

        if (button == 3) {
            lcd[0].display_string(16, 59, (uint8_t *) "0", FONT_1608, BLACK);
            lcd[0].display_string(30, 61, (uint8_t *) "WATTS", FONT_1206, BLACK);
            lcd[0].display_string(72, 59, (uint8_t *) "2", FONT_1608, BLACK);
            lcd[0].display_string(128, 59, (uint8_t *) "4", FONT_1608, BLACK);
            lcd[0].display_string(184, 59, (uint8_t *) "6", FONT_1608, BLACK);
            lcd[0].display_string(240, 59, (uint8_t *) "8", FONT_1608, BLACK);
            lcd[0].display_string(291, 59, (uint8_t *) "10", FONT_1608, BLACK);
        }
    }

    l_start = 79;
    l_height = 15;

    if (button == 4 || button == 5) {
        if (button == 4) {
            for (int i = 19; i < 300; i += 28) {
                lcd[0].draw_v_line(i, l_start, l_height, l_color);
            }

            l_start = 77;
            l_height = 12;
            lcd[0].display_string(16, 59, (uint8_t *) "0", FONT_1608, BLACK);
            lcd[0].display_string(30, 61, (uint8_t *) "VOLTS", FONT_1206, BLACK);
            lcd[0].display_string(67, 59, (uint8_t *) "12", FONT_1608, BLACK);
            lcd[0].display_string(123, 59, (uint8_t *) "24", FONT_1608, BLACK);
            lcd[0].display_string(179, 59, (uint8_t *) "36", FONT_1608, BLACK);
            lcd[0].display_string(235, 59, (uint8_t *) "48", FONT_1608, BLACK);
            lcd[0].display_string(291, 59, (uint8_t *) "60", FONT_1608, BLACK);
        }

        if (button == 5) {
            for (int i = 19; i < 300; i += 14) {
                lcd[0].draw_v_line(i, l_start, l_height, l_color);
            }

            lcd[0].display_string(16, 59, (uint8_t *) "0", FONT_1608, BLACK);
            lcd[0].display_string(40, 61, (uint8_t *) "AMPS", FONT_1206, BLACK);
            lcd[0].display_string(86, 59, (uint8_t *) "5", FONT_1608, BLACK);
            lcd[0].display_string(151, 59, (uint8_t *) "10", FONT_1608, BLACK);
            lcd[0].display_string(221, 59, (uint8_t *) "15", FONT_1608, BLACK);
            lcd[0].display_string(291, 59, (uint8_t *) "20", FONT_1608, BLACK);
        }
    }
}

void draw_button_up(const int button) {
    int x = (button - 1) * 64 + 10;
    int y = 141;
    int w = 44;
    int h = 30;

    lcd[0].draw_v_line(x, y, h, LGRAY);
    lcd[0].draw_v_line(x + 1, y + 1, h - 2, LGRAY);
    lcd[0].draw_v_line(x + w, y, h, DGRAY);
    lcd[0].draw_v_line(x + w - 1, y + 1, h - 2, DGRAY);
    lcd[0].draw_h_line(x, y, w, LGRAY);
    lcd[0].draw_h_line(x + 1, y + 1, w - 2, LGRAY);
    lcd[0].draw_h_line(x, y + h, w, DGRAY);
    lcd[0].draw_h_line(x + 1, y + h - 1, w - 2, DGRAY);
}

void draw_panel(display_board &b, const int x, const int y, const int w, const int h) {
    if (w < 8 || h < 8) return;

    b.draw_v_line(x, y, h, DGRAY);
    b.draw_v_line(x + 1, y + 1, h - 2, DGRAY);
    b.draw_v_line(x + w, y, h, LGRAY);
    b.draw_v_line(x + w - 1, y + 1, h - 2, LGRAY);
    b.draw_h_line(x, y, w, DGRAY);
    b.draw_h_line(x + 1, y + 1, w - 2, DGRAY);
    b.draw_h_line(x, y + h, w, LGRAY);
    b.draw_h_line(x + 1, y + h - 1, w - 2, LGRAY);
    b.fill_rect(x + 2, y + 2, w - 3, h - 3, MGRAY);
}

void draw_tx_panel(const uint16_t pcolor) {
    if (amp.state.isMenuActive)
        return;

    lcd[1].fill_rect(142, 10, 36, 16, pcolor);
}

void draw_mode() {
    if (amp.state.isMenuActive) return;

    lcd[1].fill_rect(38, 21, 54, 21, MGRAY);

    switch (amp.state.mode) {
        case mode_type::standby:
            lcd[1].draw_string("OFF", 36, 21, 3, YELLOW);
            break;
        case mode_type::ptt:
            lcd[1].draw_string("PTT", 36, 21, 3, GREEN);
            break;
        case mode_type::qrp:
            lcd[1].draw_string("QRP", 36, 21, 3, GREEN);
            break;
    }
}

void draw_band(const byte band, const uint16_t color) {
    if (amp.state.isMenuActive) return;

    if (band == 0) lcd[1].draw_string("UNK", 228, 21, 3, color);
    else if (band == 1) lcd[1].draw_string("10M", 228, 21, 3, color);
    else if (band == 2) lcd[1].draw_string("12M", 228, 21, 3, color);
    else if (band == 3) lcd[1].draw_string("15M", 228, 21, 3, color);
    else if (band == 4) lcd[1].draw_string("17M", 228, 21, 3, color);
    else if (band == 5) lcd[1].draw_string("20M", 228, 21, 3, color);
    else if (band == 6) lcd[1].draw_string("30M", 228, 21, 3, color);
    else if (band == 7) lcd[1].draw_string("40M", 228, 21, 3, color);
    else if (band == 8) lcd[1].draw_string("60M", 228, 21, 3, color);
    else if (band == 9) lcd[1].draw_string("80M", 228, 21, 3, color);
    else if (band == 10) lcd[1].draw_string("160M", 228, 21, 3, color);
}

void draw_ant() {
    if (amp.state.isMenuActive) return;

    lcd[1].fill_rect(43, 142, 16, 21, MGRAY);

    if (amp.state.antForBand[amp.state.band] == 1) {
        lcd[1].draw_string("1", 41, 142, 3, LBLUE);
        Serial3.println("*N1");
        SEL_ANT1;
    }

    if (amp.state.antForBand[amp.state.band] == 2) {
        lcd[1].draw_string("2", 41, 142, 3, LBLUE);
        Serial3.println("*N2");
        SEL_ANT2;
    }

    if (amp.atu.isPresent()) {
        lcd[1].fill_rect(121, 142, 74, 21, MGRAY);
    }
}

void draw_atu() {
    if (amp.state.isMenuActive) return;

    lcd[1].fill_rect(244, 142, 54, 21, MGRAY);

    if (amp.atu.isPresent()) {
        if (amp.atu.isActive()) {
            lcd[1].draw_string("ON", 244, 142, 3, LBLUE);
            Serial3.println("*Y0");
        } else {
            lcd[1].draw_string("BYP", 244, 142, 3, LBLUE);
            Serial3.println("*Y1");
        }

        EEPROM.write(eeatub, amp.atu.isActive());
    } else {
        lcd[1].draw_string("---", 244, 142, 3, LBLUE);
    }
}
