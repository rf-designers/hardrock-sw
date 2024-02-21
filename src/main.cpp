/*
Hardrock 500 Firmware
Version 3.5
*/

#include "HR500V1.h"
#include "amp_state.h"
#include "atu_functions.h"
#include "hr500_displays.h"
#include "hr500_sensors.h"
#include "menu_functions.h"
#include "serial_procs.h"
#include <EEPROM.h>
#include <FreqCount.h>
#include <HR500.h>
#include <HR500X.h>
#include <SPI.h>
#include <TimerOne.h>
#include <Wire.h>
#include <stdint.h>

XPT2046_Touchscreen ts1(TP1_CS);
XPT2046_Touchscreen ts2(TP2_CS);

int AnalogRead(byte pin);
void SendLPFRelayData(byte data);
void SendLPFRelayDataSafe(byte data);
void handleACCCommunication();
void handleUSBCommunication();
void updateAlarms();
void updateTemperatureDisplay();
void handleTouchScreen1();
void handleTouchScreen2();
void handleTrxBandDetection();

long M_CORR = 100;
byte FAN_SP = 0;
char ATU_STAT;
char ATTN_P = 0;
byte ATTN_ST = 0;
volatile int timeToTouch = 0; // countdown for TS touching. this gets decremented in timer ISR
byte menu_choice = 0;
unsigned int temp_utp, temp_dtp;
byte menuSEL = 0;
byte Bias_Meter = 0;
int MAX_CUR = 20;

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

amp_state state;

// Enable interrupt on state change of D11 (PTT)
void EnablePTTDetector() {
    PCICR |= (1 << PCIE0);
    PCMSK0 |= (1 << PCINT5); // Set PCINT0 (digital input 11) to trigger an
    // interrupt on state change.
}

// Disable interrupt for state change of D11 (PTT)
void disablePTTDetector() {
    PCMSK0 &= ~(1 << PCINT5);
}

