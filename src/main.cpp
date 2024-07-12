//#include <SPI.h>
//#include <Adafruit_GFX.h>
//#include "display_board.h"
////#include "Adafruit_ST7789.h"
//#include "../lib/Adafruit_ST7789/Adafruit_ST7789.h"
//
//void setup(void) {
//    Serial.begin(19200);
//
//    SPI.setDataMode(SPI_MODE3);
//    SPI.setBitOrder(MSBFIRST);
//    SPI.setClockDivider(SPI_CLOCK_DIV4);
//    SPI.begin();
//
//    display_board lcdxx{new lcd1};
//    lcdxx.lcd_init(GREEN);
//    lcdxx.fill_rect(10, 10, 200, 200, MAGENTA);
//}
//
//void loop() {
//}

//Hardrock 500 Firmware
//Version 3.5

#include "HR500V1.h"
#include "amp_state.h"
#include "atu_functions.h"
#include "hr500_displays.h"
#include "hr500_sensors.h"
#include "menu_functions.h"
#include "serial_procs.h"
#include "display_board.h"
#include <EEPROM.h>
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

int AnalogRead(byte pin);
void handleACCCommunication();
void handleUSBCommunication();
void updateAlarms();
void updateTemperatureDisplay();
void handleTrxBandDetection();

long M_CORR = 100;
byte FAN_SP = 0;
char ATU_STAT;
char ATTN_P = 0;
byte ATTN_ST = 0;
unsigned int temp_utp, temp_dtp;

int TICK = 0;
int RD_ave = 0, FD_ave = 0;
int FT8CNT = 0;

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

