#include "display_board.h"

extern const uint8_t c_chFont1206[95][12];
extern const uint8_t c_chFont1608[95][16];
extern const uint8_t simpleFont[96][8];

#define FONT_SPACE 6
#define FONT_X 8
#define FONT_Y 8

void display_board::lcd_draw_h_line(uint16_t hwXpos, uint16_t hwYpos, uint16_t hwWidth, uint16_t hwColor) {
    uint16_t i, x1 = min(hwXpos + hwWidth, lcd->width - 1);

    if (hwXpos >= lcd->width || hwYpos >= lcd->height) {
        return;
    }

    for (i = hwXpos; i < x1; i++) {
        lcd_draw_point(i, hwYpos, hwColor);
    }
}

void display_board::draw_string(const char *string, uint16_t poX, uint16_t poY, uint16_t size, uint16_t fgcolor) {
    while (*string) {
        draw_char(*string, poX, poY, size, fgcolor);
        string++;

        if (poX < MAX_X) {
            poX += FONT_SPACE * size; /* Move cursor right            */
        }
    }
}

void display_board::lcd_draw_v_line(uint16_t hwXpos, uint16_t hwYpos, uint16_t hwHeight, uint16_t hwColor) {
    uint16_t i, y1 = min(hwYpos + hwHeight, lcd->height - 1);
    if (hwXpos >= lcd->width || hwYpos >= lcd->height) {
        return;
    }

    for (i = hwYpos; i < y1; i++) {
        lcd_draw_point(hwXpos, i, hwColor);
    }

}

void display_board::lcd_fill_rect(uint16_t hwXpos, uint16_t hwYpos, uint16_t hwWidth, uint16_t hwHeight,
                                  uint16_t hwColor) {
    uint16_t x, y;
    uint8_t xhi = 255, xnow, yhi = 255, ynow;

    if (hwXpos >= lcd->width || hwYpos >= lcd->height) {
        return;
    }

    for (y = 240 - hwYpos; y >= (240 - hwYpos - hwHeight); y--) {
        /* Don't write high byte if unchanged */
        ynow = y >> 8;
        if (ynow != yhi) {
            lcd->lcd_write_register(0x02, ynow); //Column Start
            yhi = ynow;
        }
        lcd->lcd_write_register(0x03, y & 0xFF); //Column Start

        for (x = hwXpos; x < hwXpos + hwWidth; x++) {
            /* Don't write high byte if unchanged */
            xnow = x >> 8;
            if (xnow != xhi) {
                lcd->lcd_write_register(0x06, xnow);
                xhi = xnow;
            }
            lcd->lcd_write_register(0x07, x & 0xFF); //Row Start

            lcd->lcd_write_byte(0x22, write_type::command);
            lcd->lcd_write_word(hwColor);
        }
    }
}

void display_board::draw_char(uint8_t ascii, uint16_t poX, uint16_t poY, uint16_t size, uint16_t fgcolor) {
    if ((ascii < 32) || (ascii > 127))
        ascii = '?';

    for (int i = 0; i < FONT_X; i++) {
        uint8_t temp = pgm_read_byte(&simpleFont[ascii - 0x20][i]);
        for (uint8_t f = 0; f < 8; f++) {
            if ((temp >> f) & 0x01) {
                lcd_fill_rect(poX + i * size, poY + f * size, size, size, fgcolor);
            }
        }
    }
}

void
display_board::lcd_display_char(uint16_t hwXpos, uint16_t hwYpos, uint8_t chChr, uint8_t chSize, uint16_t hwColor) {
    uint8_t i, j, chTemp;
    uint16_t hwYpos0 = hwYpos, hwColorVal = 0;

    if (hwXpos >= lcd->width || hwYpos >= lcd->height) {
        return;
    }

    for (i = 0; i < chSize; i++) {
        if (FONT_1206 == chSize) {
            chTemp = pgm_read_byte(&c_chFont1206[chChr - 0x20][i]);
        } else if (FONT_1608 == chSize) {
            chTemp = pgm_read_byte(&c_chFont1608[chChr - 0x20][i]);
        }

        for (j = 0; j < 8; j++) {
            if (chTemp & 0x80) {
                hwColorVal = hwColor;
                lcd_draw_point(hwXpos, hwYpos, hwColorVal);
            }
            chTemp <<= 1;
            hwYpos++;
            if ((hwYpos - hwYpos0) == chSize) {
                hwYpos = hwYpos0;
                hwXpos++;
                break;
            }
        }
    }
}

void display_board::lcd_draw_point(uint16_t hwXpos, uint16_t hwYpos, uint16_t hwColor) {
    if (hwXpos >= 321 || hwYpos >= 241) {
        return;
    }

    lcd->lcd_set_cursor(240 - hwYpos, hwXpos);
    lcd->lcd_write_byte(0x22, write_type::command);
    lcd->lcd_write_word(hwColor);
}
