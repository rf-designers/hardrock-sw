#include "display_board.h"

extern const uint8_t c_chFont1206[95][12];
extern const uint8_t c_chFont1608[95][16];
extern const uint8_t simpleFont[96][8];

#define FONT_SPACE 6
#define FONT_X 8
#define FONT_Y 8


void display_board::draw_h_line(uint16_t hwXpos, uint16_t hwYpos, uint16_t hwWidth, uint16_t hwColor) {
    uint16_t i, x1 = min(hwXpos + hwWidth, lcd->width - 1);

    if (hwXpos >= lcd->width || hwYpos >= lcd->height) {
        return;
    }

    for (i = hwXpos; i < x1; i++) {
        draw_point(i, hwYpos, hwColor);
    }
}

void display_board::draw_v_line(uint16_t x, uint16_t y, uint16_t length, uint16_t color) {
    uint16_t i, y1 = min(y + length, lcd->height - 1);
    if (x >= lcd->width || y >= lcd->height) {
        return;
    }

    for (i = y; i < y1; i++) {
        draw_point(x, i, color);
    }

}

void display_board::draw_point(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= 321 || y >= 241) {
        return;
    }

    lcd->set_cursor(240 - y, x);
    lcd->write_byte(0x22, write_type::command);
    lcd->write_word(color);
}

void display_board::fill_rect(uint16_t x,
                              uint16_t y,
                              uint16_t w,
                              uint16_t h,
                              uint16_t color) {
    uint8_t xhi = 255, xnow, yhi = 255, ynow;

    if (x >= lcd->width || y >= lcd->height) {
        return;
    }

    for (uint16_t current_y = 240 - y; current_y >= (240 - y - h); current_y--) {
        // Don't write high byte if unchanged
        ynow = current_y >> 8;
        if (ynow != yhi) {
            lcd->write_register(0x02, ynow); //Column Start
            yhi = ynow;
        }
        lcd->write_register(0x03, current_y & 0xFF); //Column Start

        for (uint16_t current_x = x; current_x < x + w; current_x++) {
            /* Don't write high byte if unchanged */
            xnow = current_x >> 8;
            if (xnow != xhi) {
                lcd->write_register(0x06, xnow);
                xhi = xnow;
            }
            lcd->write_register(0x07, current_x & 0xFF); //Row Start

            lcd->write_byte(0x22, write_type::command);
            lcd->write_word(color);
        }
    }
}


void display_board::draw_string(const char *text, uint16_t x, uint16_t y, uint16_t size, uint16_t color) {
    while (*text) {
        draw_char(*text, x, y, size, color);
        text++;

        if (x < MAX_X) {
            x += FONT_SPACE * size;
        }
    }
}

void display_board::draw_char(uint8_t ascii, uint16_t x, uint16_t y, uint16_t size, uint16_t color) {
    if ((ascii < 32) || (ascii > 127))
        ascii = '?';

    for (int i = 0; i < FONT_X; i++) {
        uint8_t temp = pgm_read_byte(&simpleFont[ascii - 0x20][i]);
        for (uint8_t f = 0; f < 8; f++) {
            if ((temp >> f) & 0x01) {
                fill_rect(x + i * size,
                          y + f * size,
                          size,
                          size,
                          color);
            }
        }
    }
}

void display_board::display_char(uint16_t x, uint16_t y, uint8_t chr, uint8_t chSize, uint16_t hwColor) {
    uint8_t i, j, chTemp;
    uint16_t hwYpos0 = y, hwColorVal = 0;

    if (x >= lcd->width || y >= lcd->height) {
        return;
    }

    for (i = 0; i < chSize; i++) {
        if (FONT_1206 == chSize) {
            chTemp = pgm_read_byte(&c_chFont1206[chr - 0x20][i]);
        } else if (FONT_1608 == chSize) {
            chTemp = pgm_read_byte(&c_chFont1608[chr - 0x20][i]);
        }

        for (j = 0; j < 8; j++) {
            if (chTemp & 0x80) {
                hwColorVal = hwColor;
                draw_point(x, y, hwColorVal);
            }
            chTemp <<= 1;
            y++;
            if ((y - hwYpos0) == chSize) {
                y = hwYpos0;
                x++;
                break;
            }
        }
    }
}

void display_board::display_string(uint16_t x, uint16_t y,
                                   const uint8_t *text,
                                   uint8_t size,
                                   uint16_t color) {
    if (x >= 319 || y >= 239) {
        return;
    }

    while (*text != '\0') {
        if (x > (319U - size / 2)) {
            x = 0;
            x += size;
            if (y > (239U - size)) {
                y = x = 0;
                //lcd_clear_screen(0x00);
            }
        }

        display_char(x, y, *text, size, color);
        x += size / 2;
        text++;
    }
}

void display_board::clear_screen(uint16_t color) {
    lcd->clear_screen(color);
}

void display_board::lcd_reset() {
    lcd->reset();
}
