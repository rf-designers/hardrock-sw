#pragma once

#include <Arduino.h>

void draw_home();
void draw_menu();
void draw_meter();
void draw_rx_buttons(uint16_t bcolor);
void DrawButton(int x, int y, int w, int h);
void DrawButtonDn(int button);
void DrawButtonUp(int button);
void draw_panel(int x, int y, int w, int h);
void draw_tx_panel(const uint16_t pcolor);
void draw_mode();
void draw_band(const byte band, const uint16_t color);
void draw_ant();
void draw_atu();