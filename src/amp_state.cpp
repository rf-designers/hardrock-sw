#include "amp_state.h"

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
