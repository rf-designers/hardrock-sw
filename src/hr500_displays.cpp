#include "hr500_displays.h"

#include <amp_state.h>

#include "HR500.h"
#include "HR500V1.h"
#include <EEPROM.h>

extern TFT Tft;
extern byte menu_choice;
extern byte menuSEL;
extern amp_state state;

void drawHome() {
    Tft.LCD_SEL = 1;
    DrawButton(5, 65, 118, 50);
    DrawButton(197, 65, 58, 50);
    DrawButton(257, 65, 58, 50);
    DrawButton(5, 185, 88, 50);
    DrawButton(227, 185, 88, 50);

    if (state.isAtuPresent == 0) {
        DrawButton(130, 135, 60, 30);
    } else {
        DrawButton(130, 85, 60, 30);
        DrawButton(116, 185, 88, 50);
        DrawPanel(116, 128, 88, 50);
    }

    DrawPanel(5, 8, 118, 50);
    DrawPanel(197, 8, 118, 50);
    DrawPanel(5, 128, 88, 50);
    DrawPanel(227, 128, 88, 50);
    DrawPanel(140, 8, 40, 20);
    DrawTxPanel(GREEN);
    DrawRxButtons(GBLUE);
    DrawMode();

    if (state.band != 0)
        DrawBand(state.band, ORANGE);
    else
        DrawBand(state.band, RED);

    DrawAnt();
    DrawATU();
}

void DrawMenu() {
    Tft.LCD_SEL = 1;
    DrawButton(130, 205, 60, 30);
    Tft.drawString((char *) "EXIT", 135, 213, 2, GBLUE);
    DrawButton(14, 8, 40, 44);
    Tft.drawString((char *) "<", 24, 18, 4, GBLUE);
    DrawButton(266, 8, 40, 44);
    Tft.drawString((char *) ">", 274, 18, 4, GBLUE);
    DrawPanel(60, 8, 200, 44);
    DrawPanel(60, 68, 200, 44);
    Tft.drawString((char *) menu_items[menu_choice], 65, 20, 2, WHITE);
    Tft.drawString((char *) item_disp[menu_choice], 65, 80, 2, LGRAY);
    DrawButton(120, 125, 80, 30);
    Tft.drawString((char *) "SELECT", 124, 132, 2, GBLUE);
    menuSEL = 0;
    Tft.drawString((char *) "ATU:", 206, 190, 2, LGRAY);
    Tft.drawString(state.atuVersion, 254, 190, 2, LGRAY);
    Tft.drawString((char *) "FW:", 206, 213, 2, LGRAY);
    Tft.drawString((char *) VERSION, 244, 213, 2, LGRAY);
}

void drawMeter() {
    Tft.LCD_SEL = 0;
    Tft.drawString((char *) "FWD", 14, 8, 2, GBLUE);
    DrawPanel(18, 32, 29, 14);
    Tft.drawString((char *) "RFL", 78, 8, 2, GBLUE);
    DrawPanel(82, 32, 29, 14);
    Tft.drawString((char *) "DRV", 142, 8, 2, GBLUE);
    DrawPanel(146, 32, 29, 14);
    Tft.drawString((char *) "VDD", 206, 8, 2, GBLUE);
    DrawPanel(210, 32, 29, 14);
    Tft.drawString((char *) "IDD", 270, 8, 2, GBLUE);
    DrawPanel(274, 32, 29, 14);
    DrawPanel(7, 58, 304, 64);
    DrawButton(10, 141, 44, 30);
    Tft.drawString((char *) "FWD", 13, 150, 2, GBLUE);
    DrawButton(74, 141, 44, 30);
    Tft.drawString((char *) "RFL", 77, 150, 2, GBLUE);
    DrawButton(138, 141, 44, 30);
    Tft.drawString((char *) "DRV", 141, 150, 2, GBLUE);
    DrawButton(202, 141, 44, 30);
    Tft.drawString((char *) "VDD", 205, 150, 2, GBLUE);
    DrawButton(266, 141, 44, 30);
    Tft.drawString((char *) "IDD", 269, 150, 2, GBLUE);
    DrawButtonDn(state.meterSelection);
    Tft.drawString((char *) "SWR:", 10, 205, 2, GBLUE);
    DrawPanel(63, 196, 80, 30);
    Tft.drawString((char *) "TEMP:", 165, 205, 2, GBLUE);
    DrawPanel(230, 196, 80, 30);
}

