#pragma once
#include <stddef.h>

// Sends content to ATU and returns the number of received bytes
size_t ATUQuery(const char* command);
size_t ATUQuery(const char* command, char* response, size_t maxLength);

void DetectATU();
void TuneButtonPressed();
void TuneEnd();
