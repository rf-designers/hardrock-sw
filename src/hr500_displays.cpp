#include "hr500_displays.h"

#include <amp_state.h>

#include "HR500.h"
#include "HR500V1.h"
#include <EEPROM.h>

extern TFT Tft;
extern amplifier amp;

void draw_home() {
    Tft.LCD_SEL = 1;
    DrawButton(5, 65, 118, 50);
    DrawButton(197, 65, 58, 50);
    DrawButton(257, 65, 58, 50);
    DrawButton(5, 185, 88, 50);
    DrawButton(227, 185, 88, 50);

    if (amp.atu.isPresent()) {
        DrawButton(130, 85, 60, 30);
        DrawButton(116, 185, 88, 50);
        draw_panel(116, 128, 88, 50);
    } else {
        DrawButton(130, 135, 60, 30);
    }

    draw_panel(5, 8, 118, 50);
    draw_panel(197, 8, 118, 50);
    draw_panel(5, 128, 88, 50);
    draw_panel(227, 128, 88, 50);
    draw_panel(140, 8, 40, 20);
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
    Tft.LCD_SEL = 1;
    DrawButton(130, 205, 60, 30);
    Tft.drawString("EXIT", 135, 213, 2, GBLUE);

    DrawButton(14, 8, 40, 44);
    Tft.drawString("<", 24, 18, 4, GBLUE);

    DrawButton(266, 8, 40, 44);
    Tft.drawString(">", 274, 18, 4, GBLUE);

    draw_panel(60, 8, 200, 44);
    draw_panel(60, 68, 200, 44);

    Tft.drawString(menu_items[amp.state.menuChoice], 65, 20, 2, WHITE);
    Tft.drawString(item_disp[amp.state.menuChoice], 65, 80, 2, LGRAY);
    DrawButton(120, 125, 80, 30);

    Tft.drawString("SELECT", 124, 132, 2, GBLUE);

    amp.state.menuSelected = false;

    Tft.drawString("ATU:", 206, 190, 2, LGRAY);
    Tft.drawString(amp.atu.getVersion(), 254, 190, 2, LGRAY);
    Tft.drawString("FW:", 206, 213, 2, LGRAY);
    Tft.drawString(VERSION, 244, 213, 2, LGRAY);
}

void draw_meter() {
    Tft.LCD_SEL = 0;
    Tft.drawString("FWD", 14, 8, 2, GBLUE);
    draw_panel(18, 32, 29, 14);

    Tft.drawString("RFL", 78, 8, 2, GBLUE);
    draw_panel(82, 32, 29, 14);

    Tft.drawString("DRV", 142, 8, 2, GBLUE);
    draw_panel(146, 32, 29, 14);

    Tft.drawString("VDD", 206, 8, 2, GBLUE);
    draw_panel(210, 32, 29, 14);

    Tft.drawString("IDD", 270, 8, 2, GBLUE);

    draw_panel(274, 32, 29, 14);
    draw_panel(7, 58, 304, 64);

    DrawButton(10, 141, 44, 30);
    Tft.drawString("FWD", 13, 150, 2, GBLUE);

    DrawButton(74, 141, 44, 30);
    Tft.drawString("RFL", 77, 150, 2, GBLUE);
    DrawButton(138, 141, 44, 30);
    Tft.drawString("DRV", 141, 150, 2, GBLUE);
    DrawButton(202, 141, 44, 30);
    Tft.drawString("VDD", 205, 150, 2, GBLUE);
    DrawButton(266, 141, 44, 30);
    Tft.drawString("IDD", 269, 150, 2, GBLUE);
    DrawButtonDn(amp.state.meterSelection);
    Tft.drawString("SWR:", 10, 205, 2, GBLUE);
    draw_panel(63, 196, 80, 30);
    Tft.drawString("TEMP:", 165, 205, 2, GBLUE);
    draw_panel(230, 196, 80, 30);
}

