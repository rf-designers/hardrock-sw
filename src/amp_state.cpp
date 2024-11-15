#include "amp_state.h"

#include <EEPROM.h>
#include <Wire.h>
#include <SPI.h>
#include <FreqCount.h>

#include "hr500_sensors.h"
#include "transceivers.h"
#include "amp_view.h"

#include "atu_functions.h"
#include "HR500.h"
#include "HR500V1.h"
#include "hr500_displays.h"
#include "menu_functions.h"


extern amp_view view;

extern int analog_read(byte pin);

mode_type next_mode(mode_type mode) {
    auto md = mode_to_eeprom(mode) + 1;
    if (md == 2) {
        md = 0;
    }
    return mode_from_eeprom(md);
}

mode_type mode_from_eeprom(uint8_t mode) {
    mode = mode > 1 ? 0 : mode;
    return static_cast<mode_type>(mode);
}

uint8_t mode_to_eeprom(mode_type mode) {
    return static_cast<uint8_t>(mode);
}

serial_speed next_serial_speed(serial_speed speed) {
    auto spd = speed_to_eeprom(speed);
    spd++;
    if (spd > speed_to_eeprom(serial_speed::baud_115200)) {
        spd = 0;
    }
    return speed_from_eeprom(spd);
}

serial_speed prev_serial_speed(serial_speed speed) {
    auto spd = speed_to_eeprom(speed);
    spd--;
    if (spd == 255) {
        spd = speed_to_eeprom(serial_speed::baud_115200);
    }
    return speed_from_eeprom(spd);
}

serial_speed speed_from_eeprom(uint8_t speed) {
    speed = speed > speed_to_eeprom(serial_speed::baud_115200) ? speed_to_eeprom(serial_speed::baud_115200) : speed;
    return static_cast<serial_speed>(speed);
}

uint8_t speed_to_eeprom(serial_speed speed) {
    return static_cast<uint8_t>(speed);
}

void prepare_for_fw_update() {
    Serial.end();
    delay(50);
    // Serial.begin(115200);
    // while (!Serial.available());
    // delay(50);
    digitalWrite(RST_OUT, LOW);
}

constexpr int volts_to_voltage_reading(const int volts) {
    return volts * 40; // voltage reading offers 25mV resolution
}

void atu_board::detect() {
    Serial3.begin(19200); // ATU
    Serial3.println(" ");
    strcpy(version, "---");

    char response[20] = {};
    if (query("*I", response, 20) > 10) {
        if (strncmp(response, "HR500 ATU", 9) == 0) {
            present = true;

            // memset(response, 0, 20);
            query("*V", response, 20);

            // ATU firmware currently responds with two lines for *I query only the first time (most likely bug)
            const auto versionSize = query("*V", response, 20);
            strncpy(version, response, versionSize);
        }
    }
}

bool atu_board::is_present() const {
    return present;
}

const char* atu_board::get_version() const {
    return version;
}

size_t atu_board::query(const char* command, char* response, size_t maxLength) {
    Serial3.setTimeout(100);
    Serial3.println(command);
    const size_t receivedBytes = Serial3.readBytesUntil(13, response, maxLength);
    last_response_size = receivedBytes;
    memcpy(last_response, response, receivedBytes);

    // look for a 13


    response[receivedBytes] = 0;
    return receivedBytes;
}

void atu_board::println(const char* command) {
    Serial3.println(command);
}

void atu_board::set_tuning(bool enabled) {
    tuning = enabled;
}

void atu_board::set_active(bool a) {
    active = a;
}

bool atu_board::is_tuning() const {
    return tuning;
}

bool atu_board::is_active() const {
    return active;
}

amplifier::amplifier() {
    ltc = new ltc2945{LTCADDR, 0.005};
}

