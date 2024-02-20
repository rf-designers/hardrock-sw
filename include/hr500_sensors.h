#pragma once

#include <Arduino.h>

unsigned int ReadPower(byte pType);

unsigned int ReadVoltage(); //returns the voltage in 25mV steps
unsigned int ReadCurrent(); //returns the current in 5mA steps

void TripClear();
void TripSet();