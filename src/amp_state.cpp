#include "amp_state.h"

#include <HR500V1.h>

mode_type nextMode(mode_type mode) {
    auto md = toEEPROM(mode) + 1;
    if (md == 2) {
        md = 0;
    }
    return fromEEPROM(md);
}

mode_type fromEEPROM(uint8_t mode) {
    mode = mode > 1 ? 0 : mode;
    return static_cast<mode_type>(mode);
}

uint8_t toEEPROM(mode_type mode) {
    return static_cast<uint8_t>(mode);
}

void PrepareForFWUpdate() {
    Serial.end();
    delay(50);
    Serial.begin(115200);

    while (!Serial.available());

    delay(50);
    digitalWrite(RST_OUT, LOW);
}
