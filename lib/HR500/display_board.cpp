#include "display_board.h"
#include "HR500.h"

extern const uint8_t c_chFont1206[95][12];
extern const uint8_t c_chFont1608[95][16];
extern const uint8_t simpleFont[96][8];

#define FONT_SPACE 6
#define FONT_X 8
#define FONT_Y 8


void display_board::draw_h_line(const uint16_t x, const uint16_t y, const uint16_t width, const uint16_t color) const {
    lcd->fill_rect(x, y, width, 1, color);
}

auto display_board::draw_v_line(const uint16_t x, const uint16_t y, const uint16_t length,
                                const uint16_t color) const -> void {
    lcd->fill_rect(x, y, 1, length, color);
}

void display_board::draw_point(const uint16_t x, const uint16_t y, const uint16_t color) const {
    if (x >= lcd_comm::width || y >= lcd_comm::height) {
        return;
    }

    lcd->set_cursor(lcd_comm::height - y, x);
    lcd->write_byte(0x22, write_type::command);
    lcd->write_word(color);
}

void display_board::fill_rect(const uint16_t x, const uint16_t y,
                              const uint16_t w, const uint16_t h,
                              const uint16_t color) const {
    lcd->fill_rect(x, y, w, h, color);
}


void display_board::draw_string(const char* text, uint16_t x, const uint16_t y,
                                const uint16_t size, const uint16_t color) const {
    while (*text) {
        draw_char(*text, x, y, size, color);
        text++;

        x += FONT_SPACE * size;
        if (x >= lcd_comm::width) {
            break;
        }
    }
}

void display_board::draw_char(uint8_t ascii, uint16_t x, uint16_t y, uint16_t size, uint16_t color) const {
    if (ascii < 32 || ascii > 127)
        ascii = '?';

    for (int i = 0; i < FONT_X; i++) {
        uint8_t temp = pgm_read_byte(&simpleFont[ascii - 0x20][i]);
        for (uint8_t f = 0; f < 8; f++) {
            if (temp >> f & 0x01) {
                fill_rect(x + i * size,
                          y + f * size,
                          size,
                          size,
                          color);
            }
        }
    }
}

void display_board::display_string(const uint16_t x, const uint16_t y,
                                   const uint8_t* text,
                                   const uint8_t fnt,
                                   const uint16_t color) const {
    if (x >= lcd_comm::width || y >= lcd_comm::height) {
        return;
    }

    auto cx = x;
    auto cy = y;
    while (*text != '\0') {
        if (cx > ((lcd_comm::width - 1) - fnt / 2)) {
            cx = x;
            cx += fnt;
            if (cy > ((lcd_comm::height - 1) - fnt)) {
                cy = cx = 0;
                //lcd_clear_screen(0x00);
            }
        }

        display_char(cx, cy, *text, fnt, color);
        cx += fnt / 2;
        text++;
    }
}

void display_board::display_char(uint16_t x, uint16_t y, uint8_t chr, uint8_t fnt, uint16_t color) const {
    if (x >= lcd_comm::width || y >= lcd_comm::height) {
        return;
    }

    uint16_t start_y = y;

    // render the character by reading each byte
    // and drawing columns one by one
    for (uint8_t i = 0; i < fnt; i++) {
        uint8_t tmp_char;

        // TODO: find a better way around this in order to accommodate other fonts as well and reduce number of branches
        if (fnt == FONT_1206)
            tmp_char = pgm_read_byte(&c_chFont1206[chr - 0x20][i]);
        else
            tmp_char = pgm_read_byte(&c_chFont1608[chr - 0x20][i]);

        for (uint8_t j = 0; j < 8; j++) {
            if (tmp_char & 0x80) {
                draw_point(x, y, color);
            }
            tmp_char <<= 1;
            y++;
            if (y - start_y == fnt) {
                y = start_y;
                x++;
                break;
            }
        }
    }
}

void display_board::clear_screen(const uint16_t color) const {
    lcd->clear_screen(color);
}

void display_board::lcd_reset() const {
    lcd->reset();
}