unsigned int t_avg[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
unsigned int t_tot = 0, t_ave;
byte t_i = 0;

volatile byte curCOR;
volatile bool shouldHandlePttChange = false;

// Countdown timer for re-enabling PTT detector interrupt. This gets updated in
// the timer ISR
volatile byte timeToEnablePTTDetector = 0;

byte OLD_COR = 0;


uint8_t uartIdx = 0, uart2Idx = 0;
uint8_t readStart = 0, readStart2 = 0;
char rxbuff[128]; // 128 byte circular Buffer for storing rx data
char workingString[128];
char rxbuff2[128]; // 128 byte circular Buffer for storing rx data
char workingString2[128];

// This interrupt driven function reads data from the wattmeter
void timerISR() {
    if (amp.state.txIsOn) {
        // read forward power
        long s_calc = static_cast<long>(analogRead(12)) * M_CORR;
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
        s_calc = static_cast<long>(analogRead(13)) * M_CORR;
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
    if (amp.state.timeToTouch > 0) {
        amp.state.timeToTouch--;
    }

    // handle reenabling of PTT detection
    if (timeToEnablePTTDetector > 0) {
        timeToEnablePTTDetector--;

        if (timeToEnablePTTDetector == 0 && amp.state.mode != mode_type::standby) {
            amp.enablePTTDetector();
        }
    }
}

char TEMPbuff[16];
int s_count = 0;

unsigned int F_bar = 9, OF_bar = 9;

int t_read;

char bias_text[] = {"          "};
int old_bias_current = -1;
int otemp = -99;
byte FTband;

byte FT817det() {
    const int ftv = AnalogRead(FT817_V);

    if (ftv < 95) return 10;

    if (ftv > 99 && ftv < 160) return 9;

    if (ftv > 164 && ftv < 225) return 7;

    if (ftv > 229 && ftv < 285) return 6;

    if (ftv > 289 && ftv < 345) return 5;

    if (ftv > 349 && ftv < 410) return 4;

    if (ftv > 414 && ftv < 475) return 3;

    if (ftv > 479 && ftv < 508) return 2;

    if (ftv < 605) return 1;

    return 0;
}

byte Eladdet() {
    const int ftv = AnalogRead(FT817_V);

    if (ftv < 118) return 10;

    if (ftv > 130 && ftv < 200) return 9;

    if (ftv > 212 && ftv < 282) return 7;

    if (ftv > 295 && ftv < 365) return 6;

    if (ftv > 380 && ftv < 450) return 5;

    if (ftv > 462 && ftv < 532) return 4;

    if (ftv > 545 && ftv < 615) return 3;

    if (ftv > 630 && ftv < 700) return 2;

    if (ftv < 780) return 1;

    return 0;
}

byte Xiegudet() {
    const int ftv = AnalogRead(FT817_V);

    if (ftv < 136) return 10;

    if (ftv < 191) return 9;

    if (ftv < 246) return 8;

    if (ftv < 300) return 7;

    if (ftv < 355) return 6;

    if (ftv < 410) return 5;

    if (ftv < 465) return 4;

    if (ftv < 520) return 3;

    if (ftv < 574) return 2;

    return 1;
}


// speed = [0,3]
void set_fan_speed(byte speed) {
    switch (speed) {
        case 0:
            digitalWrite(FAN1, LOW);
            digitalWrite(FAN2, LOW);
            temp_utp = 400;
            temp_dtp = 00;
            break;

        case 1:
            digitalWrite(FAN1, HIGH);
            digitalWrite(FAN2, LOW);
            temp_utp = 500;
            temp_dtp = 350;
            break;

        case 2:
            digitalWrite(FAN1, LOW);
            digitalWrite(FAN2, HIGH);
            temp_utp = 600;
            temp_dtp = 450;
            break;

        case 3:
        default:
            digitalWrite(FAN1, HIGH);
            digitalWrite(FAN2, HIGH);
            temp_utp = 2500;
            temp_dtp = 550;
    }
}

void setTransceiver(byte s_xcvr) {
    if (s_xcvr == xhobby || s_xcvr == xkx23)
        S_POL_REV
    else
        S_POL_NORM

    if (s_xcvr == xhobby) {
        pinMode(TTL_PU, OUTPUT);
        digitalWrite(TTL_PU, HIGH);
    } else {
        digitalWrite(TTL_PU, LOW);
        pinMode(TTL_PU, INPUT);
    }

    if (s_xcvr == xft817 || s_xcvr == xelad || s_xcvr == xxieg) {
        strcpy(item_disp[mACCbaud], "  XCVR MODE ON  ");
        Serial2.end();
    } else {
        SetupAccSerial(amp.state.accSpeed);
    }

    if (s_xcvr == xic705) {
        amp.state.accSpeed = serial_speed::baud_19200;
        EEPROM.write(eeaccbaud, speedToEEPROM(amp.state.accSpeed));
        SetupAccSerial(amp.state.accSpeed);
    }
}

int AnalogRead(byte pin) {
    noInterrupts();
    int a = analogRead(pin);
    interrupts();
    return a;
}


void wakeUpFromSleep() {
    // Briefly disable sleep mode to allow further processing
    sleep_disable();
    EIFR |= 0b11111111; // clear interrupt flags so that we can have multiple subsequent touches
}

void goToSleep() {
    // Ensure the BOD (Brown-out Detector) is disabled in sleep
    MCUCR = (1 << BODS) | (1 << BODSE);
    MCUCR = (1 << BODS);

    sleep_enable();
    sleep_mode();
}

void configureWakeUpPins() {
    noInterrupts();

    pinMode(18, INPUT);
    attachInterrupt(digitalPinToInterrupt(18), wakeUpFromSleep, FALLING);

    pinMode(19, INPUT);
    attachInterrupt(digitalPinToInterrupt(19), wakeUpFromSleep, FALLING);

    // set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    set_sleep_mode(SLEEP_MODE_IDLE);

    interrupts();
}

void configureWatchDogTimer() {

    // Clear the reset flag
    MCUSR &= ~(1 << WDRF);

    wdt_reset();
    // Configure the watchdog timer to interrupt mode only (no reset)
    WDTCSR |= (1 << WDCE) | (1 << WDE);
    WDTCSR = (1 << WDIE) | (1 << WDP2) | (1 << WDP1); // 1second


    // Set the watchdog timer to wake up every second
    // wdt_enable(WDTO_1S);
}

unsigned long delta;

void test() {
//    SPI.begin();
//    Serial.begin(19200);
//
//    display_board <lcd1> l1;
//    l1.lcd_init(GREEN);
//    l1.lcd_draw_point(160, 100, RED);
//    auto start = millis();
//    l1.lcd_display_char(10, 10, 'A', FONT_1608, RED);
//    delta = millis() - start;
//    l1.draw_char('A', 30, 10, 6, WHITE);
//
//
//    display_board <lcd2> l2;
//    l2.lcd_init(RED);
//    l2.lcd_draw_point(160, 100, GREEN);
//    l2.lcd_display_char(10, 10, 'A', FONT_1608, GREEN);
//    l2.draw_char('A', 30, 10, 6, WHITE);
}


display_board lcd[2] = {display_board{new lcd1}, display_board{new lcd2}};

void setup() {
//    test();
//    return;

    amp.setup();
    amp.state.band = EEPROM.read(eeband);
    if (amp.state.band > 10)
        amp.state.band = 5;

    amp.state.mode = modeFromEEPROM(EEPROM.read(eemode));

    amp.disablePTTDetector();
    if (amp.state.mode != mode_type::standby) {
        amp.enablePTTDetector();
    }

    amp.atu.setActive(EEPROM.read(eeatub) == 1);
    for (byte i = 1; i < 11; i++) {
        amp.state.antForBand[i] = EEPROM.read(eeantsel + i);

        if (amp.state.antForBand[i] == 1) {
            // SEL_ANT1;
        } else if (amp.state.antForBand[i] == 2) {
            // SEL_ANT2;
        } else {
            // SEL_ANT1;
            amp.state.antForBand[i] = 1;
            EEPROM.write(eeantsel + i, 1);
        }
    }

    amp.state.accSpeed = speedFromEEPROM(EEPROM.read(eeaccbaud));
    SetupAccSerial(amp.state.accSpeed);

    amp.state.usbSpeed = speedFromEEPROM(EEPROM.read(eeusbbaud));
    SetupUSBSerial(amp.state.usbSpeed);

    amp.state.tempInCelsius = EEPROM.read(eecelsius) > 0;

    amp.state.meterSelection = EEPROM.read(eemetsel);

    if (amp.state.meterSelection < 1 || amp.state.meterSelection > 5)
        amp.state.meterSelection = 1;

    amp.state.oldMeterSelection = amp.state.meterSelection;

    amp.state.trxType = EEPROM.read(eexcvr);

    if (amp.state.trxType < 0 || amp.state.trxType > xcvr_max)
        amp.state.trxType = 0;

    strcpy(item_disp[mXCVR], xcvr_disp[amp.state.trxType]);
    setTransceiver(amp.state.trxType);

    byte MCAL = EEPROM.read(eemcal);

    if (MCAL < 75 || MCAL > 125)
        MCAL = 100;

    M_CORR = static_cast<long>(MCAL);
    sprintf(item_disp[mMCAL], "      %3li       ", M_CORR);

    if (ATTN_INST_READ == LOW) {
        ATTN_P = 1;
        ATTN_ST = EEPROM.read(eeattn);

        if (ATTN_ST > 1)
            ATTN_ST = 0;

        if (ATTN_ST == 1) {
            ATTN_ON_HIGH;
            item_disp[mATTN] = (char *) " ATTENUATOR IN  ";
        } else {
            ATTN_ON_LOW;
            item_disp[mATTN] = (char *) " ATTENUATOR OUT ";
        }
    } else {
        ATTN_P = 0;
        ATTN_ST = 0;
        item_disp[mATTN] = (char *) " NO ATTENUATOR  ";
    }

    analogReference(EXTERNAL);
    const unsigned char PS_32 = (1 << ADPS2) | (1 << ADPS0);
    const unsigned char PS_128 = (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
    ADCSRA &= ~PS_128;
    ADCSRA |= PS_32;

    Timer1.initialize(1000);
    Timer1.attachInterrupt(timerISR);
    interrupts();

    SPI.begin();
    Wire.begin();
    Wire.setClock(400000);
    amp.trip_clear();

    lcd[0].lcd_init(GRAY);
    lcd[1].lcd_init(GRAY);

    set_fan_speed(0);
    draw_meter();

    Serial3.begin(19200); // ATU
    amp.atu.detect();

    draw_home();
    lcd[0].fill_rect(20, 34, 25, 10, GREEN);
    lcd[0].fill_rect(84, 34, 25, 10, GREEN);

    if (ATTN_ST == 0)
        lcd[0].fill_rect(148, 34, 25, 10, GREEN);

    lcd[0].fill_rect(212, 34, 25, 10, GREEN);
    lcd[0].fill_rect(276, 34, 25, 10, GREEN);

    shouldHandlePttChange = false;

    while (amp.ts1.touched());
    while (amp.ts2.touched());

    amp.setBand();

    configureWakeUpPins();
    configureWatchDogTimer();

    // Attach interrupts to RX pins
    attachInterrupt(digitalPinToInterrupt(0), serialRxISR, FALLING); // RX for Serial
    attachInterrupt(digitalPinToInterrupt(17), serial2RxISR, FALLING); // RX for Serial2
}

ISR(PCINT0_vect) {
    sleep_disable();

    if (amp.state.mode == mode_type::standby) return; // Mode is STBY
    if (amp.state.isMenuActive) return; // Menu is active
    if (amp.state.band == 0) return; // Band is undefined
    if (amp.atu.isTuning()) return; // ATU is working

    // timeToEnablePTTDetector = 20;
    // amp.disablePTTDetector();

    shouldHandlePttChange = true; // signal to main thread

    const auto pttEnabledNow = digitalRead(PTT_DET) == 1;

    if (pttEnabledNow && !amp.state.pttEnabled) {
        amp.state.pttEnabled = true;
        RF_ACTIVE
        amp.lpf.sendRelayData(amp.lpf.serialData + 0x10);
        BIAS_ON
        amp.state.txIsOn = true;
    } else {
        amp.state.pttEnabled = false;
        BIAS_OFF
        amp.state.txIsOn = false;
        amp.lpf.sendRelayData(amp.lpf.serialData);
        RF_BYPASS
    }
}

void loop() {
//    Serial.print("Delta: ");
//    Serial.print(delta);
//    Serial.println();
//    delay(1000);
//    return;

    // if (Serial2.available()) {
    //     Serial.write(Serial2.read());
    // }
    // return;

    static int a_count = 0;

    if (++a_count == 10) {
        a_count = 0;
        updateAlarms();
    }

    static int t_count = 0;

    if (++t_count == 3) {
        t_count = 0;

        if (amp.state.meterSelection == 1) {
            unsigned int f_pw = read_power(power_type::fwd_p);
            F_bar = constrain(map(f_pw, 0, 500, 19, 300), 10, 309);
        } else if (amp.state.meterSelection == 2) {
            unsigned int r_pw = read_power(power_type::rfl_p);
            F_bar = constrain(map(r_pw, 0, 50, 19, 300), 10, 309);
        } else if (amp.state.meterSelection == 3) {
            unsigned int d_pw = read_power(power_type::drv_p);
            F_bar = constrain(map(d_pw, 0, 100, 19, 300), 10, 309);
        } else if (amp.state.meterSelection == 4) {
            F_bar = constrain(map(read_voltage(), 0, 2400, 19, 299), 19, 309);
        } else {
            // MeterSel == 5
            F_bar = constrain(map(read_current(), 0, 4000, 19, 299), 19, 309);
        }

        while (F_bar != OF_bar) {
            if (OF_bar < F_bar)
                lcd[0].draw_v_line(OF_bar++, 101, 12, GREEN);

            if (OF_bar > F_bar)
                lcd[0].draw_v_line(--OF_bar, 101, 12, MGRAY);
        }

        if (amp.state.biasMeter) {
            const int bias_current = read_current() * 5;

            if (bias_current != old_bias_current) {
                old_bias_current = bias_current;
                lcd[1].draw_string(bias_text, 65, 80, 2, MGRAY);

                sprintf(bias_text, "  %d mA", bias_current);
                lcd[1].draw_string(bias_text, 65, 80, 2, WHITE);
            }
        }

        if (amp.state.s_disp++ == 20 && f_tot > 250 && amp.state.txIsOn) {
            const auto vswr = read_power(power_type::vswr);
            amp.state.s_disp = 0;

            if (vswr > 9)
                sprintf(amp.state.RL_TXT, "%d.%d", vswr / 10, vswr % 10);

            if (strcmp(amp.state.ORL_TXT, amp.state.RL_TXT) != 0) {
                lcd[0].fill_rect(70, 203, 36, 16, MGRAY);
                lcd[0].draw_string(amp.state.RL_TXT, 70, 203, 2, WHITE);
                strcpy(amp.state.ORL_TXT, amp.state.RL_TXT);
            }
        }

        // temperature display
        static int t_disp = 0;

        if (t_disp++ == 200) {
            t_disp = 0;
            updateTemperatureDisplay();
        }
    }

    if (shouldHandlePttChange) {
        shouldHandlePttChange = false;

        if (amp.state.pttEnabled) {
            if (amp.state.mode != mode_type::standby) {
                amp.switchToTX();
            }

            const auto pttEnabledNow = digitalRead(PTT_DET) == 1;

            if (!pttEnabledNow && amp.state.mode == mode_type::ptt) {
                amp.switchToRX();
            }
        } else {
            if (amp.state.mode != mode_type::standby) {
                amp.switchToRX();
            }

            const auto pttEnabledNow = digitalRead(PTT_DET) == 1;

            if (pttEnabledNow) {
                amp.switchToTX();
            }
        }
    }

    byte sampCOR = digitalRead(COR_DET);

    if (OLD_COR != sampCOR) {
        OLD_COR = sampCOR;

        if (sampCOR == 1) {
            if (amp.state.trxType != xft817 && amp.state.txIsOn) {
                amp.readInputFrequency();
            }

            if (amp.state.band != amp.state.oldBand) {
                BIAS_OFF
                RF_BYPASS
                amp.state.txIsOn = false;
                amp.setBand();
            }
        }
    }

    handleTrxBandDetection();
    amp.handleTouchScreen1();
    amp.handleTouchScreen2();

    const auto atuBusy = digitalRead(ATU_BUSY) == 1;

    if (amp.atu.isTuning() && !atuBusy) {
        TuneEnd();
    }

    handleACCCommunication();
    handleUSBCommunication();

//    if (!amp.state.txIsOn) {
//        goToSleep();
//    }
}

void handleACCCommunication() {
    while (Serial2.available()) {
        rxbuff2[uart2Idx] = Serial2.read();

        // UART_send(1, "received data:");
        // UART_send_num(1, rxbuff2[uart2Idx]);
        // UART_send_line(1);~
        // UART_send_cr(1);
        Serial.write(rxbuff2[uart2Idx]);

        if (rxbuff2[uart2Idx] == ';') {
            uartGrabBuffer2();
            handleSerialMessage(2);
        }

        if (++uart2Idx > 127)
            uart2Idx = 0;
    }
}

void handleUSBCommunication() {
    while (Serial.available()) {
        rxbuff[uartIdx] = Serial.read(); // Storing read data

        if (rxbuff[uartIdx] == ';') {
            uartGrabBuffer();
            handleSerialMessage(1);
        }

        if (++uartIdx > 127)
            uartIdx = 0;
    }
}

void updateAlarms() {
    // forward alert
    unsigned int f_yel = 600, f_red = 660;

    if (amp.state.band == 10) {
        f_yel = 410;
        f_red = 482;
    }

    if (f_tot > f_yel && amp.state.F_alert == 1) {
        amp.state.F_alert = 2;
    }

    if (f_tot > f_red) {
        amp.state.F_alert = 3;
        amp.tripSet();
    }

    if (amp.state.F_alert != amp.state.OF_alert) {
        amp.state.OF_alert = amp.state.F_alert;
        int r_col = GREEN;

        if (amp.state.F_alert == 2)
            r_col = YELLOW;

        if (amp.state.F_alert == 3)
            r_col = RED;

        lcd[0].fill_rect(20, 34, 25, 10, r_col);
    }

    // reflected alert
    if (r_tot > 450 && amp.state.R_alert == 1) {
        amp.state.R_alert = 2;
    }

    if (r_tot > 590) {
        amp.state.R_alert = 3;
        amp.tripSet();
    }

    if (amp.state.R_alert != amp.state.OR_alert) {
        amp.state.OR_alert = amp.state.R_alert;
        unsigned int r_col = GREEN;

        if (amp.state.R_alert == 2)
            r_col = YELLOW;

        if (amp.state.R_alert == 3)
            r_col = RED;

        lcd[0].fill_rect(84, 34, 25, 10, r_col);
    }

    // drive alert
    if (d_tot > 900 && amp.state.D_alert == 1) {
        amp.state.D_alert = 2;
    }

    if (d_tot > 1100) {
        amp.state.D_alert = 3;
    }

    if (ATTN_ST == 1)
        amp.state.D_alert = 0;

    if (!amp.state.txIsOn)
        amp.state.D_alert = 0;

    if (amp.state.D_alert != amp.state.OD_alert) {
        amp.state.OD_alert = amp.state.D_alert;
        unsigned int r_col = GREEN;

        if (ATTN_ST == 1)
            r_col = DGRAY;

        if (amp.state.D_alert == 2)
            r_col = YELLOW;

        if (amp.state.D_alert == 3) {
            r_col = RED;

            if (amp.state.txIsOn)
                amp.tripSet();
        }

        lcd[0].fill_rect(148, 34, 25, 10, r_col);
    }

    // voltage alert
    const int dc_vol = read_voltage();
    amp.state.V_alert = 1;

    if (dc_vol < 1800 || dc_vol > 3000) {
        amp.state.V_alert = 2;
    }

    if (amp.state.V_alert != amp.state.OV_alert) {
        amp.state.OV_alert = amp.state.V_alert;

        unsigned int r_col = GREEN;

        if (amp.state.V_alert == 2) {
            r_col = YELLOW;
        }

        lcd[0].fill_rect(212, 34, 25, 10, r_col);
    }

    // current alert
    int dc_cur = read_current();
    int MC1 = 180 * amp.state.MAX_CUR;
    int MC2 = 200 * amp.state.MAX_CUR;

    if (dc_cur > MC1 && amp.state.I_alert == 1) {
        amp.state.I_alert = 2;
    }

    if (dc_cur > MC2) {
        amp.state.I_alert = 3;
        amp.tripSet();
    }

    if (amp.state.I_alert != amp.state.OI_alert) {
        amp.state.OI_alert = amp.state.I_alert;
        unsigned int r_col = GREEN;

        if (amp.state.I_alert == 2)
            r_col = YELLOW;

        if (amp.state.I_alert == 3)
            r_col = RED;

        lcd[0].fill_rect(276, 34, 25, 10, r_col);
    }

    if (t_ave > temp_utp)
        set_fan_speed(++FAN_SP);
    else if (t_ave < temp_dtp)
        set_fan_speed(--FAN_SP);
}

void updateTemperatureDisplay() {
    t_avg[t_i] = constrain(AnalogRead(14), 5, 2000);
    t_tot += t_avg[t_i++]; // Add in the new sample

    if (t_i > 10)
        t_i = 0; // Update the index value

    t_tot -= t_avg[t_i]; // Subtract off the 51st value
    t_ave = t_tot / 5;

    unsigned int t_color = GREEN;

    if (t_ave > 500)
        t_color = YELLOW;

    if (t_ave > 650)
        t_color = RED;

    if (t_ave > 700 && amp.state.txIsOn) {
        amp.tripSet();
    }

    if (amp.state.tempInCelsius) {
        t_read = t_ave;
    } else {
        t_read = ((t_ave * 9) / 5) + 320;
    }

    t_read /= 10;

    if (t_read != otemp) {
        otemp = t_read;
        lcd[0].draw_string(TEMPbuff, 237, 203, 2, DGRAY);

        if (amp.state.tempInCelsius) {
            sprintf(TEMPbuff, "%d&C", t_read);
        } else {
            sprintf(TEMPbuff, "%d&F", t_read);
        }

        lcd[0].draw_string(TEMPbuff, 237, 203, 2, t_color);
    }
}


void handleTrxBandDetection() {
    if (amp.state.txIsOn) return;

    if (amp.state.trxType == xft817) {
        byte nFT817 = FT817det();

        while (nFT817 != FT817det()) {
            nFT817 = FT817det();
        }

        if (nFT817 != 0) {
            FTband = nFT817;
        }

        if (FTband != amp.state.band) {
            amp.state.band = FTband;
            amp.setBand();
        }
    } else if (amp.state.trxType == xxieg) {
        byte nXieg = Xiegudet();

        while (nXieg != Xiegudet()) {
            nXieg = Xiegudet();
        }

        if (nXieg != 0) {
            FTband = nXieg;
        }

        if (FTband != amp.state.band) {
            amp.state.band = FTband;
            amp.setBand();
        }
    } else if (amp.state.trxType == xelad) {
        byte nElad = Eladdet();

        while (nElad != Eladdet()) {
            nElad = Eladdet();
        }

        if (nElad != 0)
            FTband = nElad;

        if (FTband != amp.state.band) {
            amp.state.band = FTband;
            amp.setBand();
        }
    }
}
