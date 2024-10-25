#include "hr500_sensors.h"
#include "amp_state.h"
#include <Wire.h>
#include "HR500V1.h"

extern amplifier amp;

unsigned int read_power(power_type type) {
    long pCalc, tCalc, bCalc;
    long pResult{0};

    switch (type) {
        case power_type::fwd_p:
            if (amp.state.fwd_tot == 0) return 0;
            pCalc = long(amp.state.fwd_tot) + long(30);
            pResult = (pCalc * pCalc) / long(809);
            break;
        case power_type::rfl_p:
            if (amp.state.rfl_tot == 0) return 0;

            pCalc = long(amp.state.rfl_tot) + long(140);
            pResult = (pCalc * pCalc) / long(10716);
            break;
        case power_type::drv_p:
            if (amp.state.drv_tot == 0) return 0;

            pCalc = long(amp.state.drv_tot) + long(30);
            pResult = (pCalc * pCalc) / long(10860);
            break;
        case power_type::vswr:
            if (amp.state.fwd_tot < 205) return 0;
            if (amp.state.rfl_tot < 33) return 10;

            tCalc = long(100) * long(amp.state.fwd_tot) + long(30) * (amp.state.rfl_tot) + long(2800);
            bCalc = long(10) * long(amp.state.fwd_tot) - long(3) * long(amp.state.rfl_tot) - long(280);
            pResult = tCalc / bCalc;

            if (pResult < 10) pResult = 10;
            if (pResult > 99) pResult = 99;
            break;
    }

    return pResult;
}


// returns the voltage in 25mV steps
unsigned int read_voltage() {
    Wire.beginTransmission(LTCADDR);
    Wire.write(0x1e);
    Wire.endTransmission(false);
    Wire.requestFrom(LTCADDR, 2, true);

    delay(1);

    const byte msb = Wire.read();
    const byte lsb = Wire.read();
    const unsigned int voltage_sense = ((unsigned int) (msb) << 4) + ((lsb >> 4) & 0x0F); // formats into 12bit integer;

    return voltage_sense;
}

// returns the current in 5mA steps
unsigned int read_current() {
    Wire.beginTransmission(LTCADDR); // get sense current
    Wire.write(0x14);
    Wire.endTransmission(false);
    Wire.requestFrom(LTCADDR, 2, true);

    delay(1);

    const byte msb = Wire.read();
    const byte lsb = Wire.read();
    const unsigned int current_sense = (msb << 4) + ((lsb >> 4) & 0x0F); // 12 bit format

    return current_sense;
}

