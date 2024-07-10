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
    // Serial.begin(115200);
    // while (!Serial.available());
    // delay(50);
    digitalWrite(RST_OUT, LOW);
}

void atu_board::detect() {
    Serial3.println(" ");
    strcpy(version, "---");

    char response[20] = {};
    if (query("*I", response, 20) > 10) {
        if (strncmp(response, "HR500 ATU", 9) == 0) {
            present = true;

            // memset(response, 0, 20);
            query("*V", response, 20);

            // ATU firmware currently responds with two lines for *I query only the first time (most likely bug)
            const auto versionSize = query("*V", response, 20);
            strncpy(version, response, versionSize);
        }
    }
}

bool atu_board::isPresent() const {
    return present;
}

const char* atu_board::getVersion() const {
    return version;
}

size_t atu_board::query(const char* command, char* response, size_t maxLength) {
    Serial3.setTimeout(100);
    Serial3.println(command);
    const size_t receivedBytes = Serial3.readBytesUntil(13, response, maxLength);
    last_response_size = receivedBytes;
    memcpy(last_response, response, receivedBytes);

    // look for a 13


    response[receivedBytes] = 0;
    return receivedBytes;
}

void atu_board::println(const char* command) {
    Serial3.println(command);
}

void atu_board::setTuning(bool enabled) {
    tuning = enabled;
}

void atu_board::setActive(bool a) {
    active = a;
}

bool atu_board::isTuning() const {
    return tuning;
}

bool atu_board::isActive() const {
    return active;
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
    lpf.sendRelayData(lpf.serialData);
    RF_BYPASS
}

void lpf_board::sendRelayData(const byte data) {
    RELAY_CS_LOW
    SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE3));
    SPI.transfer(data);
    SPI.endTransaction();
    RELAY_CS_HIGH
}

void lpf_board::sendRelayDataSafe(byte data) {
    noInterrupts();
    sendRelayData(data);
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
    // if (state.timeToTouch != 0) return;
    if (!ts2.touched()) return;

    const byte pressedKey = getTouchedRectangle(2);
    // state.timeToTouch = 300;

    if (state.isMenuActive) {
        switch (pressedKey) {
        case 0:
        case 1:
            if (!state.menuSelected) {
                Tft.LCD_SEL = 1;
                Tft.drawString(menu_items[state.menuChoice], 65, 20, 2, MGRAY);
                Tft.drawString(item_disp[state.menuChoice], 65, 80, 2, MGRAY);

                if (state.menuChoice-- == 0)
                    state.menuChoice = menu_max;

                Tft.drawString(menu_items[state.menuChoice], 65, 20, 2, WHITE);
                Tft.drawString(item_disp[state.menuChoice], 65, 80, 2, LGRAY);
            }
            break;

        case 3:
        case 4:
            if (!state.menuSelected) {
                Tft.LCD_SEL = 1;
                Tft.drawString(menu_items[state.menuChoice], 65, 20, 2, MGRAY);
                Tft.drawString(item_disp[state.menuChoice], 65, 80, 2, MGRAY);

                if (++state.menuChoice > menu_max)
                    state.menuChoice = 0;

                Tft.drawString(menu_items[state.menuChoice], 65, 20, 2, WHITE);
                Tft.drawString(item_disp[state.menuChoice], 65, 80, 2, LGRAY);
            }

            break;

        case 5:
        case 6:
            if (state.menuSelected) {
                menuUpdate(state.menuChoice, 0);
            }
            break;

        case 8:
        case 9:
            if (state.menuSelected) {
                menuUpdate(state.menuChoice, 1);
            }
            break;

        case 7:
        case 12:
            menuSelect();
            break;

        case 17:
            Tft.LCD_SEL = 1;
            Tft.lcd_clear_screen(GRAY);
            state.isMenuActive = false;

            if (state.menuChoice == mSETbias) {
                BIAS_OFF
                lpf.sendRelayDataSafe(lpf.serialData);
                state.mode = state.old_mode;
                state.MAX_CUR = 20;
                state.biasMeter = false;
            }

                draw_home();
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
                draw_mode();
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
                draw_ant();
            }
            break;

        case 18:
        case 19:
            if (atu.isPresent() && !state.txIsOn) {
                atu.setActive(!atu.isActive());
                draw_atu();
            }
            break;

        case 12:
            if (!atu.isPresent()) {
                Tft.LCD_SEL = 1;
                Tft.lcd_clear_screen(GRAY);
                draw_menu();
                state.isMenuActive = true;
            }
            break;

        case 7:
            if (atu.isPresent()) {
                Tft.LCD_SEL = 1;
                Tft.lcd_clear_screen(GRAY);
                draw_menu();
                state.isMenuActive = true;
            }
            break;

        case 17:
            if (atu.isPresent()) {
                TuneButtonPressed();
            }
        }
    }

    while (ts2.touched());
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
    noInterrupts();

    PCICR |= (1 << PCIE0);
    PCMSK0 |= (1 << PCINT5); // Set PCINT0 (digital input 11) to trigger an
    // interrupt on state change.

    interrupts();
}

