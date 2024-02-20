#pragma once
#include <Arduino.h>

// Sends content to ATU and returns the number of received bytes
size_t ATUQuery(const char* command);

void TuneButtonPressed(void);
void TuneEnd(void);