void amplifier::setup() {
    ltc->set_max_current(20.2);
    ltc->set_max_voltage(60.0);
    ltc->set_max_rfl_power(100);

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

void amplifier::trip_clear() {
    Wire.beginTransmission(LTCADDR); // clear any existing faults
    Wire.write(0x04);
    Wire.endTransmission(false);
    Wire.requestFrom(LTCADDR, 2, true); // shouldn't this only read one byte?
    delay(1);
    Wire.read();
    delay(10);

    Wire.beginTransmission(LTCADDR); // set alert register
    Wire.write(0x01);
    Wire.write(0x02);
    Wire.endTransmission();

    Wire.requestFrom(LTCADDR, 2, true);
    delay(1);
}

void amplifier::trip_set() {
    Wire.beginTransmission(LTCADDR); // establish a fault condition
    Wire.write(0x03);
    Wire.write(0x02);
    Wire.endTransmission();
    Wire.requestFrom(LTCADDR, 2, true);

    delay(1);

    BIAS_OFF
    lpf.send_relay_data(lpf.serial_data);
    RF_BYPASS
}

void lpf_board::send_relay_data(byte data) {
    RELAY_CS_LOW
    SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE3));
    SPI.transfer(data);
    SPI.endTransaction();
    RELAY_CS_HIGH
}

void lpf_board::send_relay_data_safe(byte data) {
    noInterrupts();
    send_relay_data(data);
    interrupts();
}

// Reads input frequency and sets state.band accordingly
void amplifier::read_input_frequency() {
    int cnt = 0;
    byte same_cnt = 0;
    byte last_band = 0;
    FreqCountClass::begin(1);

    while (cnt < 12 && digitalRead(COR_DET)) {
        while (!FreqCountClass::available());

        const unsigned long frequency = FreqCountClass::read() * 16;
        byte band_read = 0;

        if (frequency > 1750 && frequency < 2100) band_read = 10;
        else if (frequency > 3000 && frequency < 4500) band_read = 9;
        else if (frequency > 4900 && frequency < 5700) band_read = 8;
        else if (frequency > 6800 && frequency < 7500) band_read = 7;
        else if (frequency > 9800 && frequency < 11000) band_read = 6;
        else if (frequency > 13500 && frequency < 15000) band_read = 5;
        else if (frequency > 17000 && frequency < 19000) band_read = 4;
        else if (frequency > 20500 && frequency < 22000) band_read = 3;
        else if (frequency > 23500 && frequency < 25400) band_read = 2;
        else if (frequency > 27500 && frequency < 30000) band_read = 1;

        if (band_read == last_band && last_band != 0) {
            same_cnt++;
        }

        if (same_cnt > 2) {
            state.band = last_band;
            cnt = 99;
        }

        last_band = band_read;
        cnt++;
    }

    FreqCountClass::end();
}


void amplifier::handle_ts1() {
    if (ts1.touched()) {
        const byte pressedKey = get_touched_rectangle(1);
        switch (pressedKey) {
        case 10:
            state.meterSelection = 1;
            break;

        case 11:
            state.meterSelection = 2;
            break;

        case 12:
            state.meterSelection = 3;
            break;

        case 13:
            state.meterSelection = 4;
            break;

        case 14:
            state.meterSelection = 5;
            break;

        case 18:
        case 19:
            state.temp_in_celsius = !state.temp_in_celsius;
            EEPROM.write(eecelsius, state.temp_in_celsius ? 1 : 0);
            break;
        }

        if (state.oldMeterSelection != state.meterSelection) {
            draw_button_up(state.oldMeterSelection);
            draw_button_down(state.meterSelection);
            state.oldMeterSelection = state.meterSelection;
            EEPROM.write(eemetsel, state.meterSelection);
        }

        while (ts1.touched());
    }
}