// This interrupt driven function reads data from the wattmeter
void timerISR() {
    if (state.txIsOn) {
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
    dtmp = (dtmp * static_cast<long>(d_lin[state.band])) / static_cast<long>(100);

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
    if (timeToTouch > 0) {
        timeToTouch--;
    }

    // handle reenabling of PTT detection
    if (timeToEnablePTTDetector > 0) {
        timeToEnablePTTDetector--;
        if (timeToEnablePTTDetector == 0 && state.mode != mode_type::standby) {
            EnablePTTDetector();
        }
    }
}

char TEMPbuff[16];
int s_count = 0;

unsigned int F_bar = 9, OF_bar = 9;
char RL_TXT[] = {"-.-"};
char ORL_TXT[] = {"..."};

int t_read;
int s_disp = 19;

char bias_text[] = {"          "};
int old_bias_current = -1;
int otemp = -99;
byte FTband;

void SwitchToTX() {
    state.F_alert = 1;
    state.R_alert = 1;
    state.D_alert = 1;
    state.V_alert = 1;
    state.I_alert = 1;
    state.OF_alert = 0;
    state.OR_alert = 0;
    state.OD_alert = 0;
    state.OV_alert = 0;
    state.OI_alert = 0;

    delay(20);
    RESET_PULSE
    s_disp = 19;

    DrawRxButtons(DGRAY);
    DrawTxPanel(RED);
}

void SwitchToRX() {
    DrawRxButtons(GBLUE);
    DrawTxPanel(GREEN);
    TripClear();
    RESET_PULSE
    Tft.LCD_SEL = 0;
    Tft.lcd_fill_rect(70, 203, 36, 16, MGRAY);
    Tft.drawString((uint8_t*)RL_TXT, 70, 203, 2, LGRAY);
    strcpy(ORL_TXT, "   ");
}

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

// Reads input frequency and sets state.band accordingly
void ReadInputFrequency() {
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

byte getTouchedRectangle(byte touch_screen) {
    uint16_t x, y;
    uint8_t key;
    TS_Point p;

    if (touch_screen == 1)
        p = ts1.getPoint();
    if (touch_screen == 2)
        p = ts2.getPoint();

    x = map(p.x, 3850, 400, 0, 5);
    y = map(p.y, 350, 3700, 0, 4);
    //  z = p.z;
    key = x + 5 * y;
    return key;
}

void setBand() {
    if (state.band > 10)
        return;
    if (state.txIsOn)
        return;

    byte lpfSerialData = 0;
    switch (state.band) {
    case 0:
        lpfSerialData = 0x00;
        ATUPrintln("*B0");
        break;
    case 1:
        lpfSerialData = 0x20;
        ATUPrintln("*B1");
        break;
    case 2:
        lpfSerialData = 0x20;
        ATUPrintln("*B2");
        break;
    case 3:
        lpfSerialData = 0x20;
        ATUPrintln("*B3");
        break;
    case 4:
        lpfSerialData = 0x08;
        ATUPrintln("*B4");
        break;
    case 5:
        lpfSerialData = 0x08;
        ATUPrintln("*B5");
        break;
    case 6:
        lpfSerialData = 0x04;
        ATUPrintln("*B6");
        break;
    case 7:
        lpfSerialData = 0x04;
        ATUPrintln("*B7");
        break;
    case 8:
        lpfSerialData = 0x02;
        ATUPrintln("*B8");
        break;
    case 9:
        lpfSerialData = 0x01;
        ATUPrintln("*B9");
        break;
    case 10:
        lpfSerialData = 0x00;
        ATUPrintln("*B10");
        break;
    }

    if (state.isAtuPresent && !state.isMenuActive) {
        Tft.LCD_SEL = 1;
        Tft.lcd_fill_rect(121, 142, 74, 21, MGRAY);
    }

    state.lpfBoardSerialData = lpfSerialData;
    if (state.band != state.oldBand) {
        SendLPFRelayData(state.lpfBoardSerialData);

        // delete old band (dirty rectangles)
        Tft.LCD_SEL = 1;
        DrawBand(state.oldBand, MGRAY);

        state.oldBand = state.band;

        // draw the new band
        const uint16_t dcolor = state.band == 0 ? RED : ORANGE;
        DrawBand(state.band, dcolor);

        EEPROM.write(eeband, state.band);
        DrawAnt();
    }
}

// speed = [0,3]
void setFanSpeed(byte speed) {
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
        SetupAccSerial(state.accSpeed);
    }

    if (s_xcvr == xic705) {
        state.accSpeed = serial_speed::baud_19200;
        EEPROM.write(eeaccbaud, speedToEEPROM(state.accSpeed));
        SetupAccSerial(state.accSpeed);
    }
}

void SendLPFRelayData(const byte data) {
    RELAY_CS_LOW
    SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE3));
    SPI.transfer(data);
    SPI.endTransaction();
    RELAY_CS_HIGH
}

void SendLPFRelayDataSafe(byte data) {
    noInterrupts();
    SendLPFRelayData(data);
    interrupts();
}

int AnalogRead(byte pin) {
    noInterrupts();
    int a = analogRead(pin);
    interrupts();
    return a;
}

