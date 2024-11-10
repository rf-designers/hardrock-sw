//Hardrock 500 Firmware
//Version 3.5

#include "HR500V1.h"
#include "amp_state.h"
#include "atu_functions.h"
#include "hr500_displays.h"
#include "hr500_sensors.h"
#include "transceivers.h"
#include <HR500.h>
#include <menu_functions.h>
#include <TimerOne.h>
#include <Wire.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/io.h>
#include <avr/power.h>
#include "amp_view.h"

#ifndef BODS
#define BODS 7
#endif

#ifndef BODSE
#define BODSE 2
#endif


ISR(WDT_vect) {
    wdt_reset();
    WDTCSR |= 1 << WDIE; // after each interrupt changes mode to System Reset Mode and that's why needs to be re-enabled
}

amplifier amp;
amp_view view;

int analog_read(byte pin);
extern void handle_acc_comms();
extern void handle_usb_comms();

constexpr int volts_to_voltage_reading(const int volts);
char ATU_STAT;

moving_average<unsigned int, 25> fwd_pwr;
volatile bool should_handle_ptt_change = false;

// Countdown timer for re-enabling PTT detector interrupt. This gets updated in
// the timer ISR
volatile byte timeToEnablePTTDetector = 0;

byte OLD_COR = 0;


// This interrupt driven function reads data from the wattmeter
void timer_isr() {
    sleep_disable();

    if (amp.state.tx_is_on) {
        // read forward power
        long s_calc = static_cast<long>(analogRead(12)) * amp.state.M_CORR;
        fwd_pwr.add(s_calc / static_cast<long>(100));

        const unsigned int ftmp = fwd_pwr.get();

        if (ftmp > amp.state.fwd_tot) {
            amp.state.fwd_tot = ftmp;
            amp.state.fwd_pep = 75;
        } else {
            if (amp.state.fwd_pep > 0)
                amp.state.fwd_pep--;
            else if (amp.state.fwd_tot > 0)
                amp.state.fwd_tot--;
        }

        // read reflected power
        s_calc = static_cast<long>(analogRead(13)) * amp.state.M_CORR;
        const unsigned int rtmp = s_calc / static_cast<long>(100);

        if (rtmp > amp.state.rfl_tot) {
            amp.state.rfl_tot = rtmp;
            amp.state.rfl_pep = 75;
        } else {
            if (amp.state.rfl_pep > 0)
                amp.state.rfl_pep--;
            else if (amp.state.rfl_tot > 0)
                amp.state.rfl_tot--;
        }
    } else {
        amp.state.fwd_tot = 0;
        amp.state.rfl_tot = 0;
    }

    // read drive power
    long dtmp = analogRead(15);
    dtmp = (dtmp * static_cast<long>(d_lin[amp.state.band])) / static_cast<long>(100);

    if (dtmp > amp.state.drv_tot) {
        amp.state.drv_tot = dtmp;
        amp.state.drv_pep = 75;
    } else {
        if (amp.state.drv_pep > 0)
            amp.state.drv_pep--;
        else if (amp.state.drv_tot > 0)
            amp.state.drv_tot--;
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
    // disable sleep mode to allow further processing
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

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
//    set_sleep_mode(SLEEP_MODE_ADC);
//    set_sleep_mode(SLEEP_MODE_PWR_SAVE);

    interrupts();
}

void config_watchdog_timer() {
    // Clear the watchdog timer flag
    MCUSR &= ~(1 << WDRF);

    wdt_reset();

    // Configure the watchdog timer to interrupt mode only (no reset)
    // Set the watchdog timer to wake up every second
//    WDTCSR |= (1 << WDCE) | (1 << WDE);
//    WDTCSR = (1 << WDIE) | (1 << WDP2) | (1 << WDP1); // 1second

    // Enable the watchdog timer in interrupt mode
//    WDTCSR |= (1 << WDCE) | (1 << WDE);
    // Set the desired prescaler (adjust for 250ms timeout)
//    WDTCSR = (1 << WDIE) | (1 << WDP3) | (1 << WDP0);

    wdt_enable(WDTO_250MS);
    WDTCSR |= 1 << WDIE; // after each interrupt changes mode to System Reset Mode and that's why needs to be re-enabled
}


void setup() {
    view.amp = &amp;
    amp.setup();
    amp.load_eeprom_config();
    amp.configure_attenuator();
    amp.configure_adc();

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

    amp.lcd[0].lcd_init(amp.state.colors.named.bg);
    amp.lcd[1].lcd_init(amp.state.colors.named.bg);

    amp.set_fan_speed(0);
//    draw_meter();

    amp.atu.detect();

//    draw_home();
    view.refresh();

    amp.lcd[0].fill_rect(20, 34, 25, 10, amp.state.colors.named.alarm[1]);
    amp.lcd[0].fill_rect(84, 34, 25, 10, amp.state.colors.named.alarm[1]);

    if (!amp.state.attenuator_enabled)
        amp.lcd[0].fill_rect(148, 34, 25, 10, amp.state.colors.named.alarm[1]);

    amp.lcd[0].fill_rect(212, 34, 25, 10, amp.state.colors.named.alarm[1]);
    amp.lcd[0].fill_rect(276, 34, 25, 10, amp.state.colors.named.alarm[1]);

    should_handle_ptt_change = false;

    while (amp.ts1.touched());
    while (amp.ts2.touched());

    amp.set_band();

    config_wakeup_pins();
    config_watchdog_timer();

    // Attach interrupts to RX pins
//    attachInterrupt(digitalPinToInterrupt(0), serialRxISR, FALLING); // RX for Serial
//    attachInterrupt(digitalPinToInterrupt(17), serial2RxISR, FALLING); // RX for Serial2

    // Enable pin change interrupts for PCINT0 group (includes pins D0-D7)
//    PCICR |= (1 << PCIE0);
    // Enable pin change interrupt for pin 0 (PCINT0)
//    PCMSK0 |= (1 << PCINT0);

//    power_adc_disable();
//    power_spi_disable();
//    power_usart0_disable();
//    power_usart2_disable();
//    power_timer1_disable();
//    power_timer2_disable();
//    power_timer3_disable();
//    power_timer4_disable();
//    power_timer5_disable();
//    power_twi_disable();
    pinMode(9, OUTPUT);
    digitalWrite(9, LOW);


    amp.set_transceiver(xkx23);
    setup_acc_serial(serial_speed::baud_38400);
}

// ISR(PCINT0_vect) {
//     sleep_disable();
//     digitalWrite(9, HIGH);
//
// //    handle_usb_comms();
//
//     if (amp.state.mode == mode_type::standby) return; // Mode is STBY
//     if (amp.state.is_menu_active) return; // Menu is active
//     if (amp.state.band == 0) return; // Band is undefined
//     if (amp.atu.is_tuning()) return; // ATU is working
//
//     // timeToEnablePTTDetector = 20;
//     // amp.disablePTTDetector();
//
//     should_handle_ptt_change = true; // signal to main thread
//     const auto ptt_enabled_now = digitalRead(PTT_DET) == 1;
//     if (ptt_enabled_now && !amp.state.ptt_enabled) {
//         amp.state.ptt_enabled = true;
//         RF_ACTIVE
//         amp.lpf.send_relay_data(amp.lpf.serial_data + 0x10);
//         BIAS_ON
//         amp.state.tx_is_on = true;
//     } else {
//         amp.state.ptt_enabled = false;
//         BIAS_OFF
//         amp.state.tx_is_on = false;
//         amp.lpf.send_relay_data(amp.lpf.serial_data);
//         RF_BYPASS
//     }
// }
//

// ISR(PCINT2_vect) {
//     sleep_disable();
//
// //    handle_usb_comms();
//
//     if (amp.state.mode == mode_type::standby) return; // Mode is STBY
//     if (amp.state.is_menu_active) return; // Menu is active
//     if (amp.state.band == 0) return; // Band is undefined
//     if (amp.atu.is_tuning()) return; // ATU is working
//
//     // timeToEnablePTTDetector = 20;
//     // amp.disablePTTDetector();
//
//     should_handle_ptt_change = true; // signal to main thread
//     const auto ptt_enabled_now = digitalRead(PTT_DET) == 1;
//     if (ptt_enabled_now && !amp.state.ptt_enabled) {
//         amp.state.ptt_enabled = true;
//         RF_ACTIVE
//         amp.lpf.send_relay_data(amp.lpf.serial_data + 0x10);
//         BIAS_ON
//         amp.state.tx_is_on = true;
//     } else {
//         amp.state.ptt_enabled = false;
//         BIAS_OFF
//         amp.state.tx_is_on = false;
//         amp.lpf.send_relay_data(amp.lpf.serial_data);
//         RF_BYPASS
//     }
// }


// Pin change interrupt, currently used by D11 which is the PTT signal
ISR(PCINT0_vect) {
    sleep_disable();

//    handle_usb_comms();

    if (amp.state.mode == mode_type::standby) return; // Mode is STBY
    if (amp.state.is_menu_active) return; // Menu is active
    if (amp.state.band == 0) return; // Band is undefined
    if (amp.atu.is_tuning()) return; // ATU is working

    // timeToEnablePTTDetector = 20;
    // amp.disablePTTDetector();

    should_handle_ptt_change = true; // signal to main thread
    const auto ptt_enabled_now = digitalRead(PTT_DET) == 1;
    if (ptt_enabled_now && !amp.state.ptt_enabled) {
        amp.state.ptt_enabled = true;
        RF_ACTIVE
        amp.lpf.send_relay_data(amp.lpf.serial_data + 0x10);
        BIAS_ON
        amp.state.tx_is_on = true;
    } else {
        amp.state.ptt_enabled = false;
        BIAS_OFF
        amp.state.tx_is_on = false;
        amp.lpf.send_relay_data(amp.lpf.serial_data);
        RF_BYPASS
    }
}

void loop() {
//    static int a_count = 0;
//    if (++a_count == 10) {
//        a_count = 0;
    amp.ltc->update();

    amp.update_alerts();
    amp.update_fan_speed();
//    }

//    static int t_count = 0;
//    if (++t_count == 3) {
//        t_count = 0;

    amp.update_meter_drawing();
    if (amp.state.biasMeter) {
        amp.update_bias_reading();
    }

    if (amp.state.swr_display_counter++ == 20 && amp.state.fwd_tot > 250 && amp.state.tx_is_on) {
        amp.state.swr_display_counter = 0;
        amp.update_swr();
    }

//        static int temperature_display_counter = 0;
//        if (temperature_display_counter++ == 200) {
//            temperature_display_counter = 0;
    amp.update_temperature();
//        }
//    }

    if (should_handle_ptt_change) {
        should_handle_ptt_change = false;

        const auto ptt_enabled_now = digitalRead(PTT_DET) == 1;
        if (amp.state.ptt_enabled) {
            if (amp.state.mode != mode_type::standby) {
                amp.switch_to_tx();
            }

            if (!ptt_enabled_now && amp.state.mode == mode_type::ptt) {
                amp.switch_to_rx();
            }
        } else {
            if (amp.state.mode != mode_type::standby) {
                amp.switch_to_rx();
            }
            if (ptt_enabled_now) {
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

//    if (amp.state.touch_enable_counter == 0) {
    amp.handle_ts1();
    amp.handle_ts2();
//        amp.state.touch_enable_counter = 300;
//    }

    const auto atuBusy = digitalRead(ATU_BUSY) == 1;
    if (amp.atu.is_tuning() && !atuBusy) {
        on_tune_end();
    }

    handle_acc_comms();
    handle_usb_comms();

    view.refresh();

    if (!amp.state.tx_is_on) {
        digitalWrite(9, LOW);
        go_to_sleep();
    }
}