void amplifier::handle_ts2() {
    // if (state.timeToTouch != 0) return;
    if (!ts2.touched()) return;

    const byte pressedKey = get_touched_rectangle(2);

    if (state.is_menu_active) {
        switch (pressedKey) {
        case 0:
        case 1:
            if (!state.menuSelected) {
                // erase old
                lcd[1].draw_string(menu_items[state.menuChoice], 65, 20, 2, state.colors.named.menu_1);
                lcd[1].draw_string(item_disp[state.menuChoice], 65, 80, 2, state.colors.named.menu_1);

                if (state.menuChoice-- == 0)
                    state.menuChoice = menu_max;

                lcd[1].draw_string(menu_items[state.menuChoice], 65, 20, 2, state.colors.named.menu_2);
                lcd[1].draw_string(item_disp[state.menuChoice], 65, 80, 2, state.colors.named.menu_3);
            }
            break;

        case 3:
        case 4:
            if (!state.menuSelected) {
                // erase old
                lcd[1].draw_string(menu_items[state.menuChoice], 65, 20, 2, state.colors.named.menu_1);
                lcd[1].draw_string(item_disp[state.menuChoice], 65, 80, 2, state.colors.named.menu_1);

                if (++state.menuChoice > menu_max)
                    state.menuChoice = 0;

                lcd[1].draw_string(menu_items[state.menuChoice], 65, 20, 2, state.colors.named.menu_2);
                lcd[1].draw_string(item_disp[state.menuChoice], 65, 80, 2, state.colors.named.menu_3);
            }

            break;

        case 5:
        case 6:
            if (state.menuSelected) {
                menu_update(state.menuChoice, 0);
            }
            break;

        case 8:
        case 9:
            if (state.menuSelected) {
                menu_update(state.menuChoice, 1);
            }
            break;

        case 7:
        case 12:
            menuSelect();
            break;

        case 17:
            lcd[1].clear_screen(GRAY);
            state.is_menu_active = false;

            if (state.menuChoice == mSETbias) {
                BIAS_OFF
                lpf.send_relay_data_safe(lpf.serial_data);
                state.mode = state.old_mode;
                state.MAX_CUR = 20;
                state.biasMeter = false;
            }

            draw_home();
        //                lcd[0].lcd_reset();
        }
    } else {
        switch (pressedKey) {
        case 5:
        case 6:
            if (!state.tx_is_on) {
                state.mode = next_mode(state.mode);
                EEPROM.write(eemode, mode_to_eeprom(state.mode));
                draw_ptt_mode();
                disable_ptt_detector();
                if (state.mode == mode_type::ptt) {
                    enable_ptt_detector();
                }
            }
            break;

        case 8:
            if (!state.tx_is_on) {
                if (++state.band >= 11)
                    state.band = 1;
                set_band();
            }
            break;

        case 9:
            if (!state.tx_is_on) {
                if (--state.band == 0)
                    state.band = 10;

                if (state.band == 0xff)
                    state.band = 10;
                set_band();
            }
            break;

        case 15:
        case 16:
            if (!state.tx_is_on) {
                if (++state.antForBand[state.band] == 3)
                    state.antForBand[state.band] = 1;

                EEPROM.write(eeantsel + state.band, state.antForBand[state.band]);
                if (state.antForBand[state.band] == 1) {
                    SEL_ANT1;
                } else if (state.antForBand[state.band] == 2) {
                    SEL_ANT2;
                }
                draw_ant();
            }
            break;

        case 18:
        case 19:
            if (atu.is_present() && !state.tx_is_on) {
                atu.set_active(!atu.is_active());
                draw_atu();
            }
            break;

        case 12:
            if (!atu.is_present()) {
                lcd[1].clear_screen(GRAY);
                draw_menu();
                state.is_menu_active = true;
            }
            break;

        case 7:
            if (atu.is_present()) {
                lcd[1].clear_screen(GRAY);
                draw_menu();
                state.is_menu_active = true;
            }
            break;

        case 17:
            if (atu.is_present()) {
                on_tune_button_pressed();
            }
        }
    }
    while (ts2.touched());
}

byte amplifier::get_touched_rectangle(byte touch_screen) {
    uint16_t x, y;
    uint8_t key;
    TS_Point p;

    if (touch_screen == 1)
        p = ts1.getPoint();
    else if (touch_screen == 2)
        p = ts2.getPoint();

    x = map(p.x, 3850, 400, 0, 5);
    y = map(p.y, 350, 3700, 0, 4);
    //  z = p.z;
    key = x + 5 * y;
    return key;
}

