#pragma once
#include <Arduino.h>
#include <HR500V1.h>
#include <HR500X.h>

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

struct amp_state {
    volatile bool txIsOn;
    mode_type mode = mode_type::standby; // 0 - OFF, 1 - PTT
    mode_type old_mode;

    volatile byte band = 6;
    byte oldBand = 0;


    volatile bool pttEnabled;

    int s_disp = 19;
    char RL_TXT[4] = {"-.-"};
    char ORL_TXT[4] = {"..."};

    volatile int timeToTouch = 0; // countdown for TS touching. this gets decremented in timer ISR
    byte menuChoice = 0;
    bool menuSelected = false;
    bool biasMeter = false;
    int MAX_CUR = 20;

    // alerts
    byte I_alert = 0, V_alert = 0, F_alert = 0, R_alert = 0, D_alert = 0;
    byte OI_alert = 0, OV_alert = 0, OF_alert = 0, OR_alert = 0, OD_alert = 0;

    byte meterSelection = 0; // 1 - FWD; 2 - RFL; 3 - DRV; 4 - VDD; 5 - IDD
    byte oldMeterSelection;

    byte trxType = 0;
    byte antForBand[11] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}; // antenna selection for each band
    bool isMenuActive; // 0 is normal, 1 is menu mode
    bool tempInCelsius = true; // display temperature in Celsius?

    serial_speed accSpeed;
    serial_speed usbSpeed;
};

mode_type nextMode(mode_type mode);
mode_type modeFromEEPROM(uint8_t mode);
uint8_t modeToEEPROM(mode_type mode);

serial_speed nextSerialSpeed(serial_speed speed);
serial_speed previousSerialSpeed(serial_speed speed);
serial_speed speedFromEEPROM(uint8_t speed);
uint8_t speedToEEPROM(serial_speed speed);

void PrepareForFWUpdate();

struct lpf_board {
    void sendRelayData(byte data);
    void sendRelayDataSafe(byte data);

    volatile byte serialData = 0; // serial data to be sent to LPF
};

struct atu_board {
    void detect();
    bool isPresent() const;
    const char* getVersion() const;
    size_t query(const char* command, char* response, size_t maxLength);
    void println(const char* command);
    void setTuning(bool enabled);
    void setActive(bool active);
    bool isTuning() const;
    bool isActive() const;

    bool present = false;
    char version[8] = {};
    volatile bool tuning;
    volatile bool active;
};

struct display {
};

struct touchscreen {
};

struct amplifier {

    void setup();
    void update();

    void tripClear();
    void tripSet();
    void readInputFrequency();
    void handleTouchScreen1();
    void handleTouchScreen2();
    byte getTouchedRectangle(byte touch_screen);
    void enablePTTDetector();
    void disablePTTDetector();
    void setBand();
    void switchToTX();
    void switchToRX();

    amp_state state;
    atu_board atu;
    lpf_board lpf;
    display tft1, tft2;
    // touchscreen ts1, ts2;
    XPT2046_Touchscreen ts1{TP1_CS};
    XPT2046_Touchscreen ts2{TP2_CS};
};

