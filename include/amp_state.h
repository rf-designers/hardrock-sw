#pragma once
#include <Arduino.h>

struct amp_state {
    volatile bool isTuning;
    volatile bool atuIsPresent;
    volatile bool txIsOn;
    volatile byte mode = 0; // 0 - OFF, 1 - PTT
    volatile bool atuActive;

    byte meterSelection = 0; // 1 - FWD; 2 - RFL; 3 - DRV; 4 - VDD; 5 - IDD
    volatile byte band = 6;
    byte trxType = 0;
    byte antSelection[11] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}; // antenna selection for each band
    bool menuDisplayed; // 0 is normal, 1 is menu mode
    bool CELSIUS = true; // display temperature in Celsius?

};
