#pragma once

#include <Arduino.h>

unsigned int Read_Power(byte pType);

unsigned int Read_Voltage(void); //returns the voltage in 25mV steps
unsigned int Read_Current(void); //returns the current in 5mA steps

void trip_clear(void);
void trip_set(void);