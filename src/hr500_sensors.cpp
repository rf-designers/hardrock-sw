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

ltc2945::ltc2945(const uint8_t address, const double sense_resistor)
    : address(address),
      max_sense_amps{d_sense_range / sense_resistor},
      sense_resistor(sense_resistor) {
}

void ltc2945::set_max_current(double amps) {
    // we need to set the 12bit value which tops out at 0.1024V
    constexpr uint32_t current_full_scale = 0b111111111111; // 12 bit
    const auto threshold_value = static_cast<uint32_t>((amps * current_full_scale) / max_sense_amps);

    const uint8_t msb = threshold_value >> 4 & 0xff;
    const uint8_t lsb = (threshold_value & 0xf) << 4;

    Wire.beginTransmission(address);
    Wire.write(0x1A); // MAX ∆SENSE THRESHOLD MSB
    Wire.write(msb);
    Wire.write(lsb);
    Wire.endTransmission();
}


void ltc2945::set_max_voltage(double maxVolts) {
    const auto threshold_value = static_cast<uint32_t>(maxVolts / v_per_bit) & 0x0FFF;

    const uint8_t msb = threshold_value >> 4 & 0xff;
    const uint8_t lsb = (threshold_value & 0xf) << 4;

    Wire.beginTransmission(address);
    Wire.write(0x24); // MAX VIN THRESHOLD MSB
    Wire.write(msb);
    Wire.write(lsb);
    Wire.endTransmission();
}

void ltc2945::set_max_rfl_power(double watts) {
    // ADIN is 12bit and has full scale voltage of 2.048V
    // this means that if we want the top to be 100W then
    // we need to scale the Vrfl coming to ADIN so that 2.048 will be 100W of reflected power

    const auto threshold_value = static_cast<uint32_t>(watts / max_rfl_watts) & 0x0FFF;

    const uint8_t msb = threshold_value >> 4 & 0xff;
    const uint8_t lsb = (threshold_value & 0xf) << 4;

    Wire.beginTransmission(address);
    Wire.write(0x2E); // MAX ADIN THRESHOLD MSB
    Wire.write(msb);
    Wire.write(lsb);
    Wire.endTransmission();
}

void ltc2945::enable_alerts_for(uint8_t params) {
    Wire.beginTransmission(address);
    Wire.write(0x01); // ALERT register, Selects Which Faults Generate Alerts
    Wire.write(alerts_from_params(params));
    Wire.endTransmission();
}

uint8_t ltc2945::alerts_from_params(uint8_t params) const {
    uint8_t alerts = 0;
    if ((params & param_voltage) == param_voltage) {
        alerts |= 0b00001000; // bit 3 = Maximum VIN Alert
    }
    if ((params & param_current) == param_current) {
        alerts |= 0b00100000; // bit 5 = Maximum ∆SENSE Alert
    }
    if ((params & param_rfl_pwr) == param_rfl_pwr) {
        alerts |= 0b00000010; // bit 1 =  Maximum ADIN Alert
    }
    return alerts;
}

uint8_t ltc2945::params_from_alerts(uint8_t alerts) const {
    uint8_t params = 0;
    if ((alerts & 0b00001000) == 0b00001000) {
        params |= param_voltage; // bit 3 = Maximum VIN Alert
    }
    if ((alerts & 0b00100000) == 0b00100000) {
        params |= param_current; // bit 5 = Maximum ∆SENSE Alert
    }
    if ((alerts & 0b00000010) == 0b00000010) {
        params |= param_rfl_pwr; // bit 1 =  Maximum ADIN Alert
    }
    return params;
}

void ltc2945::update() {
    // read status with clear on read
    // 04h - FAULT CoR (E) - CoR - Same Data as Register D, D Content Cleared on Read
    status = read_byte_at_address(0x04);

    // 1Eh: VIN MSB
    // 14h: ∆SENSE MSB
    // 28h: ADIN MSB
    voltage = voltage_from_12bit(read_12bit_at_address(0x1E));
    current = current_from_12bit(read_12bit_at_address(0x14));
    reflected_power = reflected_power_from_12bit(read_12bit_at_address(0x28));
}

bool ltc2945::is_tripped() const {
    return status > 0;
}

bool ltc2945::is_alert_triggered_for(uint8_t param) const {
    return status & alerts_from_params(param);
}

uint8_t ltc2945::get_triggered_alerts() const {
    return params_from_alerts(status);
}

uint8_t ltc2945::read_byte_at_address(uint8_t reg_addr) const {
    Wire.beginTransmission(address);
    Wire.write(reg_addr);
    Wire.endTransmission(false);

    Wire.requestFrom(static_cast<int>(address), 1, true);
    delay(1); // needed?
    return Wire.read();
}

uint32_t ltc2945::read_12bit_at_address(uint8_t reg_addr) const {
    Wire.beginTransmission(address);
    Wire.write(reg_addr);
    Wire.endTransmission(false);

    Wire.requestFrom(static_cast<int>(address), 2, true);
    delay(1); // needed?
    const byte msb = Wire.read();
    const byte lsb = Wire.read();

    const unsigned int result = (static_cast<unsigned int>(msb) << 4) + (lsb >> 4 & 0x0F);
    // formats into 12bit integer;
    return result;
}

double ltc2945::voltage_from_12bit(uint32_t value) const {
    return value * v_per_bit;
}

double ltc2945::current_from_12bit(uint32_t value) {
    return value * max_sense_amps / static_cast<double>(0xFFFF);
}

double ltc2945::reflected_power_from_12bit(uint32_t value) {
    return value * max_rfl_watts / static_cast<double>(0xFFFF);
}
