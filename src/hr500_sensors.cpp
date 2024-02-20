#include "hr500_sensors.h"
#include "amp_state.h"
#include <Wire.h>
#include "HR500V1.h"

extern volatile unsigned int f_tot, f_ave;
extern volatile unsigned int r_tot;
extern volatile unsigned int d_tot;
extern amp_state state;

void SendLPFRelayData(byte data);

unsigned int ReadPower(power_type powerType) {
    long pCalc, tCalc, bCalc;
    long pResult{0};

    switch (powerType) {
        case power_type::fwd_p:
            if (f_tot == 0) return 0;
            pCalc = long(f_tot) + long(30);
            pResult = (pCalc * pCalc) / long(809);
            break;
        case power_type::rfl_p:
            if (r_tot == 0) return 0;

            pCalc = long(r_tot) + long(140);
            pResult = (pCalc * pCalc) / long(10716);
            break;
        case power_type::drv_p:
            if (d_tot == 0) return 0;

            pCalc = long(d_tot) + long(30);
            pResult = (pCalc * pCalc) / long(10860);
            break;
        case power_type::vswr:
            if (f_tot < 205) return 0;
            if (r_tot < 33) return 10;

            tCalc = long(100) * long(f_tot) + long(30) * (r_tot) + long(2800);
            bCalc = long(10) * long(f_tot) - long(3) * long(r_tot) - long(280);
            pResult = tCalc / bCalc;

            if (pResult < 10) pResult = 10;
            if (pResult > 99) pResult = 99;
            break;
    }

    return pResult;
}


// returns the voltage in 25mV steps
unsigned int ReadVoltage() {
    Wire.beginTransmission(LTCADDR); // first get Input Voltage - 80V max
    Wire.write(0x1e);
    Wire.endTransmission(false);
    Wire.requestFrom(LTCADDR, 2, true);
    delay(1);
    byte ADCvinMSB = Wire.read();
    byte ADCvinLSB = Wire.read();
    return ((unsigned int) (ADCvinMSB) << 4) + ((ADCvinLSB >> 4) & 0x0F); // formats into 12bit integer
}

// returns the current in 5mA steps
unsigned int ReadCurrent() {
    Wire.beginTransmission(LTCADDR); // get sense current
    Wire.write(0x14);
    Wire.endTransmission(false);
    Wire.requestFrom(LTCADDR, 2, true);
    delay(1);
    byte curSenseMSB = Wire.read();
    byte curSenseLSB = Wire.read();

    return (curSenseMSB << 4) + ((curSenseLSB >> 4) & 0x0F); // 12 bit format
}

void TripClear() {
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

void TripSet() {
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
