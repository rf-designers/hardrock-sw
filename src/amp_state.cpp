#include "amp_state.h"
#include <HR500V1.h>
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