// Disable interrupt for state change of D11 (PTT)
void amplifier::disablePTTDetector() {
    noInterrupts();

    PCMSK0 &= ~(1 << PCINT5);

    interrupts();
}

void amplifier::setBand() {
    if (state.band > 10) return;
    if (state.txIsOn) return;

    byte lpfSerialData = 0;
    switch (state.band) {
    case 0:
        lpfSerialData = 0x00;
        atu.println("*B0");
        break;
    case 1:
        lpfSerialData = 0x20;
        atu.println("*B1");
        break;
    case 2:
        lpfSerialData = 0x20;
        atu.println("*B2");
        break;
    case 3:
        lpfSerialData = 0x20;
        atu.println("*B3");
        break;
    case 4:
        lpfSerialData = 0x08;
        atu.println("*B4");
        break;
    case 5:
        lpfSerialData = 0x08;
        atu.println("*B5");
        break;
    case 6:
        lpfSerialData = 0x04;
        atu.println("*B6");
        break;
    case 7:
        lpfSerialData = 0x04;
        atu.println("*B7");
        break;
    case 8:
        lpfSerialData = 0x02;
        atu.println("*B8");
        break;
    case 9:
        lpfSerialData = 0x01;
        atu.println("*B9");
        break;
    case 10:
        lpfSerialData = 0x00;
        atu.println("*B10");
        break;
    }

    if (atu.isPresent() && !state.isMenuActive) {
        Tft.LCD_SEL = 1;
        Tft.lcd_fill_rect(121, 142, 74, 21, MGRAY);
    }

    lpf.serialData = lpfSerialData;
    if (state.band != state.oldBand) {
        lpf.sendRelayData(lpf.serialData);

        // delete old band (dirty rectangles)
        Tft.LCD_SEL = 1;
        draw_band(state.oldBand, MGRAY);

        state.oldBand = state.band;

        // draw the new band
        const uint16_t dcolor = state.band == 0 ? RED : ORANGE;
        draw_band(state.band, dcolor);

        EEPROM.write(eeband, state.band);
        draw_ant();
    }
}

void amplifier::switchToTX() {
    state.F_alert = 1;
    state.R_alert = 1;
    state.D_alert = 1;
    state.V_alert = 1;
    state.I_alert = 1;
    state.OF_alert = 0;
    state.OR_alert = 0;
    state.OD_alert = 0;
    state.OV_alert = 0;
    state.OI_alert = 0;

    delay(20);
    RESET_PULSE
    state.s_disp = 19;

    draw_rx_buttons(DGRAY);
    draw_tx_panel(RED);
}

void amplifier::switchToRX() {
    draw_rx_buttons(GBLUE);
    draw_tx_panel(GREEN);
    tripClear();
    RESET_PULSE
    Tft.LCD_SEL = 0;
    Tft.lcd_fill_rect(70, 203, 36, 16, MGRAY);
    Tft.drawString(state.RL_TXT, 70, 203, 2, LGRAY);
    strcpy(state.ORL_TXT, "   ");
}