void DrawRxButtons(uint16_t bcolor) {
    if (state.isMenuActive) return;

    Tft.LCD_SEL = 1;
    Tft.drawString((char *) "MODE", 26, 79, 3, bcolor);
    Tft.drawString((char *) "<", 219, 79, 3, bcolor);
    Tft.drawString((char *) ">", 279, 79, 3, bcolor);
    Tft.drawString((char *) "ANT", 21, 199, 3, bcolor);

    if (state.isAtuPresent == 0) {
        Tft.drawString((char *) "MENU", 135, 143, 2, bcolor);
    } else {
        Tft.drawString((char *) "MENU", 135, 93, 2, bcolor);
        Tft.drawString((char *) "TUNA", 122, 199, 3, GBLUE);
        //Tft.drawString((char*)"----", 122, 142,  3, LBLUE);
    }

    if (state.isAtuPresent == 0) bcolor = DGRAY;

    Tft.drawString((char *) "ATU", 244, 199, 3, bcolor);
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

void DrawButtonUp(int button) {
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

void DrawPanel(int x, int y, int w, int h) {
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

void DrawTxPanel(uint16_t pcolor) {
    if (state.isMenuActive) return;

    Tft.LCD_SEL = 1;
    Tft.lcd_fill_rect(142, 10, 36, 16, pcolor);
}

void DrawMode() {
    if (state.isMenuActive) return;

    Tft.LCD_SEL = 1;
    Tft.lcd_fill_rect(38, 21, 54, 21, MGRAY);

    switch (state.mode) {
        case mode_type::standby:
            Tft.drawString((char *) "OFF", 36, 21, 3, YELLOW);
            break;
        case mode_type::ptt:
            Tft.drawString((char *) "PTT", 36, 21, 3, GREEN);
            break;
        case mode_type::qrp:
            Tft.drawString((char *) "QRP", 36, 21, 3, GREEN);
            break;
    }
}

void DrawBand(byte Band, uint16_t bcolor) {
    if (state.isMenuActive) return;

    if (Band == 0) Tft.drawString((char *) "UNK", 228, 21, 3, bcolor);
    else if (Band == 1) Tft.drawString((char *) "10M", 228, 21, 3, bcolor);
    else if (Band == 2) Tft.drawString((char *) "12M", 228, 21, 3, bcolor);
    else if (Band == 3) Tft.drawString((char *) "15M", 228, 21, 3, bcolor);
    else if (Band == 4) Tft.drawString((char *) "17M", 228, 21, 3, bcolor);
    else if (Band == 5) Tft.drawString((char *) "20M", 228, 21, 3, bcolor);
    else if (Band == 6) Tft.drawString((char *) "30M", 228, 21, 3, bcolor);
    else if (Band == 7) Tft.drawString((char *) "40M", 228, 21, 3, bcolor);
    else if (Band == 8) Tft.drawString((char *) "60M", 228, 21, 3, bcolor);
    else if (Band == 9) Tft.drawString((char *) "80M", 228, 21, 3, bcolor);
    else if (Band == 10) Tft.drawString((char *) "160M", 228, 21, 3, bcolor);
    else return;
}

void DrawAnt() {
    if (state.isMenuActive != 0) return;

    Tft.LCD_SEL = 1;
    Tft.lcd_fill_rect(43, 142, 16, 21, MGRAY);

    if (state.antForBand[state.band] == 1) {
        Tft.drawString((char *) "1", 41, 142, 3, LBLUE);
        Serial3.println("*N1");
        SEL_ANT1;
    }

    if (state.antForBand[state.band] == 2) {
        Tft.drawString((char *) "2", 41, 142, 3, LBLUE);
        Serial3.println("*N2");
        SEL_ANT2;
    }

    if (state.isAtuPresent) {
        Tft.LCD_SEL = 1;
        Tft.lcd_fill_rect(121, 142, 74, 21, MGRAY);
    }
}

void DrawATU() {
    if (state.isMenuActive) return;

    Tft.LCD_SEL = 1;
    Tft.lcd_fill_rect(244, 142, 54, 21, MGRAY);

    if (!state.isAtuPresent) {
        Tft.drawString((char *) "---", 244, 142, 3, LBLUE);
    } else {
        if (!state.isAtuActive) {
            Tft.drawString((char *) "BYP", 244, 142, 3, LBLUE);
            Serial3.println("*Y1");
        }

        if (state.isAtuActive) {
            Tft.drawString((char *) "ON", 244, 142, 3, LBLUE);
            Serial3.println("*Y0");
        }

        EEPROM.write(eeatub, state.isAtuActive);
    }
}
