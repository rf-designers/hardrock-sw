//Hardrock 500 Firmware
//Version 3.5

#include "HR500V1.h"
#include "amp_state.h"
#include "atu_functions.h"
#include "hr500_displays.h"
#include "hr500_sensors.h"
#include "transceivers.h"
#include <HR500.h>
#include <HR500X.h>
#include <TimerOne.h>
#include <Wire.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/io.h>

#ifndef BODS
#define BODS 7
#endif

#ifndef BODSE
#define BODSE 2
#endif


// Flags to indicate data received on Serial or Serial2
volatile bool serialEvent = false;
volatile bool serial2Event = false;

// Interrupt Service Routine for Serial RX (Pin 0)
void serialRxISR() {
    serialEvent = true;
}

// Interrupt Service Routine for Serial2 RX (Pin 17)
void serial2RxISR() {
    serial2Event = true;
}


ISR(WDT_vect) {
    wdt_reset();
}

amplifier amp;

int analog_read(byte pin);
extern void handle_acc_comms();
extern void handle_usb_comms();
void update_alarms();

constexpr int volts_to_voltage_reading(const int volts);
void update_fan_speed();
byte FAN_SP = 0;
char ATU_STAT;

volatile unsigned int f_tot = 0, f_ave = 0;
volatile unsigned int f_avg[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
volatile byte f_i = 0;
volatile unsigned int r_tot = 0;
volatile unsigned int d_tot = 0;

volatile unsigned int f_pep = 0;
volatile unsigned int r_pep = 0;
volatile unsigned int d_pep = 0;


volatile bool shouldHandlePttChange = false;

// Countdown timer for re-enabling PTT detector interrupt. This gets updated in
// the timer ISR
volatile byte timeToEnablePTTDetector = 0;

byte OLD_COR = 0;


// This interrupt driven function reads data from the wattmeter
void timer_isr() {
    if (amp.state.tx_is_on) {
        // read forward power
        long s_calc = static_cast<long>(analogRead(12)) * amp.state.M_CORR;
        f_avg[f_i] = s_calc / static_cast<long>(100);

        f_ave += f_avg[f_i++]; // Add in the new sample

        if (f_i > 24)
            f_i = 0; // Update the index value

        f_ave -= f_avg[f_i]; // Subtract off the 51st value
        const unsigned int ftmp = f_ave / 25;

        if (ftmp > f_tot) {
            f_tot = ftmp;
            f_pep = 75;
        } else {
            if (f_pep > 0)
                f_pep--;
            else if (f_tot > 0)
                f_tot--;
        }

        // read reflected power
        s_calc = static_cast<long>(analogRead(13)) * amp.state.M_CORR;
        const unsigned int rtmp = s_calc / static_cast<long>(100);

        if (rtmp > r_tot) {
            r_tot = rtmp;
            r_pep = 75;
        } else {
            if (r_pep > 0)
                r_pep--;
            else if (r_tot > 0)
                r_tot--;
        }
    } else {
        f_tot = 0;
        r_tot = 0;
    }

    // read drive power
    long dtmp = analogRead(15);
    dtmp = (dtmp * static_cast<long>(d_lin[amp.state.band])) / static_cast<long>(100);

    if (dtmp > d_tot) {
        d_tot = dtmp;
        d_pep = 75;
    } else {
        if (d_pep > 0)
            d_pep--;
        else if (d_tot > 0)
            d_tot--;
    }

    // handle touching repeat
    if (amp.state.touch_enable_counter > 0) {
        amp.state.touch_enable_counter--;
    }

    // handle reenabling of PTT detection
    if (timeToEnablePTTDetector > 0) {
        timeToEnablePTTDetector--;

        if (timeToEnablePTTDetector == 0 && amp.state.mode != mode_type::standby) {
            amp.enable_ptt_detector();
        }
    }
}


int analog_read(byte pin) {
    noInterrupts();
    int a = analogRead(pin);
    interrupts();
    return a;
}

void wake_up_from_sleep() {
    // Briefly disable sleep mode to allow further processing
    sleep_disable();
    EIFR |= 0b11111111; // clear interrupt flags so that we can have multiple subsequent touches
}

void go_to_sleep() {
    // Ensure the BOD (Brown-out Detector) is disabled in sleep
    MCUCR = (1 << BODS) | (1 << BODSE);
    MCUCR = (1 << BODS);

    sleep_enable();
    sleep_mode();
}

void config_wakeup_pins() {
    noInterrupts();

    pinMode(18, INPUT);
    attachInterrupt(digitalPinToInterrupt(18), wake_up_from_sleep, FALLING);

    pinMode(19, INPUT);
    attachInterrupt(digitalPinToInterrupt(19), wake_up_from_sleep, FALLING);

    // set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    set_sleep_mode(SLEEP_MODE_IDLE);

    interrupts();
}

void config_watchdog_timer() {
    // Clear the reset flag
    MCUSR &= ~(1 << WDRF);

    wdt_reset();
    // Configure the watchdog timer to interrupt mode only (no reset)
    WDTCSR |= (1 << WDCE) | (1 << WDE);
    WDTCSR = (1 << WDIE) | (1 << WDP2) | (1 << WDP1); // 1second

    // Set the watchdog timer to wake up every second
    // wdt_enable(WDTO_1S);
}


void setup() {
    amp.setup();
    amp.load_eeprom_config();
    amp.configure_attenuator();


    analogReference(EXTERNAL);
    const unsigned char PS_32 = (1 << ADPS2) | (1 << ADPS0);
    const unsigned char PS_128 = (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
    ADCSRA &= ~PS_128;
    ADCSRA |= PS_32;

    Timer1.initialize(1000);
    Timer1.attachInterrupt(timer_isr);
    interrupts();

    SPI.setDataMode(SPI_MODE3);
    SPI.setBitOrder(MSBFIRST);
    SPI.setClockDivider(SPI_CLOCK_DIV2);
    SPI.begin();

    Wire.begin();
    Wire.setClock(400000);
    amp.trip_clear();

    amp.lcd[0].lcd_init(GRAY);
    amp.lcd[1].lcd_init(GRAY);

    amp.set_fan_speed(0);
    draw_meter();

    Serial3.begin(19200); // ATU
    amp.atu.detect();

    draw_home();
    amp.lcd[0].fill_rect(20, 34, 25, 10, GREEN);
    amp.lcd[0].fill_rect(84, 34, 25, 10, GREEN);

    if (amp.state.ATTN_ST == 0)
        amp.lcd[0].fill_rect(148, 34, 25, 10, GREEN);

    amp.lcd[0].fill_rect(212, 34, 25, 10, GREEN);
    amp.lcd[0].fill_rect(276, 34, 25, 10, GREEN);

    shouldHandlePttChange = false;

    while (amp.ts1.touched());
    while (amp.ts2.touched());

    amp.set_band();

    config_wakeup_pins();
    config_watchdog_timer();

    // Attach interrupts to RX pins
    attachInterrupt(digitalPinToInterrupt(0), serialRxISR, FALLING); // RX for Serial
    attachInterrupt(digitalPinToInterrupt(17), serial2RxISR, FALLING); // RX for Serial2
}

ISR(PCINT0_vect) {
    sleep_disable();

    if (amp.state.mode == mode_type::standby) return; // Mode is STBY
    if (amp.state.isMenuActive) return; // Menu is active
    if (amp.state.band == 0) return; // Band is undefined
    if (amp.atu.is_tuning()) return; // ATU is working

    // timeToEnablePTTDetector = 20;
    // amp.disablePTTDetector();

    shouldHandlePttChange = true; // signal to main thread

    const auto pttEnabledNow = digitalRead(PTT_DET) == 1;

    if (pttEnabledNow && !amp.state.pttEnabled) {
        amp.state.pttEnabled = true;
        RF_ACTIVE
        amp.lpf.send_relay_data(amp.lpf.serial_data + 0x10);
        BIAS_ON
        amp.state.tx_is_on = true;
    } else {
        amp.state.pttEnabled = false;
        BIAS_OFF
        amp.state.tx_is_on = false;
        amp.lpf.send_relay_data(amp.lpf.serial_data);
        RF_BYPASS
    }
}

void loop() {
    static int a_count = 0;
    if (++a_count == 10) {
        a_count = 0;
        update_alarms();
        update_fan_speed();
    }

    static int t_count = 0;
    if (++t_count == 3) {
        t_count = 0;

        amp.update_meter_drawing();
        if (amp.state.biasMeter) {
            amp.update_bias_reading();
        }

        if (amp.state.swr_display_counter++ == 20 && f_tot > 250 && amp.state.tx_is_on) {
            amp.state.swr_display_counter = 0;
            amp.update_swr();
        }

        // temperature display
        static int temperature_display_counter = 0;
        if (temperature_display_counter++ == 200) {
            temperature_display_counter = 0;
            amp.update_temperature();
        }
    }

    if (shouldHandlePttChange) {
        shouldHandlePttChange = false;

        if (amp.state.pttEnabled) {
            if (amp.state.mode != mode_type::standby) {
                amp.switch_to_tx();
            }

            const auto pttEnabledNow = digitalRead(PTT_DET) == 1;
            if (!pttEnabledNow && amp.state.mode == mode_type::ptt) {
                amp.switch_to_rx();
            }
        } else {
            if (amp.state.mode != mode_type::standby) {
                amp.switch_to_rx();
            }

            const auto pttEnabledNow = digitalRead(PTT_DET) == 1;

            if (pttEnabledNow) {
                amp.switch_to_tx();
            }
        }
    }

    byte sampCOR = digitalRead(COR_DET);

    if (OLD_COR != sampCOR) {
        OLD_COR = sampCOR;

        if (sampCOR == 1) {
            if (amp.state.trx_type != xft817 && amp.state.tx_is_on) {
                amp.read_input_frequency();
            }

            if (amp.state.band != amp.state.oldBand) {
                BIAS_OFF
                RF_BYPASS
                amp.state.tx_is_on = false;
                amp.set_band();
            }
        }
    }

    amp.handle_trx_band_detection();
    amp.handle_ts1();
    amp.handle_ts2();

    const auto atuBusy = digitalRead(ATU_BUSY) == 1;

    if (amp.atu.is_tuning() && !atuBusy) {
        on_tune_end();
    }

    handle_acc_comms();
    handle_usb_comms();

//    if (!amp.state.txIsOn) {
//        goToSleep();
//    }
}

void update_fan_speed() {
    if (amp.state.t_ave > amp.state.temp_utp)
        amp.set_fan_speed(++FAN_SP);
    else if (amp.state.t_ave < amp.state.temp_dtp)
        amp.set_fan_speed(--FAN_SP);
}

void update_alarms() {
    // forward alert
    unsigned int f_yel = 600, f_red = 660;

    if (amp.state.band == 10) {
        f_yel = 410;
        f_red = 482;
    }

    // establish current forward power alert
    if (f_tot > f_red) {
        amp.state.F_alert = 3;
        amp.trip_set();
    } else if (f_tot > f_yel && amp.state.F_alert == 1) {
        amp.state.F_alert = 2;
    }

    // see if we need to redisplay the current status
    if (amp.state.F_alert != amp.state.OF_alert) {
        amp.state.OF_alert = amp.state.F_alert;
        int r_col = GREEN;

        if (amp.state.F_alert == 2) {
            r_col = YELLOW;
        } else if (amp.state.F_alert == 3) {
            r_col = RED;
        }
        amp.lcd[0].fill_rect(20, 34, 25, 10, r_col);
    }

    // reflected alert
    if (r_tot > 590) {
        amp.state.R_alert = 3;
        amp.trip_set();
    } else if (r_tot > 450 && amp.state.R_alert == 1) {
        amp.state.R_alert = 2;
    }

    if (amp.state.R_alert != amp.state.OR_alert) {
        amp.state.OR_alert = amp.state.R_alert;
        unsigned int r_col = GREEN;

        if (amp.state.R_alert == 2)
            r_col = YELLOW;

        if (amp.state.R_alert == 3)
            r_col = RED;

        amp.lcd[0].fill_rect(84, 34, 25, 10, r_col);
    }

    // drive alert
    if (d_tot > 1100) {
        amp.state.D_alert = 3;
    } else if (d_tot > 900 && amp.state.D_alert == 1) {
        amp.state.D_alert = 2;
    }

    if (amp.state.ATTN_ST == 1)
        amp.state.D_alert = 0;

    if (!amp.state.tx_is_on)
        amp.state.D_alert = 0;

    if (amp.state.D_alert != amp.state.OD_alert) {
        amp.state.OD_alert = amp.state.D_alert;

        unsigned int r_col = GREEN;
        if (amp.state.ATTN_ST == 1) {
            r_col = DGRAY;
        } else if (amp.state.D_alert == 2) {
            r_col = YELLOW;

        } else if (amp.state.D_alert == 3) {
            r_col = RED;
            if (amp.state.tx_is_on) {
                amp.trip_set();
            }
        }

        amp.lcd[0].fill_rect(148, 34, 25, 10, r_col);
    }

    // voltage alert
    amp.state.V_alert = 1;

    const int dc_vol = read_voltage();
    if (dc_vol < volts_to_voltage_reading(45) ||
        dc_vol > volts_to_voltage_reading(75)) {
        amp.state.V_alert = 2;
    }

    if (amp.state.V_alert != amp.state.OV_alert) {
        amp.state.OV_alert = amp.state.V_alert;

        unsigned int r_col = GREEN;
        if (amp.state.V_alert == 2) {
            r_col = YELLOW;
        }

        amp.lcd[0].fill_rect(212, 34, 25, 10, r_col);
    }

    // current alert
    int dc_cur = read_current();
    int MC1 = 180 * amp.state.MAX_CUR;
    int MC2 = 200 * amp.state.MAX_CUR;

    if (dc_cur > MC2) {
        amp.state.I_alert = 3;
        amp.trip_set();
    } else if (dc_cur > MC1 && amp.state.I_alert == 1) {
        amp.state.I_alert = 2;
    }


    if (amp.state.I_alert != amp.state.OI_alert) {
        amp.state.OI_alert = amp.state.I_alert;
        unsigned int r_col = GREEN;

        if (amp.state.I_alert == 2) {
            r_col = YELLOW;
        } else if (amp.state.I_alert == 3) {
            r_col = RED;
        }

        amp.lcd[0].fill_rect(276, 34, 25, 10, r_col);
    }
}

constexpr int volts_to_voltage_reading(const int volts) {
    return volts * 40; // voltage reading offers 25mV resolution
}



