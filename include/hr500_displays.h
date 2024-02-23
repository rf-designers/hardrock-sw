#pragma once

#include <Arduino.h>

void drawHome();
void DrawMenu();
void drawMeter();
void DrawRxButtons(uint16_t bcolor);
void DrawButton(int x, int y, int w, int h);
void DrawButtonDn(int button);
void DrawButtonUp(int button);
void DrawPanel(int x, int y, int w, int h);
void DrawTxPanel(uint16_t pcolor);
void DrawMode();
void DrawBand(byte band, uint16_t color);
void DrawAnt();
void DrawATU();