// Enable interrupt on state change of D11 (PTT)
void amplifier::enable_ptt_detector() {
    noInterrupts();

    PCICR |= (1 << PCIE0);
    PCMSK0 |= (1 << PCINT5); // Set PCINT0 (digital input 11) to trigger an
    // interrupt on state change.

    interrupts();
}

// Disable interrupt for state change of D11 (PTT)
void amplifier::disable_ptt_detector() {
    noInterrupts();

    PCMSK0 &= ~(1 << PCINT5);

    interrupts();
}

void amplifier::set_band() {
    if (state.band > 10) return;
    if (state.tx_is_on) return;

    byte lpfSerialData = 0;
    switch (state.band) {
    case 0:
        lpfSerialData = 0x00;
        atu.println("*B0");
        break;
    case 1:
        lpfSerialData = 0x20;
        atu.println("*B1");
        break;
    case 2:
        lpfSerialData = 0x20;
        atu.println("*B2");
        break;
    case 3:
        lpfSerialData = 0x20;
        atu.println("*B3");
        break;
    case 4:
        lpfSerialData = 0x08;
        atu.println("*B4");
        break;
    case 5:
        lpfSerialData = 0x08;
        atu.println("*B5");
        break;
    case 6:
        lpfSerialData = 0x04;
        atu.println("*B6");
        break;
    case 7:
        lpfSerialData = 0x04;
        atu.println("*B7");
        break;
    case 8:
        lpfSerialData = 0x02;
        atu.println("*B8");
        break;
    case 9:
        lpfSerialData = 0x01;
        atu.println("*B9");
        break;
    case 10:
        lpfSerialData = 0x00;
        atu.println("*B10");
        break;
    }

    if (atu.is_present() && !state.is_menu_active) {
        lcd[1].fill_rect(121, 142, 74, 21, MGRAY);
    }

    lpf.serial_data = lpfSerialData;
    if (state.band != state.oldBand) {
        lpf.send_relay_data(lpf.serial_data);

        // delete old band (dirty rectangles)
        draw_band(state.oldBand, MGRAY);

        state.oldBand = state.band;

        // draw the new band
        const uint16_t dcolor = state.band == 0 ? RED : ORANGE;
        draw_band(state.band, dcolor);

        EEPROM.write(eeband, state.band);
        draw_ant();
    }
}

void amplifier::switch_to_tx() {
    for (auto i = 0; i < 5; i++) {
        state.alerts[i] = 1;
        state.old_alerts[i] = 0;
    }

    delay(20);
    RESET_PULSE
    state.swr_display_counter = 19;

    draw_rx_buttons(state.colors.named.text_disabled);
    draw_tx_sensor(state.colors.named.tx_sensor_tx);
}

void amplifier::switch_to_rx() {
    draw_rx_buttons(state.colors.named.text_enabled);
    draw_tx_sensor(state.colors.named.tx_sensor_rx);
    trip_clear();
    RESET_PULSE

    lcd[0].fill_rect(70, 203, 36, 16, MGRAY);
    lcd[0].draw_string(state.RL_TXT, 70, 203, 2, LGRAY);
    strcpy(state.ORL_TXT, "   ");
}

