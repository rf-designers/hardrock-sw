#include "amp_state.h"

#include <HR500V1.h>

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
