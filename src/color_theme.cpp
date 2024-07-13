#include "color_theme.h"

color_theme color_theme::make_default() {
    color_theme set{};
    set.named.bg = 0x0016; // GRAY
    set.named.panel_bg = 0x038C; // GRAY
    set.named.panel_fg1 = 0x043C;
    set.named.panel_fg2 = 0x043C;
    set.named.text_enabled = GBLUE;
    set.named.text_disabled = 0x043C;
    set.named.meter_bg = 0xFFDC;
    set.named.meter_ticks = BLACK;
    set.named.meter_fg = 0x038C;
    set.named.alarm[0] = 0x043C;
    set.named.alarm[1] = GREEN;
    set.named.alarm[2] = YELLOW;
    set.named.alarm[3] = RED;
    set.named.menu_1 = 0x038C;
    set.named.menu_2 = WHITE;
    set.named.menu_3 = LGRAY;
    set.named.button_bg = GRAY;
    set.named.text_ant_atu = LBLUE;
    set.named.tx_sensor_rx = GREEN;
    set.named.tx_sensor_tx = RED;
    set.named.band_text = ORANGE;
    return set;
}

color_theme color_theme::make_classic() {
    color_theme set{};
    set.named.bg = GRAY; // GRAY
    set.named.panel_bg = MGRAY; // GRAY
    set.named.panel_fg1 = DGRAY;
    set.named.panel_fg2 = LGRAY;
    set.named.text_enabled = GBLUE;
    set.named.text_disabled = DGRAY;
    set.named.meter_bg = 0xFFDC;
    set.named.meter_ticks = BLACK;
    set.named.meter_fg = MGRAY;
    set.named.alarm[0] = DGRAY;
    set.named.alarm[1] = GREEN;
    set.named.alarm[2] = YELLOW;
    set.named.alarm[3] = RED;
    set.named.menu_1 = MGRAY;
    set.named.menu_2 = WHITE;
    set.named.menu_3 = LGRAY;
    set.named.button_bg = GRAY;
    set.named.text_ant_atu = LBLUE;
    set.named.tx_sensor_rx = GREEN;
    set.named.tx_sensor_tx = RED;
    set.named.band_text = ORANGE;
    return set;
}
