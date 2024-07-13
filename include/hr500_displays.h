#pragma once

#include <Arduino.h>
#include "display_board.h"

void draw_home();
void draw_menu();
void draw_meter();
void draw_rx_buttons(uint16_t color);
void draw_button(display_board &b, int x, int y, int w, int h);
void draw_button_down(int button);
void draw_button_up(int button);
void draw_panel(display_board &b, int x, int y, int w, int h);
void draw_tx_sensor(const uint16_t pcolor);
void draw_mode();
void draw_band(const byte band, const uint16_t color);
void draw_ant();
void draw_atu();