void setup() {
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

    state.band = EEPROM.read(eeband);
    if (state.band > 10)
        state.band = 5;

    state.mode = modeFromEEPROM(EEPROM.read(eemode));

    disablePTTDetector();
    if (state.mode != mode_type::standby) {
        EnablePTTDetector();
    }

    state.isAtuActive = EEPROM.read(eeatub) == 1;

    for (byte i = 1; i < 11; i++) {
        state.antForBand[i] = EEPROM.read(eeantsel + i);

        if (state.antForBand[i] == 1) {
            SEL_ANT1;
        } else if (state.antForBand[i] == 2) {
            SEL_ANT2;
        } else {
            SEL_ANT1;
            state.antForBand[i] = 1;
            EEPROM.write(eeantsel + i, 1);
        }
    }

    state.accSpeed = speedFromEEPROM(EEPROM.read(eeaccbaud));
    SetupAccSerial(state.accSpeed);

    state.usbSpeed = speedFromEEPROM(EEPROM.read(eeusbbaud));
    SetupUSBSerial(state.usbSpeed);

    state.tempInCelsius = EEPROM.read(eecelsius) > 0;

    state.meterSelection = EEPROM.read(eemetsel);
    if (state.meterSelection < 1 || state.meterSelection > 5)
        state.meterSelection = 1;

    state.oldMeterSelection = state.meterSelection;

    state.trxType = EEPROM.read(eexcvr);
    if (state.trxType < 0 || state.trxType > xcvr_max)
        state.trxType = 0;
    strcpy(item_disp[mXCVR], xcvr_disp[state.trxType]);
    setTransceiver(state.trxType);

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
            item_disp[mATTN] = (char*)" ATTENUATOR IN  ";
        } else {
            ATTN_ON_LOW;
            item_disp[mATTN] = (char*)" ATTENUATOR OUT ";
        }
    } else {
        ATTN_P = 0;
        ATTN_ST = 0;
        item_disp[mATTN] = (char*)" NO ATTENUATOR  ";
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
    TripClear();

    Tft.LCD_SEL = 0;
    Tft.lcd_init(GRAY);
    Tft.LCD_SEL = 1;
    Tft.lcd_init(GRAY);

    setFanSpeed(0);
    drawMeter();

    Serial3.begin(19200); // ATU
    detectATU();

    drawHome();
    Tft.LCD_SEL = 0;
    Tft.lcd_fill_rect(20, 34, 25, 10, GREEN);
    Tft.lcd_fill_rect(84, 34, 25, 10, GREEN);

    if (ATTN_ST == 0)
        Tft.lcd_fill_rect(148, 34, 25, 10, GREEN);

    Tft.lcd_fill_rect(212, 34, 25, 10, GREEN);
    Tft.lcd_fill_rect(276, 34, 25, 10, GREEN);

    shouldHandlePttChange = false;

    while (ts1.touched());
    while (ts2.touched());

    setBand();
}

ISR(PCINT0_vect) {
    if (state.mode == mode_type::standby) return; // Mode is STBY
    if (state.isMenuActive) return; // Menu is active
    if (state.band == 0) return; // Band is undefined
    if (state.isAtuTuning) return; // ATU is working

    timeToEnablePTTDetector = 20;
    disablePTTDetector();

    shouldHandlePttChange = true; // signal to main thread

    const auto pttEnabledNow = digitalRead(PTT_DET) == 1;
    if (pttEnabledNow && !state.pttEnabled) {
        state.pttEnabled = true;
        RF_ACTIVE
        SendLPFRelayData(state.lpfBoardSerialData + 0x10);
        BIAS_ON
        state.txIsOn = true;
    } else {
        state.pttEnabled = false;
        BIAS_OFF
        state.txIsOn = false;
        SendLPFRelayData(state.lpfBoardSerialData);
        RF_BYPASS
    }
}

