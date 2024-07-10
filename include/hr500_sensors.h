#pragma once

#include "amp_state.h"

unsigned int read_power(power_type powerType);
unsigned int read_voltage(); //returns the voltage in 25mV steps
unsigned int read_current(); //returns the current in 5mA steps