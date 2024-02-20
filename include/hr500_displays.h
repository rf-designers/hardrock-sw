#pragma once

#include <Arduino.h>

void DrawHome();
void DrawMenu();
void DrawMeter();
void DrawRxButtons(uint16_t bcolor);
void DrawButton(int x, int y, int w, int h);
void DrawButtonDn(int button);
void DrawButtonUp(int button);
void DrawPanel(int x, int y, int w, int h);
void DrawTxPanel(uint16_t pcolor);
void DrawMode();
void DrawBand(byte Band, uint16_t bcolor);
void DrawAnt();
void DrawATU();