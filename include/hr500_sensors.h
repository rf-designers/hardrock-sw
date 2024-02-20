#pragma once

#include "amp_state.h"

unsigned int ReadPower(power_type type);
unsigned int ReadVoltage(); //returns the voltage in 25mV steps
unsigned int ReadCurrent(); //returns the current in 5mA steps

void TripClear();
void TripSet();