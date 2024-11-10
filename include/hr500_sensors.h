#pragma once

#include "amp_state.h"

unsigned int read_power(power_type type);

class ltc2945 {
public:

    enum parameters {
        param_voltage = 1,
        param_current = 2,
        param_rfl_pwr = 4
    };

    explicit ltc2945(uint8_t address, double sense_resistor);

    void set_max_current(double amps);
    void set_max_voltage(double volts);
    void set_max_rfl_power(double watts);
    void enable_alerts_for(uint8_t params);

    void update();

    bool is_tripped() const;
    bool is_alert_triggered_for(uint8_t param) const;
    uint8_t get_triggered_alerts() const;

    // returns last read current in amps
    double get_current() const { return current; }

    // returns last read voltage in volts
    double get_voltage() const { return voltage; }

    // returns last read reflected power in watts
    double get_rfl_power() const { return reflected_power; }

private:

    uint8_t alerts_from_params(uint8_t params) const;
    uint8_t params_from_alerts(uint8_t alerts) const;
    uint8_t read_byte_at_address(uint8_t reg_addr) const;
    uint32_t read_12bit_at_address(uint8_t reg_addr) const;
    double voltage_from_12bit(uint32_t value) const;
    double current_from_12bit(uint32_t value);
    double reflected_power_from_12bit(uint32_t value);

    const uint8_t address = 0;
    const double d_sense_range = 0.1024; // V, max ADC
    const double max_sense_amps = 0;
    const double sense_resistor = 0.005; // ohms
    const double v_per_bit = 0.025; // V,
    const double max_rfl_watts = 100.0;

    uint8_t status = 0;
    double current = 0;
    double voltage = 0;
    double reflected_power = 0;
    uint8_t alert_flags = 0;
};