void amplifier::update_meter_drawing() {
    if (state.meterSelection == 1) {
        unsigned int f_pw = read_power(power_type::fwd_p);
        state.F_bar = constrain(map(f_pw, 0, 500, 19, 300), 10, 309);
    } else if (state.meterSelection == 2) {
        unsigned int r_pw = read_power(power_type::rfl_p);
        state.F_bar = constrain(map(r_pw, 0, 50, 19, 300), 10, 309);
    } else if (state.meterSelection == 3) {
        unsigned int d_pw = read_power(power_type::drv_p);
        state.F_bar = constrain(map(d_pw, 0, 100, 19, 300), 10, 309);
    } else if (state.meterSelection == 4) {
        state.F_bar = constrain(map(ltc->get_voltage(), 0, 60, 19, 299), 19, 309);
    } else {
        // MeterSel == 5
        state.F_bar = constrain(map(ltc->get_current(), 0, 20, 19, 299), 19, 309);
    }

    while (state.F_bar != state.OF_bar) {
        if (state.OF_bar < state.F_bar)
            lcd[0].draw_v_line(state.OF_bar++, 101, 12, GREEN);

        if (state.OF_bar > state.F_bar)
            lcd[0].draw_v_line(--state.OF_bar, 101, 12, MGRAY);
    }
}

void amplifier::update_bias_reading() {
    ltc->update();
    const int bias_current = ltc->get_current() * 1000;
    if (bias_current != state.old_bias_current) {
        state.old_bias_current = bias_current;
        lcd[1].draw_string(state.bias_text, 65, 80, 2, MGRAY);

        sprintf(state.bias_text, "  %d mA", bias_current);
        lcd[1].draw_string(state.bias_text, 65, 80, 2, WHITE);
    }
}

void amplifier::handle_trx_band_detection() {
    if (state.tx_is_on) return;

    // byte detected_band = this->get_detected_trx_band();
    Wire.beginTransmission(0x55); // arduino nano address
    Wire.write(0x01); // CMD_GET_BAND
    Wire.endTransmission(false);
    Wire.requestFrom(0x55, 1, true);

    delay(1);

    byte detected_band = Wire.read();
    if (detected_band != 0 && detected_band != state.band) {
        state.band = detected_band;
        set_band();
    }
}

byte amplifier::get_detected_trx_band() const {
    byte detected_band = 0;
    if (state.trx_type == xft817) {
        do {
            detected_band = FT817det();
        } while (detected_band != FT817det());
    } else if (state.trx_type == xxieg) {
        do {
            detected_band = Xiegudet();
        } while (detected_band != Xiegudet());
    } else if (state.trx_type == xelad) {
        do {
            detected_band = Eladdet();
        } while (detected_band != Eladdet());
    }

    return detected_band;
}

void amplifier::update_swr() {
    const auto vswr = read_power(power_type::vswr);
    if (vswr > 9) {
        sprintf(state.RL_TXT, "%d.%d", vswr / 10, vswr % 10);
    }

    // FIXME: compare vswr, not string.
    if (strcmp(state.ORL_TXT, state.RL_TXT) != 0) {
        lcd[0].fill_rect(70, 203, 36, 16, MGRAY);
        lcd[0].draw_string(state.RL_TXT, 70, 203, 2, WHITE);
        strcpy(state.ORL_TXT, state.RL_TXT);
    }
}

void amplifier::update_temperature() {
    state.temperature.add(constrain(analog_read(14), 5, 2000));
    auto temp = state.temperature.get();

    if (temp > 700 && state.tx_is_on) {
        trip_set();
    }

    if (state.temp_in_celsius) {
        state.temp_read = temp;
    } else {
        state.temp_read = ((temp * 9) / 5) + 320;
    }

    state.temp_read /= 10;

    if (state.temp_read != state.old_temp_read) {
        state.old_temp_read = state.temp_read;
        view.item_changed(refresh_item::view_item_temperature);
    }
}