void draw_rx_buttons(uint16_t bcolor) {
    if (amp.state.isMenuActive) return;

    Tft.LCD_SEL = 1;
    Tft.drawString("MODE", 26, 79, 3, bcolor);
    Tft.drawString("<", 219, 79, 3, bcolor);
    Tft.drawString(">", 279, 79, 3, bcolor);
    Tft.drawString("ANT", 21, 199, 3, bcolor);

    if (amp.atu.isPresent()) {
        Tft.drawString("MENU", 135, 93, 2, bcolor);
        Tft.drawString("TUNE", 122, 199, 3, GBLUE);
        //Tft.drawString((char*)"----", 122, 142,  3, LBLUE);
    } else {
        Tft.drawString("MENU", 135, 143, 2, bcolor);
    }

    if (!amp.atu.isPresent()) bcolor = DGRAY;

    Tft.drawString("ATU", 244, 199, 3, bcolor);
}

void DrawButton(int x, int y, int w, int h) {
    if (h < 8 || w < 8) return;

    Tft.lcd_draw_v_line(x, y, h, LGRAY);
    Tft.lcd_draw_v_line(x + 1, y + 1, h - 2, LGRAY);
    Tft.lcd_draw_v_line(x + w, y, h, DGRAY);
    Tft.lcd_draw_v_line(x + w - 1, y + 1, h - 2, DGRAY);
    Tft.lcd_draw_h_line(x, y, w, LGRAY);
    Tft.lcd_draw_h_line(x + 1, y + 1, w - 2, LGRAY);
    Tft.lcd_draw_h_line(x, y + h, w, DGRAY);
    Tft.lcd_draw_h_line(x + 1, y + h - 1, w - 2, DGRAY);
    Tft.lcd_fill_rect(x + 2, y + 2, w - 4, h - 4, GRAY);
}

