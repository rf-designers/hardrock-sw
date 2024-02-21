#include "amp_state.h"

#include <atu_functions.h>
#include <EEPROM.h>
#include <FreqCount.h>
#include <HR500.h>
#include <HR500V1.h>
#include <hr500_displays.h>
#include <menu_functions.h>
#include <Wire.h>
#include <SPI.h>

mode_type nextMode(mode_type mode) {
    auto md = modeToEEPROM(mode) + 1;
    if (md == 2) {
        md = 0;
    }
    return modeFromEEPROM(md);
}

mode_type modeFromEEPROM(uint8_t mode) {
    mode = mode > 1 ? 0 : mode;
    return static_cast<mode_type>(mode);
}

uint8_t modeToEEPROM(mode_type mode) {
    return static_cast<uint8_t>(mode);
}

serial_speed nextSerialSpeed(serial_speed speed) {
    auto spd = speedToEEPROM(speed);
    spd++;
    if (spd > speedToEEPROM(serial_speed::baud_115200)) {
        spd = 0;
    }
    return speedFromEEPROM(spd);
}

serial_speed previousSerialSpeed(serial_speed speed) {
    auto spd = speedToEEPROM(speed);
    spd--;
    if (spd == 255) {
        spd = speedToEEPROM(serial_speed::baud_115200);
    }
    return speedFromEEPROM(spd);
}

serial_speed speedFromEEPROM(uint8_t speed) {
    speed = speed > speedToEEPROM(serial_speed::baud_115200) ? speedToEEPROM(serial_speed::baud_115200) : speed;
    return static_cast<serial_speed>(speed);
}

uint8_t speedToEEPROM(serial_speed speed) {
    return static_cast<uint8_t>(speed);
}

void PrepareForFWUpdate() {
    Serial.end();
    delay(50);
    Serial.begin(115200);

    while (!Serial.available());

    delay(50);
    digitalWrite(RST_OUT, LOW);
}

void amplifier::setup() {
    SETUP_RELAY_CS
    RELAY_CS_HIGH
    SETUP_LCD1_CS
    LCD1_CS_HIGH
    SETUP_LCD2_CS
    LCD2_CS_HIGH
    SETUP_SD1_CS
    SD1_CS_HIGH
    SETUP_SD2_CS
    SD2_CS_HIGH
    SETUP_TP1_CS
    TP1_CS_HIGH
    SETUP_TP2_CS
    TP2_CS_HIGH
    SETUP_LCD_BL
    analogWrite(LCD_BL, 255);
    SETUP_BYP_RLY
    RF_BYPASS
    SETUP_FAN1
    SETUP_FAN2
    SETUP_F_COUNT
    SETUP_ANT_RLY
    SEL_ANT1
    SETUP_COR
    SETUP_PTT
    SETUP_TRIP_FWD
    SETUP_TRIP_RFL
    SETUP_RESET
    RESET_PULSE
    SETUP_BIAS
    BIAS_OFF
    SETUP_TTL_PU
    S_POL_SETUP
    SETUP_ATU_TUNE
    ATU_TUNE_HIGH
    SETUP_ATTN_INST
    SETUP_ATTN_ON
    ATTN_ON_LOW

    pinMode(RST_OUT, OUTPUT);
    digitalWrite(RST_OUT, HIGH);
}

void amplifier::update() {
}

void amplifier::tripClear() {
    Wire.beginTransmission(LTCADDR); //clear any existing faults
    Wire.write(0x04);
    Wire.endTransmission(false);
    Wire.requestFrom(LTCADDR, 2, true);
    delay(1);

    Wire.read();
    delay(10);

    Wire.beginTransmission(LTCADDR); //set alert register
    Wire.write(0x01);
    Wire.write(0x02);
    Wire.endTransmission();
    Wire.requestFrom(LTCADDR, 2, true);
    delay(1);
}

void amplifier::tripSet() {
    Wire.beginTransmission(LTCADDR); //establish a fault condition
    Wire.write(0x03);
    Wire.write(0x02);
    Wire.endTransmission();
    Wire.requestFrom(LTCADDR, 2, true);
    delay(1);

    BIAS_OFF
    SendLPFRelayData(state.lpfBoardSerialData);
    RF_BYPASS
}

void amplifier::SendLPFRelayData(const byte data) {
    RELAY_CS_LOW
    SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE3));
    SPI.transfer(data);
    SPI.endTransaction();
    RELAY_CS_HIGH
}

void amplifier::SendLPFRelayDataSafe(byte data) {
    noInterrupts();
    SendLPFRelayData(data);
    interrupts();
}

