#pragma once

#include <Arduino.h>


struct colors
{
    uint8_t bg = GRAY;
    uint16_t panel_bg = MGRAY;
    uint16_t panel_fg1 = DGRAY;
    uint16_t panel_fg2 = LGRAY;
    uint16_t text_enabled;
    uint16_t text_disabled;
    uint16_t meter_bg = 0xFFDC;
    uint16_t meter_ticks = BLACK;
    uint16_t meter_fg = GRAY;
    uint16_t alarm[4] = {DGRAY, GREEN, YELLOW, RED};
    uint16_t menu_1 = MGRAY;
    uint16_t menu_2 = WHITE;
    uint16_t menu_3 = LGRAY;
};

struct color_theme
{
    union
    {
        uint16_t values[11];
        colors named;
    };
};