void amplifier::load_eeprom_config() {
    state.band = EEPROM.read(eeband);
    if (state.band > 10)
        state.band = 5;

    state.mode = mode_from_eeprom(EEPROM.read(eemode));
    disable_ptt_detector();
    if (state.mode != mode_type::standby) {
        enable_ptt_detector();
    }

    atu.set_active(EEPROM.read(eeatub) == 1);
    for (byte i = 1; i < 11; i++) {
        state.antForBand[i] = EEPROM.read(eeantsel + i);

        if (state.antForBand[i] == 1) {
            // SEL_ANT1;
        } else if (state.antForBand[i] == 2) {
            // SEL_ANT2;
        } else {
            // SEL_ANT1;
            state.antForBand[i] = 1;
            EEPROM.write(eeantsel + i, 1);
        }
    }

    state.accSpeed = speed_from_eeprom(EEPROM.read(eeaccbaud));
    setup_acc_serial(state.accSpeed);

    state.usbSpeed = speed_from_eeprom(EEPROM.read(eeusbbaud));
    setup_usb_serial(state.usbSpeed);

    state.temp_in_celsius = EEPROM.read(eecelsius) > 0;

    state.meterSelection = EEPROM.read(eemetsel);
    if (state.meterSelection < 1 || state.meterSelection > 5) {
        state.meterSelection = 1;
    }
    state.oldMeterSelection = state.meterSelection;


    state.trx_type = constrain(EEPROM.read(eexcvr), 0, xcvr_max);
    strcpy(item_disp[mXCVR], xcvr_disp[state.trx_type]);
    this->set_transceiver(state.trx_type);

    byte MCAL = constrain(EEPROM.read(eemcal), 75, 125);
    state.M_CORR = static_cast<long>(MCAL);
    sprintf(item_disp[mMCAL], "      %3li       ", state.M_CORR);
}

void amplifier::set_transceiver(byte type) {
    Wire.beginTransmission(0x55); // arduino nano address
    Wire.write(0x02); // CMD_SET_TRX_TYPE
    Wire.write(type);
    Wire.endTransmission();

    // if (type == xhobby || type == xkx23)
    //     S_POL_REV
    // else
    //     S_POL_NORM
    //
    // if (type == xhobby) {
    //     pinMode(TTL_PU, OUTPUT);
    //     digitalWrite(TTL_PU, HIGH);
    // } else {
    //     digitalWrite(TTL_PU, LOW);
    //     pinMode(TTL_PU, INPUT);
    // }
    //
    // if (type == xft817 || type == xelad || type == xxieg) {
    //     strcpy(item_disp[mACCbaud], "  XCVR MODE ON  ");
    //     Serial2.end();
    // } else {
    //     setup_acc_serial(state.accSpeed);
    // }
    //
    // if (type == xic705) {
    //     state.accSpeed = serial_speed::baud_19200;
    //     EEPROM.write(eeaccbaud, speed_to_eeprom(state.accSpeed));
    //     setup_acc_serial(state.accSpeed);
    // }
}

void amplifier::configure_attenuator() {
    if (ATTN_INST_READ == LOW) {
        state.attenuator_present = true;
        state.attenuator_enabled = EEPROM.read(eeattn) == 1;

        if (state.attenuator_enabled) {
            ATTN_ON_HIGH;
            item_disp[mATTN] = (char*)" ATTENUATOR IN  ";
        } else {
            ATTN_ON_LOW;
            item_disp[mATTN] = (char*)" ATTENUATOR OUT ";
        }
    } else {
        state.attenuator_present = false;
        state.attenuator_enabled = false;
        item_disp[mATTN] = (char*)" NO ATTENUATOR  ";
    }
}

void amplifier::set_fan_speed(int speed) {
    // speed = [0,3]
    switch (speed) {
    case 0:
        digitalWrite(FAN1, LOW);
        digitalWrite(FAN2, LOW);
        state.temp_utp = 400;
        state.temp_dtp = 00;
        break;

    case 1:
        digitalWrite(FAN1, HIGH);
        digitalWrite(FAN2, LOW);
        state.temp_utp = 500;
        state.temp_dtp = 350;
        break;

    case 2:
        digitalWrite(FAN1, LOW);
        digitalWrite(FAN2, HIGH);
        state.temp_utp = 600;
        state.temp_dtp = 450;
        break;

    case 3:
    default:
        digitalWrite(FAN1, HIGH);
        digitalWrite(FAN2, HIGH);
        state.temp_utp = 2500;
        state.temp_dtp = 550;
    }
}