// Reads input frequency and sets state.band accordingly
void amplifier::readInputFrequency() {
    int cnt = 0;
    byte same_cnt = 0;
    byte last_band = 0;
    FreqCountClass::begin(1);

    while (cnt < 12 && digitalRead(COR_DET)) {
        while (!FreqCountClass::available());

        const unsigned long frequency = FreqCountClass::read() * 16;
        byte band_read = 0;

        if (frequency > 1750 && frequency < 2100) band_read = 10;
        else if (frequency > 3000 && frequency < 4500) band_read = 9;
        else if (frequency > 4900 && frequency < 5700) band_read = 8;
        else if (frequency > 6800 && frequency < 7500) band_read = 7;
        else if (frequency > 9800 && frequency < 11000) band_read = 6;
        else if (frequency > 13500 && frequency < 15000) band_read = 5;
        else if (frequency > 17000 && frequency < 19000) band_read = 4;
        else if (frequency > 20500 && frequency < 22000) band_read = 3;
        else if (frequency > 23500 && frequency < 25400) band_read = 2;
        else if (frequency > 27500 && frequency < 30000) band_read = 1;

        if (band_read == last_band && last_band != 0) {
            same_cnt++;
        }

        if (same_cnt > 2) {
            state.band = last_band;
            cnt = 99;
        }

        last_band = band_read;
        cnt++;
    }

    FreqCountClass::end();
}


void amplifier::handleTouchScreen1() {
    if (ts1.touched()) {
        const byte pressedKey = getTouchedRectangle(1);
        switch (pressedKey) {
        case 10:
            state.meterSelection = 1;
            break;

        case 11:
            state.meterSelection = 2;
            break;

        case 12:
            state.meterSelection = 3;
            break;

        case 13:
            state.meterSelection = 4;
            break;

        case 14:
            state.meterSelection = 5;
            break;

        case 18:
        case 19:
            state.tempInCelsius = !state.tempInCelsius;
            EEPROM.write(eecelsius, state.tempInCelsius ? 1 : 0);
            break;
        }

        if (state.oldMeterSelection != state.meterSelection) {
            DrawButtonUp(state.oldMeterSelection);
            DrawButtonDn(state.meterSelection);
            state.oldMeterSelection = state.meterSelection;
            EEPROM.write(eemetsel, state.meterSelection);
        }

        while (ts1.touched());
    }
}

void amplifier::handleTouchScreen2() {
    if (ts2.touched() && state.timeToTouch == 0) {
        const byte pressedKey = getTouchedRectangle(2);
        state.timeToTouch = 300;

        if (state.isMenuActive) {
            switch (pressedKey) {
            case 0:
            case 1:
                if (state.menuSEL == 0) {
                    Tft.LCD_SEL = 1;
                    Tft.drawString((uint8_t*)menu_items[state.menu_choice], 65, 20, 2, MGRAY);
                    Tft.drawString((uint8_t*)item_disp[state.menu_choice], 65, 80, 2, MGRAY);

                    if (state.menu_choice-- == 0)
                        state.menu_choice = menu_max;

                    Tft.drawString((uint8_t*)menu_items[state.menu_choice], 65, 20, 2, WHITE);
                    Tft.drawString((uint8_t*)item_disp[state.menu_choice], 65, 80, 2, LGRAY);
                }

                break;

            case 3:
            case 4:
                if (state.menuSEL == 0) {
                    Tft.LCD_SEL = 1;
                    Tft.drawString((uint8_t*)menu_items[state.menu_choice], 65, 20, 2, MGRAY);
                    Tft.drawString((uint8_t*)item_disp[state.menu_choice], 65, 80, 2, MGRAY);

                    if (++state.menu_choice > menu_max)
                        state.menu_choice = 0;

                    Tft.drawString((uint8_t*)menu_items[state.menu_choice], 65, 20, 2, WHITE);
                    Tft.drawString((uint8_t*)item_disp[state.menu_choice], 65, 80, 2, LGRAY);
                }

                break;

            case 5:
            case 6:
                if (state.menuSEL == 1)
                    menuUpdate(state.menu_choice, 0);
                break;

            case 8:
            case 9:
                if (state.menuSEL == 1)
                    menuUpdate(state.menu_choice, 1);
                break;

            case 7:
            case 12:
                menuSelect();
                break;

            case 17:
                Tft.LCD_SEL = 1;
                Tft.lcd_clear_screen(GRAY);
                state.isMenuActive = false;

                if (state.menu_choice == mSETbias) {
                    BIAS_OFF
                    SendLPFRelayDataSafe(state.lpfBoardSerialData);
                    state.mode = state.old_mode;
                    state.MAX_CUR = 20;
                    state.Bias_Meter = 0;
                }

                drawHome();
                Tft.LCD_SEL = 0;
                Tft.lcd_reset();
            }
        } else {
            switch (pressedKey) {
            case 5:
            case 6:
                if (!state.txIsOn) {
                    state.mode = nextMode(state.mode);
                    EEPROM.write(eemode, modeToEEPROM(state.mode));
                    DrawMode();
                    disablePTTDetector();

                    if (state.mode == mode_type::ptt) {
                        enablePTTDetector();
                    }
                }
                break;

            case 8:
                if (!state.txIsOn) {
                    if (++state.band >= 11)
                        state.band = 1;
                    setBand();
                }
                break;

            case 9:
                if (!state.txIsOn) {
                    if (--state.band == 0)
                        state.band = 10;

                    if (state.band == 0xff)
                        state.band = 10;
                    setBand();
                }
                break;

            case 15:
            case 16:
                if (!state.txIsOn) {
                    if (++state.antForBand[state.band] == 3)
                        state.antForBand[state.band] = 1;

                    EEPROM.write(eeantsel + state.band, state.antForBand[state.band]);

                    if (state.antForBand[state.band] == 1) {
                        SEL_ANT1;
                    } else if (state.antForBand[state.band] == 2) {
                        SEL_ANT2;
                    }
                    DrawAnt();
                }
                break;

            case 18:
            case 19:
                if (state.isAtuPresent && !state.txIsOn) {
                    state.isAtuActive = !state.isAtuActive;
                    DrawATU();
                }
                break;

            case 12:
                if (!state.isAtuPresent) {
                    Tft.LCD_SEL = 1;
                    Tft.lcd_clear_screen(GRAY);
                    DrawMenu();
                    state.isMenuActive = true;
                }
                break;

            case 7:
                if (state.isAtuPresent) {
                    Tft.LCD_SEL = 1;
                    Tft.lcd_clear_screen(GRAY);
                    DrawMenu();
                    state.isMenuActive = true;
                }
                break;

            case 17:
                if (state.isAtuPresent) {
                    TuneButtonPressed();
                }
            }
        }

        while (ts2.touched());
    }
}