void loop() {
    static int a_count = 0;
    if (++a_count == 10) {
        a_count = 0;
        updateAlarms();
    }

    static int t_count = 0;
    if (++t_count == 3) {
        t_count = 0;

        if (state.meterSelection == 1) {
            unsigned int f_pw = ReadPower(power_type::fwd_p);
            F_bar = constrain(map(f_pw, 0, 500, 19, 300), 10, 309);
        } else if (state.meterSelection == 2) {
            unsigned int r_pw = ReadPower(power_type::rfl_p);
            F_bar = constrain(map(r_pw, 0, 50, 19, 300), 10, 309);
        } else if (state.meterSelection == 3) {
            unsigned int d_pw = ReadPower(power_type::drv_p);
            F_bar = constrain(map(d_pw, 0, 100, 19, 300), 10, 309);
        } else if (state.meterSelection == 4) {
            F_bar = constrain(map(ReadVoltage(), 0, 2400, 19, 299), 19, 309);
        } else {
            // MeterSel == 5
            F_bar = constrain(map(ReadCurrent(), 0, 4000, 19, 299), 19, 309);
        }

        Tft.LCD_SEL = 0;

        while (F_bar != OF_bar) {
            if (OF_bar < F_bar)
                Tft.lcd_draw_v_line(OF_bar++, 101, 12, GREEN);

            if (OF_bar > F_bar)
                Tft.lcd_draw_v_line(--OF_bar, 101, 12, MGRAY);
        }

        if (Bias_Meter == 1) {
            int bias_current = ReadCurrent() * 5;

            if (bias_current != old_bias_current) {
                old_bias_current = bias_current;
                Tft.LCD_SEL = 1;
                Tft.drawString((uint8_t*)bias_text, 65, 80, 2, MGRAY);

                sprintf(bias_text, "  %d mA", bias_current);
                Tft.drawString((uint8_t*)bias_text, 65, 80, 2, WHITE);
            }
        }

        if (s_disp++ == 20 && f_tot > 250 && state.txIsOn) {
            const auto vswr = ReadPower(power_type::vswr);
            s_disp = 0;
            if (vswr > 9)
                sprintf(RL_TXT, "%d.%d", vswr / 10, vswr % 10);

            if (strcmp(ORL_TXT, RL_TXT) != 0) {
                Tft.LCD_SEL = 0;
                Tft.lcd_fill_rect(70, 203, 36, 16, MGRAY);
                Tft.drawString((uint8_t*)RL_TXT, 70, 203, 2, WHITE);
                strcpy(ORL_TXT, RL_TXT);
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

        if (state.pttEnabled) {
            if (state.mode != mode_type::standby) {
                SwitchToTX();
            }

            const auto pttEnabledNow = digitalRead(PTT_DET) == 1;
            if (!pttEnabledNow && state.mode == mode_type::ptt) {
                SwitchToRX();
            }
        } else {
            if (state.mode != mode_type::standby) {
                SwitchToRX();
            }

            const auto pttEnabledNow = digitalRead(PTT_DET) == 1;
            if (pttEnabledNow) {
                SwitchToTX();
            }
        }
    }

    byte sampCOR = digitalRead(COR_DET);
    if (OLD_COR != sampCOR) {
        OLD_COR = sampCOR;

        if (sampCOR == 1) {
            if (state.trxType != xft817 && state.txIsOn) {
                ReadInputFrequency();
            }

            if (state.band != state.oldBand) {
                BIAS_OFF
                RF_BYPASS
                state.txIsOn = false;
                setBand();
            }
        }
    }

    handleTrxBandDetection();
    handleTouchScreen1();
    handleTouchScreen2();

    const auto atuBusy = digitalRead(ATU_BUSY) == 1;
    if (state.isAtuTuning && !atuBusy) {
        TuneEnd();
    }

    handleACCCommunication();
    handleUSBCommunication();
}

void handleACCCommunication() {
    while (Serial2.available()) {
        rxbuff2[uart2Idx] = Serial2.read();
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
    if (state.band == 10) {
        f_yel = 410;
        f_red = 482;
    }

    if (f_tot > f_yel && state.F_alert == 1) {
        state.F_alert = 2;
    }

    if (f_tot > f_red) {
        state.F_alert = 3;
        TripSet();
    }

    if (state.F_alert != state.OF_alert) {
        state.OF_alert = state.F_alert;
        int r_col = GREEN;
        if (state.F_alert == 2)
            r_col = YELLOW;
        if (state.F_alert == 3)
            r_col = RED;
        Tft.LCD_SEL = 0;
        Tft.lcd_fill_rect(20, 34, 25, 10, r_col);
    }

    // reflected alert
    if (r_tot > 450 && state.R_alert == 1) {
        state.R_alert = 2;
    }

    if (r_tot > 590) {
        state.R_alert = 3;
        TripSet();
    }

    if (state.R_alert != state.OR_alert) {
        state.OR_alert = state.R_alert;
        unsigned int r_col = GREEN;
        if (state.R_alert == 2)
            r_col = YELLOW;
        if (state.R_alert == 3)
            r_col = RED;

        Tft.LCD_SEL = 0;
        Tft.lcd_fill_rect(84, 34, 25, 10, r_col);
    }

    // drive alert
    if (d_tot > 900 && state.D_alert == 1) {
        state.D_alert = 2;
    }

    if (d_tot > 1100) {
        state.D_alert = 3;
    }

    if (ATTN_ST == 1)
        state.D_alert = 0;

    if (!state.txIsOn)
        state.D_alert = 0;

    if (state.D_alert != state.OD_alert) {
        state.OD_alert = state.D_alert;
        unsigned int r_col = GREEN;
        if (ATTN_ST == 1)
            r_col = DGRAY;
        if (state.D_alert == 2)
            r_col = YELLOW;
        if (state.D_alert == 3) {
            r_col = RED;
            if (state.txIsOn)
                TripSet();
        }

        Tft.LCD_SEL = 0;
        Tft.lcd_fill_rect(148, 34, 25, 10, r_col);
    }

    // voltage alert
    int dc_vol = ReadVoltage();

    if (dc_vol < 1800)
        state.V_alert = 2;
    else if (dc_vol > 3000)
        state.V_alert = 2;
    else
        state.V_alert = 1;

    if (state.V_alert != state.OV_alert) {
        state.OV_alert = state.V_alert;
        unsigned int r_col = GREEN;

        if (state.V_alert == 2) {
            r_col = YELLOW;
        }

        Tft.LCD_SEL = 0;
        Tft.lcd_fill_rect(212, 34, 25, 10, r_col);
    }

    // current alert
    int dc_cur = ReadCurrent();
    int MC1 = 180 * MAX_CUR;
    int MC2 = 200 * MAX_CUR;

    if (dc_cur > MC1 && state.I_alert == 1) {
        state.I_alert = 2;
    }

    if (dc_cur > MC2) {
        state.I_alert = 3;
        TripSet();
    }

    if (state.I_alert != state.OI_alert) {
        state.OI_alert = state.I_alert;
        unsigned int r_col = GREEN;

        if (state.I_alert == 2)
            r_col = YELLOW;

        if (state.I_alert == 3)
            r_col = RED;

        Tft.LCD_SEL = 0;
        Tft.lcd_fill_rect(276, 34, 25, 10, r_col);
    }

    if (t_ave > temp_utp)
        setFanSpeed(++FAN_SP);
    else if (t_ave < temp_dtp)
        setFanSpeed(--FAN_SP);
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
    if (t_ave > 700 && state.txIsOn) {
        TripSet();
    }

    if (state.tempInCelsius) {
        t_read = t_ave;
    } else {
        t_read = ((t_ave * 9) / 5) + 320;
    }

    t_read /= 10;

    if (t_read != otemp) {
        otemp = t_read;
        Tft.LCD_SEL = 0;
        Tft.drawString((uint8_t*)TEMPbuff, 237, 203, 2, DGRAY);
        if (state.tempInCelsius) {
            sprintf(TEMPbuff, "%d&C", t_read);
        } else {
            sprintf(TEMPbuff, "%d&F", t_read);
        }

        Tft.drawString((uint8_t*)TEMPbuff, 237, 203, 2, t_color);
    }
}

void handleTouchScreen1() {
    if (ts1.touched()) {
        const byte pressedKey = getTouchedRectangle(1);
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
            state.tempInCelsius = !state.tempInCelsius;
            EEPROM.write(eecelsius, state.tempInCelsius ? 1 : 0);
            break;
        }

        if (state.oldMeterSelection != state.meterSelection) {
            DrawButtonUp(state.oldMeterSelection);
            DrawButtonDn(state.meterSelection);
            state.oldMeterSelection = state.meterSelection;
            EEPROM.write(eemetsel, state.meterSelection);
        }

        while (ts1.touched());
    }
}

void handleTouchScreen2() {
    if (ts2.touched() && timeToTouch == 0) {
        const byte pressedKey = getTouchedRectangle(2);
        timeToTouch = 300;

        if (state.isMenuActive) {
            switch (pressedKey) {
            case 0:
            case 1:
                if (menuSEL == 0) {
                    Tft.LCD_SEL = 1;
                    Tft.drawString((uint8_t*)menu_items[menu_choice], 65, 20, 2, MGRAY);
                    Tft.drawString((uint8_t*)item_disp[menu_choice], 65, 80, 2, MGRAY);

                    if (menu_choice-- == 0)
                        menu_choice = menu_max;

                    Tft.drawString((uint8_t*)menu_items[menu_choice], 65, 20, 2, WHITE);
                    Tft.drawString((uint8_t*)item_disp[menu_choice], 65, 80, 2, LGRAY);
                }

                break;

            case 3:
            case 4:
                if (menuSEL == 0) {
                    Tft.LCD_SEL = 1;
                    Tft.drawString((uint8_t*)menu_items[menu_choice], 65, 20, 2, MGRAY);
                    Tft.drawString((uint8_t*)item_disp[menu_choice], 65, 80, 2, MGRAY);

                    if (++menu_choice > menu_max)
                        menu_choice = 0;

                    Tft.drawString((uint8_t*)menu_items[menu_choice], 65, 20, 2, WHITE);
                    Tft.drawString((uint8_t*)item_disp[menu_choice], 65, 80, 2, LGRAY);
                }

                break;

            case 5:
            case 6:
                if (menuSEL == 1)
                    menuUpdate(menu_choice, 0);
                break;

            case 8:
            case 9:
                if (menuSEL == 1)
                    menuUpdate(menu_choice, 1);
                break;

            case 7:
            case 12:
                menuSelect();
                break;

            case 17:
                Tft.LCD_SEL = 1;
                Tft.lcd_clear_screen(GRAY);
                state.isMenuActive = false;

                if (menu_choice == mSETbias) {
                    BIAS_OFF
                    SendLPFRelayDataSafe(state.lpfBoardSerialData);
                    state.mode = state.old_mode;
                    MAX_CUR = 20;
                    Bias_Meter = 0;
                }

                drawHome();
                Tft.LCD_SEL = 0;
                Tft.lcd_reset();
            }
        } else {
            switch (pressedKey) {
            case 5:
            case 6:
                if (!state.txIsOn) {
                    state.mode = nextMode(state.mode);
                    EEPROM.write(eemode, modeToEEPROM(state.mode));
                    DrawMode();
                    disablePTTDetector();

                    if (state.mode == mode_type::ptt) {
                        EnablePTTDetector();
                    }
                }
                break;

            case 8:
                if (!state.txIsOn) {
                    if (++state.band >= 11)
                        state.band = 1;
                    setBand();
                }
                break;

            case 9:
                if (!state.txIsOn) {
                    if (--state.band == 0)
                        state.band = 10;

                    if (state.band == 0xff)
                        state.band = 10;
                    setBand();
                }
                break;

            case 15:
            case 16:
                if (!state.txIsOn) {
                    if (++state.antForBand[state.band] == 3)
                        state.antForBand[state.band] = 1;

                    EEPROM.write(eeantsel + state.band, state.antForBand[state.band]);

                    if (state.antForBand[state.band] == 1) {
                        SEL_ANT1;
                    } else if (state.antForBand[state.band] == 2) {
                        SEL_ANT2;
                    }
                    DrawAnt();
                }
                break;

            case 18:
            case 19:
                if (state.isAtuPresent && !state.txIsOn) {
                    state.isAtuActive = !state.isAtuActive;
                    DrawATU();
                }
                break;

            case 12:
                if (!state.isAtuPresent) {
                    Tft.LCD_SEL = 1;
                    Tft.lcd_clear_screen(GRAY);
                    DrawMenu();
                    state.isMenuActive = true;
                }
                break;

            case 7:
                if (state.isAtuPresent) {
                    Tft.LCD_SEL = 1;
                    Tft.lcd_clear_screen(GRAY);
                    DrawMenu();
                    state.isMenuActive = true;
                }
                break;

            case 17:
                if (state.isAtuPresent) {
                    TuneButtonPressed();
                }
            }
        }

        while (ts2.touched());
    }
}

void handleTrxBandDetection() {
    if (state.txIsOn) return;

    if (state.trxType == xft817) {
        byte nFT817 = FT817det();
        while (nFT817 != FT817det()) {
            nFT817 = FT817det();
        }

        if (nFT817 != 0) {
            FTband = nFT817;
        }

        if (FTband != state.band) {
            state.band = FTband;
            setBand();
        }
    } else if (state.trxType == xxieg) {
        byte nXieg = Xiegudet();
        while (nXieg != Xiegudet()) {
            nXieg = Xiegudet();
        }

        if (nXieg != 0) {
            FTband = nXieg;
        }

        if (FTband != state.band) {
            state.band = FTband;
            setBand();
        }
    } else if (state.trxType == xelad) {
        byte nElad = Eladdet();
        while (nElad != Eladdet()) {
            nElad = Eladdet();
        }

        if (nElad != 0)
            FTband = nElad;

        if (FTband != state.band) {
            state.band = FTband;
            setBand();
        }
    }
}
