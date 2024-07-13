#pragma once

#include <Arduino.h>
#include "HR500.h"

struct colors {
    uint16_t bg;
    uint16_t panel_bg;
    uint16_t panel_fg1;
    uint16_t panel_fg2;
    uint16_t text_enabled;
    uint16_t text_disabled;
    uint16_t meter_bg;
    uint16_t meter_ticks;
    uint16_t meter_fg;
    uint16_t button_bg;
    uint16_t alarm[4];
    uint16_t menu_1;
    uint16_t menu_2;
    uint16_t menu_3;
    uint16_t text_ant_atu;
    uint16_t tx_sensor_tx;
    uint16_t tx_sensor_rx;
};


struct color_theme {
    union {
        uint16_t values[20];
        colors named;
    };

    static color_theme make_default();
    static color_theme make_theme(int which);
};