void amplifier::configure_adc() {
    analogReference(EXTERNAL);
    const unsigned char PS_32 = (1 << ADPS2) | (1 << ADPS0);
    const unsigned char PS_128 = (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
    ADCSRA &= ~PS_128;
    ADCSRA |= PS_32;
}

void amplifier::update_fan_speed() {
    auto temp = state.temperature.get();
    if (temp > state.temp_utp)
        set_fan_speed(++state.fan_speed);
    else if (temp < state.temp_dtp)
        set_fan_speed(--state.fan_speed);
}

void amplifier::update_alerts() {
    this->update_fwd_pwr_alert();
    this->update_rfl_pwr_alert();
    this->update_drive_pwr_alert();
    this->update_vdd_alert();
    this->update_idd_alert();
}

void amplifier::update_fwd_pwr_alert() {
    unsigned int f_yel = 600, f_red = 660;
    if (state.band == 10) {
        f_yel = 410;
        f_red = 482;
    }

    if (state.fwd_tot > f_red) {
        state.alerts[alert_fwd_pwr] = 3;
        trip_set();
    } else if (state.fwd_tot > f_yel && state.alerts[alert_fwd_pwr] == 1) {
        state.alerts[alert_fwd_pwr] = 2;
    }

    if (state.alerts[alert_fwd_pwr] != state.old_alerts[alert_fwd_pwr]) {
        state.old_alerts[alert_fwd_pwr] = state.alerts[alert_fwd_pwr];
        view.item_changed(refresh_item::view_item_fwd_alert);
    }
}

void amplifier::update_rfl_pwr_alert() {
    if (state.rfl_tot > 590) {
        state.alerts[alert_rfl_pwr] = 3;
        trip_set();
    } else if (state.rfl_tot > 450 && state.alerts[alert_rfl_pwr] == 1) {
        state.alerts[alert_rfl_pwr] = 2;
    }

    if (state.alerts[alert_rfl_pwr] != state.old_alerts[alert_rfl_pwr]) {
        state.old_alerts[alert_rfl_pwr] = state.alerts[alert_rfl_pwr];
        view.item_changed(refresh_item::view_item_rfl_alert);
    }
}

void amplifier::update_drive_pwr_alert() {
    if (state.drv_tot > 1100) {
        state.alerts[alert_drive_pwr] = 3;
    } else if (state.drv_tot > 900 && state.alerts[alert_drive_pwr] == 1) {
        state.alerts[alert_drive_pwr] = 2;
    }

    if (state.attenuator_enabled)
        state.alerts[alert_drive_pwr] = 0;

    if (!state.tx_is_on)
        state.alerts[alert_drive_pwr] = 0;

    if (state.alerts[alert_drive_pwr] != state.old_alerts[alert_drive_pwr]) {
        state.old_alerts[alert_drive_pwr] = state.alerts[alert_drive_pwr];

        if (state.alerts[alert_drive_pwr] == 3 && state.tx_is_on) {
            trip_set();
        }
        view.item_changed(refresh_item::view_item_drive_alert);
    }
}

void amplifier::update_vdd_alert() {
    state.alerts[alert_vdd] = 1;

    const int dc_voltage = ltc->get_voltage();
    if (dc_voltage < 45 || dc_voltage > 75) {
        state.alerts[alert_vdd] = 2;
    }

    if (state.alerts[alert_vdd] != state.old_alerts[alert_vdd]) {
        state.old_alerts[alert_vdd] = state.alerts[alert_vdd];
        view.item_changed(refresh_item::view_item_voltage_alert);
    }
}

void amplifier::update_idd_alert() {
    const int dc_current = ltc->get_current();
    if (dc_current > 20) {
        state.alerts[alert_idd] = 3;
        trip_set();
    } else if (dc_current > 18 && state.alerts[alert_idd] == 1) {
        state.alerts[alert_idd] = 2;
    }

    if (state.alerts[alert_idd] != state.old_alerts[alert_idd]) {
        state.old_alerts[alert_idd] = state.alerts[alert_idd];
        view.item_changed(refresh_item::view_item_current_alert);
    }
}
