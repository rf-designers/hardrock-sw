#pragma once

#include <Arduino.h>
#include <display_board.h>
#include <HR500V1.h>
#include <HR500X.h>
#include "color_theme.h"
#include "moving_average.h"

enum class mode_type : uint8_t {
    standby = 0,
    ptt = 1,
    qrp = 2
};

enum class power_type : uint8_t {
    fwd_p = 0,
    rfl_p = 1,
    drv_p = 2,
    vswr = 3
};

enum class serial_speed : uint8_t {
    baud_4800 = 0,
    baud_9600 = 1,
    baud_19200 = 2,
    baud_38400 = 3,
    baud_57600 = 4,
    baud_115200 = 5
};

enum alert_type {
    alert_fwd_pwr = 0,
    alert_rfl_pwr = 1,
    alert_drive_pwr = 2,
    alert_vdd = 3,
    alert_idd = 4
};

struct amp_state {
    volatile bool tx_is_on = false;
    mode_type mode = mode_type::standby; // 0 - OFF, 1 - PTT
    mode_type old_mode = mode_type::standby;

    volatile byte band = 6;
    byte oldBand = 0;

    volatile bool ptt_enabled = false;

    int swr_display_counter = 19;
    char RL_TXT[4] = {"-.-"};
    char ORL_TXT[4] = {"..."};

    volatile int touch_enable_counter = 0; // countdown for TS touching. this gets decremented in timer ISR
    byte menuChoice = 0;
    bool menuSelected = false;
    bool biasMeter = false;
    int MAX_CUR = 20;

    // alerts
    byte alerts[5] = {0, 0, 0, 0, 0};
    byte old_alerts[5] = {0, 0, 0, 0, 0};

    byte meterSelection = 0; // 1 - FWD; 2 - RFL; 3 - DRV; 4 - VDD; 5 - IDD
    byte oldMeterSelection = 0;

    byte trx_type = 0;
    byte antForBand[11] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}; // antenna selection for each band
    bool is_menu_active = false; // 0 is normal, 1 is menu mode
    bool tempInCelsius = true; // display temperature in Celsius?

    serial_speed accSpeed = serial_speed::baud_19200;
    serial_speed usbSpeed = serial_speed::baud_19200;

    unsigned int F_bar = 9, OF_bar = 9;

    char bias_text[11] = {"          "};
    int old_bias_current = -1;

    // temp
    moving_average<unsigned int, 11, 1> temperature;
    char TEMPbuff[16];
    int temp_read = 0;
    int old_temp_read = -99;

    // meter calibration
    long M_CORR = 100;

    // attenuator status
    bool attenuator_present = false;
    bool attenuator_enabled = false;

    unsigned int temp_utp = 0, temp_dtp = 0;
    color_theme colors = color_theme::make_classic();

    byte fan_speed = 0;
    volatile unsigned int f_tot = 0;
    volatile unsigned int r_tot = 0;
    volatile unsigned int d_tot = 0;

    volatile unsigned int f_pep = 0;
    volatile unsigned int r_pep = 0;
    volatile unsigned int d_pep = 0;
};

mode_type next_mode(mode_type mode);
mode_type mode_from_eeprom(uint8_t mode);
uint8_t mode_to_eeprom(mode_type mode);

serial_speed next_serial_speed(serial_speed speed);
serial_speed prev_serial_speed(serial_speed speed);
serial_speed speed_from_eeprom(uint8_t speed);
uint8_t speed_to_eeprom(serial_speed speed);

void prepare_for_fw_update();

struct lpf_board {
    void send_relay_data(byte data);
    void send_relay_data_safe(byte data);

    volatile byte serial_data = 0; // serial data to be sent to LPF
};

struct atu_board {
    void detect();
    bool is_present() const;
    const char *get_version() const;
    size_t query(const char *command, char *response, size_t maxLength);
    void println(const char *command);
    void set_tuning(bool enabled);
    void set_active(bool active);
    bool is_tuning() const;
    bool is_active() const;

    bool present = false;
    char version[8] = {0};
    volatile bool tuning;
    volatile bool active;
    size_t last_response_size;
    char last_response[100];
};

struct amplifier {
    void setup();

    void trip_clear();
    void trip_set();

    void read_input_frequency();

    void handle_ts1();
    void handle_ts2();

    byte get_touched_rectangle(byte touch_screen);

    void enable_ptt_detector();
    void disable_ptt_detector();

    void set_band();

    void switch_to_tx();
    void switch_to_rx();

    amp_state state{};
    atu_board atu{};
    lpf_board lpf{};
    display_board lcd[2] = {display_board{new lcd1}, display_board{new lcd2}};
    XPT2046_Touchscreen ts1{TP1_CS};
    XPT2046_Touchscreen ts2{TP2_CS};
    void update_meter_drawing();
    void update_bias_reading();
    void handle_trx_band_detection();
    byte get_detected_trx_band() const;
    void update_swr();
    void update_temperature();
    void load_eeprom_config();
    void set_transceiver(byte type);
    void configure_attenuator();
    void set_fan_speed(int i);
    void configure_adc();
    void update_fan_speed();
    void update_alerts();
    void update_fwd_pwr_alert();
    void update_rfl_pwr_alert();
    void update_drive_pwr_alert();
    void update_vdd_alert();
    void update_idd_alert();
};