void DrawButtonDn(int button) {
    int x = (button - 1) * 64 + 10;
    int y = 141;
    int w = 44;
    int h = 30;
    int l_start = 83, l_height = 11, l_color = BLACK;

    Tft.LCD_SEL = 0;
    Tft.lcd_draw_v_line(x, y, h, DGRAY);
    Tft.lcd_draw_v_line(x + 1, y + 1, h - 2, DGRAY);
    Tft.lcd_draw_v_line(x + w, y, h, LGRAY);
    Tft.lcd_draw_v_line(x + w - 1, y + 1, h - 2, LGRAY);
    Tft.lcd_draw_h_line(x, y, w, DGRAY);
    Tft.lcd_draw_h_line(x + 1, y + 1, w - 2, DGRAY);
    Tft.lcd_draw_h_line(x, y + h, w, LGRAY);
    Tft.lcd_draw_h_line(x + 1, y + h - 1, w - 2, LGRAY);
    Tft.lcd_fill_rect(9, 60, 301, 34, 0xFFDC);

    if (button == 1 || button == 2 || button == 3) {
        for (int i = 19; i < 300; i += 56) {
            Tft.lcd_draw_v_line(i, l_start - 7, l_height + 7, l_color);
        }

        for (int i = 47; i < 300; i += 56) {
            Tft.lcd_draw_v_line(i, l_start, l_height, l_color);
        }

        if (button == 1) {
            Tft.lcd_display_string(16, 59, (uint8_t *) "0", FONT_1608, BLACK);
            Tft.lcd_display_string(30, 61, (uint8_t *) "WATTS", FONT_1206, BLACK);
            Tft.lcd_display_string(64, 59, (uint8_t *) "100", FONT_1608, BLACK);
            Tft.lcd_display_string(120, 59, (uint8_t *) "200", FONT_1608, BLACK);
            Tft.lcd_display_string(176, 59, (uint8_t *) "300", FONT_1608, BLACK);
            Tft.lcd_display_string(232, 59, (uint8_t *) "400", FONT_1608, BLACK);
            Tft.lcd_display_string(286, 59, (uint8_t *) "500", FONT_1608, BLACK);
        }

        if (button == 2) {
            Tft.lcd_display_string(16, 59, (uint8_t *) "0", FONT_1608, BLACK);
            Tft.lcd_display_string(30, 61, (uint8_t *) "WATTS", FONT_1206, BLACK);
            Tft.lcd_display_string(67, 59, (uint8_t *) "10", FONT_1608, BLACK);
            Tft.lcd_display_string(123, 59, (uint8_t *) "20", FONT_1608, BLACK);
            Tft.lcd_display_string(179, 59, (uint8_t *) "30", FONT_1608, BLACK);
            Tft.lcd_display_string(235, 59, (uint8_t *) "40", FONT_1608, BLACK);
            Tft.lcd_display_string(291, 59, (uint8_t *) "50", FONT_1608, BLACK);
        }

        if (button == 3) {
            Tft.lcd_display_string(16, 59, (uint8_t *) "0", FONT_1608, BLACK);
            Tft.lcd_display_string(30, 61, (uint8_t *) "WATTS", FONT_1206, BLACK);
            Tft.lcd_display_string(72, 59, (uint8_t *) "2", FONT_1608, BLACK);
            Tft.lcd_display_string(128, 59, (uint8_t *) "4", FONT_1608, BLACK);
            Tft.lcd_display_string(184, 59, (uint8_t *) "6", FONT_1608, BLACK);
            Tft.lcd_display_string(240, 59, (uint8_t *) "8", FONT_1608, BLACK);
            Tft.lcd_display_string(291, 59, (uint8_t *) "10", FONT_1608, BLACK);
        }
    }

    l_start = 79;
    l_height = 15;

    if (button == 4 || button == 5) {
        if (button == 4) {
            for (int i = 19; i < 300; i += 28) {
                Tft.lcd_draw_v_line(i, l_start, l_height, l_color);
            }

            l_start = 77;
            l_height = 12;
            Tft.lcd_display_string(16, 59, (uint8_t *) "0", FONT_1608, BLACK);
            Tft.lcd_display_string(30, 61, (uint8_t *) "VOLTS", FONT_1206, BLACK);
            Tft.lcd_display_string(67, 59, (uint8_t *) "12", FONT_1608, BLACK);
            Tft.lcd_display_string(123, 59, (uint8_t *) "24", FONT_1608, BLACK);
            Tft.lcd_display_string(179, 59, (uint8_t *) "36", FONT_1608, BLACK);
            Tft.lcd_display_string(235, 59, (uint8_t *) "48", FONT_1608, BLACK);
            Tft.lcd_display_string(291, 59, (uint8_t *) "60", FONT_1608, BLACK);
        }

        if (button == 5) {
            for (int i = 19; i < 300; i += 14) {
                Tft.lcd_draw_v_line(i, l_start, l_height, l_color);
            }

            Tft.lcd_display_string(16, 59, (uint8_t *) "0", FONT_1608, BLACK);
            Tft.lcd_display_string(40, 61, (uint8_t *) "AMPS", FONT_1206, BLACK);
            Tft.lcd_display_string(86, 59, (uint8_t *) "5", FONT_1608, BLACK);
            Tft.lcd_display_string(151, 59, (uint8_t *) "10", FONT_1608, BLACK);
            Tft.lcd_display_string(221, 59, (uint8_t *) "15", FONT_1608, BLACK);
            Tft.lcd_display_string(291, 59, (uint8_t *) "20", FONT_1608, BLACK);
        }
    }
}

void DrawButtonUp(const int button) {
    int x = (button - 1) * 64 + 10;
    int y = 141;
    int w = 44;
    int h = 30;

    Tft.LCD_SEL = 0;
    Tft.lcd_draw_v_line(x, y, h, LGRAY);
    Tft.lcd_draw_v_line(x + 1, y + 1, h - 2, LGRAY);
    Tft.lcd_draw_v_line(x + w, y, h, DGRAY);
    Tft.lcd_draw_v_line(x + w - 1, y + 1, h - 2, DGRAY);
    Tft.lcd_draw_h_line(x, y, w, LGRAY);
    Tft.lcd_draw_h_line(x + 1, y + 1, w - 2, LGRAY);
    Tft.lcd_draw_h_line(x, y + h, w, DGRAY);
    Tft.lcd_draw_h_line(x + 1, y + h - 1, w - 2, DGRAY);
}

