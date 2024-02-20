#pragma once
#include <Arduino.h>

enum class mode_type : uint8_t {
    standby = 0,
    ptt = 1,
    qrp = 2
};

struct amp_state {
    volatile bool isTuning;
    volatile bool atuIsPresent;
    volatile bool txIsOn;
    mode_type mode = mode_type::standby; // 0 - OFF, 1 - PTT
    volatile bool atuActive;

    byte meterSelection = 0; // 1 - FWD; 2 - RFL; 3 - DRV; 4 - VDD; 5 - IDD
    volatile byte band = 6;
    byte trxType = 0;
    byte antForBand[11] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}; // antenna selection for each band
    bool menuDisplayed; // 0 is normal, 1 is menu mode
    bool tempInCelsius = true; // display temperature in Celsius?
    volatile byte lpfBoardSerialData = 0; // serial data to be sent to LPF
};

mode_type nextMode(mode_type mode);
mode_type fromEEPROM(uint8_t mode);
uint8_t toEEPROM(mode_type mode);