byte amplifier::getTouchedRectangle(byte touch_screen) {
    uint16_t x, y;
    uint8_t key;
    TS_Point p;

    if (touch_screen == 1)
        p = ts1.getPoint();
    if (touch_screen == 2)
        p = ts2.getPoint();

    x = map(p.x, 3850, 400, 0, 5);
    y = map(p.y, 350, 3700, 0, 4);
    //  z = p.z;
    key = x + 5 * y;
    return key;
}

// Enable interrupt on state change of D11 (PTT)
void amplifier::enablePTTDetector() {
    PCICR |= (1 << PCIE0);
    PCMSK0 |= (1 << PCINT5); // Set PCINT0 (digital input 11) to trigger an
    // interrupt on state change.
}

// Disable interrupt for state change of D11 (PTT)
void amplifier::disablePTTDetector() {
    PCMSK0 &= ~(1 << PCINT5);
}

void amplifier::setBand() {
    if (state.band > 10) return;
    if (state.txIsOn) return;

    byte lpfSerialData = 0;
    switch (state.band) {
    case 0:
        lpfSerialData = 0x00;
        ATUPrintln("*B0");
        break;
    case 1:
        lpfSerialData = 0x20;
        ATUPrintln("*B1");
        break;
    case 2:
        lpfSerialData = 0x20;
        ATUPrintln("*B2");
        break;
    case 3:
        lpfSerialData = 0x20;
        ATUPrintln("*B3");
        break;
    case 4:
        lpfSerialData = 0x08;
        ATUPrintln("*B4");
        break;
    case 5:
        lpfSerialData = 0x08;
        ATUPrintln("*B5");
        break;
    case 6:
        lpfSerialData = 0x04;
        ATUPrintln("*B6");
        break;
    case 7:
        lpfSerialData = 0x04;
        ATUPrintln("*B7");
        break;
    case 8:
        lpfSerialData = 0x02;
        ATUPrintln("*B8");
        break;
    case 9:
        lpfSerialData = 0x01;
        ATUPrintln("*B9");
        break;
    case 10:
        lpfSerialData = 0x00;
        ATUPrintln("*B10");
        break;
    }

    if (state.isAtuPresent && !state.isMenuActive) {
        Tft.LCD_SEL = 1;
        Tft.lcd_fill_rect(121, 142, 74, 21, MGRAY);
    }

    state.lpfBoardSerialData = lpfSerialData;
    if (state.band != state.oldBand) {
        SendLPFRelayData(state.lpfBoardSerialData);

        // delete old band (dirty rectangles)
        Tft.LCD_SEL = 1;
        DrawBand(state.oldBand, MGRAY);

        state.oldBand = state.band;

        // draw the new band
        const uint16_t dcolor = state.band == 0 ? RED : ORANGE;
        DrawBand(state.band, dcolor);

        EEPROM.write(eeband, state.band);
        DrawAnt();
    }
}