void draw_panel(const int x, const int y, const int w, const int h) {
    if (w < 8 || h < 8) return;

    Tft.lcd_draw_v_line(x, y, h, DGRAY);
    Tft.lcd_draw_v_line(x + 1, y + 1, h - 2, DGRAY);
    Tft.lcd_draw_v_line(x + w, y, h, LGRAY);
    Tft.lcd_draw_v_line(x + w - 1, y + 1, h - 2, LGRAY);
    Tft.lcd_draw_h_line(x, y, w, DGRAY);
    Tft.lcd_draw_h_line(x + 1, y + 1, w - 2, DGRAY);
    Tft.lcd_draw_h_line(x, y + h, w, LGRAY);
    Tft.lcd_draw_h_line(x + 1, y + h - 1, w - 2, LGRAY);
    Tft.lcd_fill_rect(x + 2, y + 2, w - 3, h - 3, MGRAY);
}

void draw_tx_panel(const uint16_t pcolor) {
    if (amp.state.isMenuActive) return;

    Tft.LCD_SEL = 1;
    Tft.lcd_fill_rect(142, 10, 36, 16, pcolor);
}

void draw_mode() {
    if (amp.state.isMenuActive) return;

    Tft.LCD_SEL = 1;
    Tft.lcd_fill_rect(38, 21, 54, 21, MGRAY);

    switch (amp.state.mode) {
        case mode_type::standby:
            Tft.drawString("OFF", 36, 21, 3, YELLOW);
            break;
        case mode_type::ptt:
            Tft.drawString("PTT", 36, 21, 3, GREEN);
            break;
        case mode_type::qrp:
            Tft.drawString("QRP", 36, 21, 3, GREEN);
            break;
    }
}

void draw_band(const byte band, const uint16_t color) {
    if (amp.state.isMenuActive) return;

    if (band == 0) Tft.drawString("UNK", 228, 21, 3, color);
    else if (band == 1) Tft.drawString("10M", 228, 21, 3, color);
    else if (band == 2) Tft.drawString("12M", 228, 21, 3, color);
    else if (band == 3) Tft.drawString("15M", 228, 21, 3, color);
    else if (band == 4) Tft.drawString("17M", 228, 21, 3, color);
    else if (band == 5) Tft.drawString("20M", 228, 21, 3, color);
    else if (band == 6) Tft.drawString("30M", 228, 21, 3, color);
    else if (band == 7) Tft.drawString("40M", 228, 21, 3, color);
    else if (band == 8) Tft.drawString("60M", 228, 21, 3, color);
    else if (band == 9) Tft.drawString("80M", 228, 21, 3, color);
    else if (band == 10) Tft.drawString("160M", 228, 21, 3, color);
}

void draw_ant() {
    if (amp.state.isMenuActive) return;

    Tft.LCD_SEL = 1;
    Tft.lcd_fill_rect(43, 142, 16, 21, MGRAY);

    if (amp.state.antForBand[amp.state.band] == 1) {
        Tft.drawString("1", 41, 142, 3, LBLUE);
        Serial3.println("*N1");
        SEL_ANT1;
    }

    if (amp.state.antForBand[amp.state.band] == 2) {
        Tft.drawString("2", 41, 142, 3, LBLUE);
        Serial3.println("*N2");
        SEL_ANT2;
    }

    if (amp.atu.isPresent()) {
        Tft.LCD_SEL = 1;
        Tft.lcd_fill_rect(121, 142, 74, 21, MGRAY);
    }
}

void draw_atu() {
    if (amp.state.isMenuActive) return;

    Tft.LCD_SEL = 1;
    Tft.lcd_fill_rect(244, 142, 54, 21, MGRAY);

    if (amp.atu.isPresent()) {
        if (amp.atu.isActive()) {
            Tft.drawString("ON", 244, 142, 3, LBLUE);
            Serial3.println("*Y0");
        } else {
            Tft.drawString("BYP", 244, 142, 3, LBLUE);
            Serial3.println("*Y1");
        }

        EEPROM.write(eeatub, amp.atu.isActive());
    } else {
        Tft.drawString("---", 244, 142, 3, LBLUE);
    }
}
