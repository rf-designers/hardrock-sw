/*
Hardrock 500 Firmware
Version 3.5
*/

#include <stdint.h>
#include <HR500.h>
#include <HR500X.h>
#include "HR500V1.h"
#include <SPI.h>
#include <FreqCount.h>
#include <EEPROM.h>
#include <TimerOne.h>
#include <Wire.h>
#include "atu_functions.h"
#include "hr500_displays.h"
#include "hr500_sensors.h"
#include "serial_procs.h"
#include "menu_functions.h"
#include "amp_state.h"

const unsigned char PS_32 = (1 << ADPS2) | (1 << ADPS0);
const unsigned char PS_128 = (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

XPT2046_Touchscreen ts1(TP1_CS);
XPT2046_Touchscreen ts2(TP2_CS);

int AnalogRead(byte pin);

void SendLPFRelayData(byte data);

void SendLPFRelayDataSafe(byte data);

long M_CORR = 100;

byte FAN_SP = 0;
char ATU_STAT;
char ATTN_P = 0;
byte ATTN_ST = 0;
byte OBAND = 0;
volatile int timeToTouch = 0; // countdown for TS touching. this gets decremented in timer ISR
byte ACC_Baud;
byte USB_Baud;
byte menu_choice = 0;
byte OMeterSel;
unsigned int temp_utp, temp_dtp;
byte menuSEL = 0;
byte Bias_Meter = 0;
int MAX_CUR = 20;

byte oMODE;
int TICK = 0;
int RD_ave = 0, FD_ave = 0;
int FT8CNT = 0;

volatile unsigned int f_tot = 0, f_ave = 0;
volatile unsigned int f_avg[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
volatile byte f_i = 0;
volatile unsigned int r_tot = 0;
volatile unsigned int d_tot = 0;

volatile unsigned int f_pep = 0;
volatile unsigned int r_pep = 0;
volatile unsigned int d_pep = 0;

unsigned int t_avg[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
unsigned int t_tot = 0, t_ave;
byte t_i = 0;

volatile byte curCOR, curPTT;
volatile byte flagPTT = 0;
volatile byte PTT = 0;
volatile byte timeToEnablePTTDetector = 0;
// Countdown timer for re-enabling PTT detector interrupt. This gets updated in the timer ISR
volatile int RL_off = 0;
byte OLD_COR = 0;

byte I_alert = 0, V_alert = 0, F_alert = 0, R_alert = 0, D_alert = 0;
byte OI_alert = 0, OV_alert = 0, OF_alert = 0, OR_alert = 0, OD_alert = 0;

byte ADCvinMSB, ADCvinLSB, curSenseMSB, curSenseLSB;
unsigned int ADCvin, ADCcur;

unsigned int uartPtr = 0, uartPtr2 = 0;
unsigned int uartMsgs = 0, uartMsgs2 = 0, readStart = 0, readStart2 = 0;
char rxbuff[128]; // 128 byte circular Buffer for storing rx data
char workingString[128];
char rxbuff2[128]; // 128 byte circular Buffer for storing rx data
char workingString2[128];

char ATU_buff[40], ATU_cbuf[32], ATU_ver[8];
amp_state state;


// Enable interrupt on state change of D11 (PTT)
void EnablePTTDetector() {
    PCICR |= (1 << PCIE0);
    PCMSK0 |= (1 << PCINT5); //Set PCINT0 (digital input 11) to trigger an interrupt on state change.
}

// Disable interrupt for state change of D11 (PTT)
void DisablePTTDetector() {
    PCMSK0 &= ~(1 << PCINT5);
}

// This interrupt driven function reads data from the wattmeter
void timerISR() {
    if (state.txIsOn == 1) {
        // read forward power
        long s_calc = static_cast<long>(analogRead(12)) * M_CORR;
        f_avg[f_i] = s_calc / static_cast<long>(100);

        f_ave += f_avg[f_i++]; // Add in the new sample
        if (f_i > 24) f_i = 0; // Update the index value
        f_ave -= f_avg[f_i]; // Subtract off the 51st value
        const unsigned int ftmp = f_ave / 25;

        if (ftmp > f_tot) {
            f_tot = ftmp;
            f_pep = 75;
        } else {
            if (f_pep > 0)f_pep--;
            else if (f_tot > 0) f_tot--;
        }

        // read reflected power
        s_calc = static_cast<long>(analogRead(13)) * M_CORR;
        const unsigned int rtmp = s_calc / static_cast<long>(100);

        if (rtmp > r_tot) {
            r_tot = rtmp;
            r_pep = 75;
        } else {
            if (r_pep > 0)r_pep--;
            else if (r_tot > 0) r_tot--;
        }
    } else {
        f_tot = 0;
        r_tot = 0;
    }

    long dtmp = analogRead(15);
    dtmp = (dtmp * static_cast<long>(d_lin[state.band])) / static_cast<long>(100);

    if (dtmp > d_tot) {
        d_tot = dtmp;
        d_pep = 75;
    } else {
        if (d_pep > 0)d_pep--;
        else if (d_tot > 0) d_tot--;
    }


    // handle touching repeat
    if (timeToTouch > 0) {
        timeToTouch--;
    }

    // handle reenabling of PTT detection
    if (timeToEnablePTTDetector > 0) {
        timeToEnablePTTDetector--;
        if (timeToEnablePTTDetector == 0 && state.mode != 0) {
            EnablePTTDetector();
        }
    }

    if (RL_off > 0) {
        RL_off--;
    }
}

char TEMPbuff[16];
int t_count = 0;
int a_count = 0;
int s_count = 0;

unsigned int F_bar = 9, OF_bar = 9;
char RL_TXT[] = {"-.-"};
char ORL_TXT[] = {"..."};

int t_disp = 0, t_read;
int s_disp = 19;

char Bias_buff[] = {"          "};
int oBcurr = -1;
int otemp = -99;
byte FTband;


void SwitchToTX(void) {
    F_alert = 1;
    R_alert = 1;
    D_alert = 1;
    V_alert = 1;
    I_alert = 1;
    OF_alert = 0;
    OR_alert = 0;
    OD_alert = 0;
    OV_alert = 0;
    OI_alert = 0;

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
    Tft.drawString((uint8_t *) RL_TXT, 70, 203, 2, LGRAY);
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
    if (ftv < 780)return 1;

    return 0;
}

byte Xiegudet() {
    int ftv = AnalogRead(FT817_V);

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

void ReadFreq(void) {
    unsigned long frq;
    int cnt = 0;
    byte same_cnt = 0;
    byte band_read;
    byte last_band = 0;
    FreqCount.begin(1);

    while (cnt < 12 && digitalRead(COR_DET)) {
        while (!FreqCount.available());

        frq = FreqCount.read() * 16;
        band_read = 0;

        if (frq > 1750 && frq < 2100) band_read = 10;
        else if (frq > 3000 && frq < 4500) band_read = 9;
        else if (frq > 4900 && frq < 5700) band_read = 8;
        else if (frq > 6800 && frq < 7500) band_read = 7;
        else if (frq > 9800 && frq < 11000) band_read = 6;
        else if (frq > 13500 && frq < 15000) band_read = 5;
        else if (frq > 17000 && frq < 19000) band_read = 4;
        else if (frq > 20500 && frq < 22000) band_read = 3;
        else if (frq > 23500 && frq < 25400) band_read = 2;
        else if (frq > 27500 && frq < 30000) band_read = 1;

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

    FreqCount.end();
}


byte getTS(byte ts) {
    uint16_t x, y;
    uint8_t key;
    TS_Point p;

    if (ts == 1) p = ts1.getPoint();

    if (ts == 2) p = ts2.getPoint();

    x = map(p.x, 3850, 400, 0, 5);
    y = map(p.y, 350, 3700, 0, 4);
    //  z = p.z;
    key = x + 5 * y;

    return key;
}

void SetBand() {
    byte sr_data = 0;
    if (state.band > 10) return;
    if (state.txIsOn) return;

    switch (state.band) {
        case 0:
            sr_data = 0x00;
            Serial3.println("*B0");
            break;

        case 1:
            sr_data = 0x20;
            Serial3.println("*B1");
            break;

        case 2:
            sr_data = 0x20;
            Serial3.println("*B2");
            break;

        case 3:
            sr_data = 0x20;
            Serial3.println("*B3");
            break;

        case 4:
            sr_data = 0x08;
            Serial3.println("*B4");
            break;

        case 5:
            sr_data = 0x08;
            Serial3.println("*B5");
            break;

        case 6:
            sr_data = 0x04;
            Serial3.println("*B6");
            break;

        case 7:
            sr_data = 0x04;
            Serial3.println("*B7");
            break;

        case 8:
            sr_data = 0x02;
            Serial3.println("*B8");
            break;

        case 9:
            sr_data = 0x01;
            Serial3.println("*B9");
            break;

        case 10:
            sr_data = 0x00;
            Serial3.println("*B10");
            break;
    }

    if (state.atuIsPresent == 1 && !state.menuDisplayed) {
        Tft.LCD_SEL = 1;
        Tft.lcd_fill_rect(121, 142, 74, 21, MGRAY);
    }

    state.lpfBoardSerialData = sr_data;

    if (state.band != OBAND) {
        SendLPFRelayData(state.lpfBoardSerialData);
        Tft.LCD_SEL = 1;
        DrawBand(OBAND, MGRAY);
        OBAND = state.band;
        int dcolor = ORANGE;

        if (state.band == 0) dcolor = RED;

        DrawBand(state.band, dcolor);
        EEPROM.write(eeband, state.band);
        DrawAnt();
    }
}

void SetFanSpeed(byte FS) {
    switch (FS) {
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
            digitalWrite(FAN1, HIGH);
            digitalWrite(FAN2, HIGH);
            temp_utp = 2500;
            temp_dtp = 550;
    }
}

void SetTransceiver(byte s_xcvr) {
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
        Set_Ser2(ACC_Baud);
    }

    if (s_xcvr == xic705) {
        ACC_Baud = 2;
        EEPROM.write(eeaccbaud, ACC_Baud);
        Set_Ser2(ACC_Baud);
    }
}

void SendLPFRelayData(byte data) {
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
    if (state.band > 10) state.band = 5;

    state.mode = EEPROM.read(eemode);
    if (state.mode > 1) state.mode = 0;

    DisablePTTDetector();
    if (state.mode != 0) {
        EnablePTTDetector();
    }

    state.atuActive = EEPROM.read(eeatub) == 1;

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

    ACC_Baud = EEPROM.read(eeaccbaud);
    if (ACC_Baud < 0 || ACC_Baud > 3) ACC_Baud = 2;
    Set_Ser2(ACC_Baud);

    USB_Baud = EEPROM.read(eeusbbaud);
    if (USB_Baud < 0 || USB_Baud > 5) USB_Baud = 2;
    Set_Ser(USB_Baud);

    state.tempInCelsius = EEPROM.read(eecelsius) > 0;

    state.meterSelection = EEPROM.read(eemetsel);
    if (state.meterSelection < 1 || state.meterSelection > 5)
        state.meterSelection = 1;

    OMeterSel = state.meterSelection;

    state.trxType = EEPROM.read(eexcvr);
    if (state.trxType < 0 || state.trxType > xcvr_max) state.trxType = 0;
    strcpy(item_disp[mXCVR], xcvr_disp[state.trxType]);
    SetTransceiver(state.trxType);

    byte MCAL = EEPROM.read(eemcal);
    if (MCAL < 75 || MCAL > 125) MCAL = 100;
    M_CORR = static_cast<long>(MCAL);
    sprintf(item_disp[mMCAL], "      %3li       ", M_CORR);

    if (ATTN_INST_READ == LOW) {
        ATTN_P = 1;
        ATTN_ST = EEPROM.read(eeattn);

        if (ATTN_ST > 1) ATTN_ST = 0;

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

    SetFanSpeed(0);
    DrawMeter();

    Serial3.begin(19200); //ATU
    Serial3.println(" ");
    strcpy(ATU_ver, "---");

    if (ATUQuery("*I") > 10) {
        strncpy(ATU_cbuf, ATU_buff, 9);
        if (strcmp(ATU_cbuf, "HR500 ATU") == 0) {
            state.atuIsPresent = true;
            auto c = ATUQuery("*V");
            strncpy(ATU_ver, ATU_buff, c - 1);
        }
    }

    DrawHome();
    Tft.LCD_SEL = 0;
    Tft.lcd_fill_rect(20, 34, 25, 10, GREEN);
    Tft.lcd_fill_rect(84, 34, 25, 10, GREEN);

    if (ATTN_ST == 0) Tft.lcd_fill_rect(148, 34, 25, 10, GREEN);

    Tft.lcd_fill_rect(212, 34, 25, 10, GREEN);
    Tft.lcd_fill_rect(276, 34, 25, 10, GREEN);

    flagPTT = 0;

    while (ts1.touched());
    while (ts2.touched());

    SetBand();
}


ISR(PCINT0_vect) {
    if (state.mode == 0) return; // Mode is STBY
    if (state.menuDisplayed) return; // Menu is active
    if (state.band == 0) return; // Band is undefined
    if (state.isTuning == 1) return; //ATU is working

    curPTT = digitalRead(PTT_DET);
    flagPTT = 1;
    DisablePTTDetector();
    timeToEnablePTTDetector = 20;

    if (curPTT == 1 && PTT == 0) {
        PTT = 1;
        RF_ACTIVE
        SendLPFRelayData(state.lpfBoardSerialData + 0x10);
        BIAS_ON
        state.txIsOn = true;
    } else {
        PTT = 0;
        BIAS_OFF
        state.txIsOn = false;
        SendLPFRelayData(state.lpfBoardSerialData);
        RF_BYPASS
    }
}

void loop() {
    if (++a_count == 10) {
        unsigned int f_yel = 600, f_red = 660;
        a_count = 0;

        if (state.band == 10) {
            f_yel = 410;
            f_red = 482;
        }

        if (f_tot > f_yel && F_alert == 1) {
            F_alert = 2;
        }

        if (f_tot > f_red) {
            F_alert = 3;
            TripSet();
        }

        if (F_alert != OF_alert) {
            OF_alert = F_alert;
            int r_col = GREEN;
            if (F_alert == 2) r_col = YELLOW;
            if (F_alert == 3) r_col = RED;
            Tft.LCD_SEL = 0;
            Tft.lcd_fill_rect(20, 34, 25, 10, r_col);
        }

        if (r_tot > 450 && R_alert == 1) {
            R_alert = 2;
        }

        if (r_tot > 590) {
            R_alert = 3;
            TripSet();
        }

        if (R_alert != OR_alert) {
            OR_alert = R_alert;
            unsigned int r_col = GREEN;

            if (R_alert == 2) r_col = YELLOW;

            if (R_alert == 3) r_col = RED;

            Tft.LCD_SEL = 0;
            Tft.lcd_fill_rect(84, 34, 25, 10, r_col);
        }

        if (d_tot > 900 && D_alert == 1) {
            D_alert = 2;
        }

        if (d_tot > 1100) {
            D_alert = 3;
        }

        if (ATTN_ST == 1) D_alert = 0;

        if (state.txIsOn == 0) D_alert = 0;

        if (D_alert != OD_alert) {
            OD_alert = D_alert;
            unsigned int r_col = GREEN;

            if (ATTN_ST == 1) r_col = DGRAY;

            if (D_alert == 2) r_col = YELLOW;

            if (D_alert == 3) {
                r_col = RED;

                if (state.txIsOn == 1) TripSet();
            }

            Tft.LCD_SEL = 0;
            Tft.lcd_fill_rect(148, 34, 25, 10, r_col);
        }

        int dc_vol = ReadVoltage();

        if (dc_vol < 1800) V_alert = 2;
        else if (dc_vol > 3000) V_alert = 2;
        else V_alert = 1;

        if (V_alert != OV_alert) {
            OV_alert = V_alert;
            unsigned int r_col = GREEN;

            if (V_alert == 2) {
                r_col = YELLOW;
            }

            Tft.LCD_SEL = 0;
            Tft.lcd_fill_rect(212, 34, 25, 10, r_col);
        }

        int dc_cur = ReadCurrent();
        int MC1 = 180 * MAX_CUR;
        int MC2 = 200 * MAX_CUR;

        if (dc_cur > MC1 && I_alert == 1) {
            I_alert = 2;;
        }

        if (dc_cur > MC2) {
            I_alert = 3;
            TripSet();
        }

        if (I_alert != OI_alert) {
            OI_alert = I_alert;
            unsigned int r_col = GREEN;

            if (I_alert == 2) r_col = YELLOW;

            if (I_alert == 3) r_col = RED;

            Tft.LCD_SEL = 0;
            Tft.lcd_fill_rect(276, 34, 25, 10, r_col);
        }

        if (t_ave > temp_utp) SetFanSpeed(++FAN_SP);
        else if (t_ave < temp_dtp) SetFanSpeed(--FAN_SP);
    }

    if (++t_count == 3) {
        t_count = 0;

        if (state.meterSelection == 1) {
            unsigned int f_pw = ReadPower(fwd_p);
            F_bar = constrain(map(f_pw, 0, 500, 19, 300), 10, 309);
        } else if (state.meterSelection == 2) {
            unsigned int r_pw = ReadPower(rfl_p);
            F_bar = constrain(map(r_pw, 0, 50, 19, 300), 10, 309);
        } else if (state.meterSelection == 3) {
            unsigned int d_pw = ReadPower(drv_p);
            F_bar = constrain(map(d_pw, 0, 100, 19, 300), 10, 309);
        } else if (state.meterSelection == 4) {
            F_bar = constrain(map(ReadVoltage(), 0, 2400, 19, 299), 19, 309);
        } else {
            //MeterSel == 5
            F_bar = constrain(map(ReadCurrent(), 0, 4000, 19, 299), 19, 309);
        }

        Tft.LCD_SEL = 0;

        while (F_bar != OF_bar) {
            if (OF_bar < F_bar) Tft.lcd_draw_v_line(OF_bar++, 101, 12, GREEN);

            if (OF_bar > F_bar) Tft.lcd_draw_v_line(--OF_bar, 101, 12, MGRAY);
        }

        if (Bias_Meter == 1) {
            int Bcurr = ReadCurrent() * 5;

            if (Bcurr != oBcurr) {
                oBcurr = Bcurr;
                Tft.LCD_SEL = 1;
                Tft.drawString((uint8_t *) Bias_buff, 65, 80, 2, MGRAY);
                sprintf(Bias_buff, "  %d mA", Bcurr);
                Tft.drawString((uint8_t *) Bias_buff, 65, 80, 2, WHITE);
            }
        }

        if (s_disp++ == 20 && f_tot > 250 && state.txIsOn == 1) {
            int VSWR = ReadPower(vswr);

            s_disp = 0;

            if (VSWR > 9) sprintf(RL_TXT, "%d.%d", VSWR / 10, VSWR % 10);

            if (strcmp(ORL_TXT, RL_TXT) != 0) {
                Tft.LCD_SEL = 0;
                Tft.lcd_fill_rect(70, 203, 36, 16, MGRAY);
                Tft.drawString((uint8_t *) RL_TXT, 70, 203, 2, WHITE);
                strcpy(ORL_TXT, RL_TXT);
            }
        }

        if (t_disp++ == 200) {
            t_disp = 0;
            t_avg[t_i] = constrain(AnalogRead(14), 5, 2000);
            t_tot += t_avg[t_i++]; // Add in the new sample

            if (t_i > 10) t_i = 0; // Update the index value

            t_tot -= t_avg[t_i]; // Subtract off the 51st value
            t_ave = t_tot / 5;

            unsigned int t_color = GREEN;
            if (t_ave > 500) t_color = YELLOW;
            if (t_ave > 650) t_color = RED;
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
                Tft.drawString((uint8_t *) TEMPbuff, 237, 203, 2, DGRAY);
                if (state.tempInCelsius) {
                    sprintf(TEMPbuff, "%d&C", t_read);
                } else {
                    sprintf(TEMPbuff, "%d&F", t_read);
                }

                Tft.drawString((uint8_t *) TEMPbuff, 237, 203, 2, t_color);
            }
        }
    }


    if (flagPTT == 1) {
        flagPTT = 0;

        if (PTT == 1) {
            if (state.mode != 0) {
                SwitchToTX();
            }

            curPTT = digitalRead(PTT_DET);
            if (curPTT == 0 && state.mode == 1) {
                SwitchToRX();
            }
        } else {
            if (state.mode != 0) {
                SwitchToRX();
            }

            curPTT = digitalRead(PTT_DET);
            if (curPTT == 1) {
                SwitchToTX();
            }
        }
    }

    byte sampCOR = digitalRead(COR_DET);
    if (OLD_COR != sampCOR) {
        OLD_COR = sampCOR;

        if (sampCOR == 1) {
            if (state.trxType != xft817 && state.txIsOn)
                ReadFreq();

            if (state.band != OBAND) {
                BIAS_OFF
                RF_BYPASS
                state.txIsOn = false;
                SetBand();
            }
        }
    }

    if (!state.txIsOn && state.trxType == xft817) {
        byte nFT817 = FT817det();
        while (nFT817 != FT817det()) {
            nFT817 = FT817det();
        }

        if (nFT817 != 0) {
            FTband = nFT817;
        }

        if (FTband != state.band) {
            state.band = FTband;
            SetBand();
        }
    }

    if (!state.txIsOn && state.trxType == xxieg) {
        byte nXieg = Xiegudet();
        while (nXieg != Xiegudet()) {
            nXieg = Xiegudet();
        }

        if (nXieg != 0) {
            FTband = nXieg;
        }

        if (FTband != state.band) {
            state.band = FTband;
            SetBand();
        }
    }

    if (!state.txIsOn && state.trxType == xelad) {
        byte nElad = Eladdet();
        while (nElad != Eladdet()) {
            nElad = Eladdet();
        }

        if (nElad != 0) FTband = nElad;

        if (FTband != state.band) {
            state.band = FTband;
            SetBand();
        }
    }

    if (ts1.touched()) {
        const byte pressedKey = getTS(1);
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

        if (OMeterSel != state.meterSelection) {
            DrawButtonUp(OMeterSel);
            DrawButtonDn(state.meterSelection);
            OMeterSel = state.meterSelection;
            EEPROM.write(eemetsel, state.meterSelection);
        }

        while (ts1.touched());
    }

    if (ts2.touched() && timeToTouch == 0) {
        const byte pressedKey = getTS(2);
        timeToTouch = 300;

        if (!state.menuDisplayed) {
            switch (pressedKey) {
                case 5:
                case 6:
                    if (state.txIsOn == 0) {
                        if (++state.mode == 2) state.mode = 0;

                        EEPROM.write(eemode, state.mode);
                        DrawMode();
                        DisablePTTDetector();

                        if (state.mode == 1) EnablePTTDetector();
                    }
                    break;

                case 8:
                    if (state.txIsOn == 0) {
                        if (++state.band >= 11) state.band = 1;

                        SetBand();
                    }
                    break;

                case 9:
                    if (state.txIsOn == 0) {
                        if (--state.band == 0) state.band = 10;

                        if (state.band == 0xff) state.band = 10;

                        SetBand();
                    }
                    break;

                case 15:
                case 16:
                    if (state.txIsOn == 0) {
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
                    if (state.atuIsPresent && !state.txIsOn) {
                        state.atuActive = !state.atuActive;
                        DrawATU();
                    }
                    break;

                case 12:
                    if (!state.atuIsPresent) {
                        Tft.LCD_SEL = 1;
                        Tft.lcd_clear_screen(GRAY);
                        DrawMenu();
                        state.menuDisplayed = true;
                    }
                    break;

                case 7:
                    if (state.atuIsPresent) {
                        Tft.LCD_SEL = 1;
                        Tft.lcd_clear_screen(GRAY);
                        DrawMenu();
                        state.menuDisplayed = true;
                    }
                    break;

                case 17:
                    if (state.atuIsPresent) {
                        TuneButtonPressed();
                    }
            }
        } else {
            switch (pressedKey) {
                case 0:
                case 1:
                    if (menuSEL == 0) {
                        Tft.LCD_SEL = 1;
                        Tft.drawString((uint8_t *) menu_items[menu_choice], 65, 20, 2, MGRAY);
                        Tft.drawString((uint8_t *) item_disp[menu_choice], 65, 80, 2, MGRAY);

                        if (menu_choice-- == 0) menu_choice = menu_max;

                        Tft.drawString((uint8_t *) menu_items[menu_choice], 65, 20, 2, WHITE);
                        Tft.drawString((uint8_t *) item_disp[menu_choice], 65, 80, 2, LGRAY);
                    }

                    break;

                case 3:
                case 4:
                    if (menuSEL == 0) {
                        Tft.LCD_SEL = 1;
                        Tft.drawString((uint8_t *) menu_items[menu_choice], 65, 20, 2, MGRAY);
                        Tft.drawString((uint8_t *) item_disp[menu_choice], 65, 80, 2, MGRAY);

                        if (++menu_choice > menu_max) menu_choice = 0;

                        Tft.drawString((uint8_t *) menu_items[menu_choice], 65, 20, 2, WHITE);
                        Tft.drawString((uint8_t *) item_disp[menu_choice], 65, 80, 2, LGRAY);
                    }

                    break;

                case 5:
                case 6:
                    if (menuSEL == 1)
                        menuFunction(menu_choice, 0);
                    break;

                case 8:
                case 9:
                    if (menuSEL == 1)
                        menuFunction(menu_choice, 1);
                    break;

                case 7:
                case 12:
                    menuSelect();
                    break;

                case 17:
                    Tft.LCD_SEL = 1;
                    Tft.lcd_clear_screen(GRAY);
                    state.menuDisplayed = false;

                    if (menu_choice == mSETbias) {
                        BIAS_OFF
                        SendLPFRelayDataSafe(state.lpfBoardSerialData);
                        state.mode = oMODE;
                        MAX_CUR = 20;
                        Bias_Meter = 0;
                    }

                    DrawHome();
                    Tft.LCD_SEL = 0;
                    Tft.lcd_reset();
            }
        }

        while (ts2.touched());
    }

    const auto atuBusy = digitalRead(ATU_BUSY) == 1;
    if (state.isTuning && !atuBusy) {
        TuneEnd();
    }

    while (Serial2.available()) {
        unsigned char cx = Serial2.read();
        rxbuff2[uartPtr2] = cx;

        if (rxbuff2[uartPtr2] == 0x3B) {
            uartGrabBuffer2();
            findBand(2);
        }

        if (++uartPtr2 > 127) uartPtr2 = 0;
    }

    while (Serial.available()) {
        rxbuff[uartPtr] = Serial.read(); // Storing read data

        if (rxbuff[uartPtr] == 0x3B) {
            uartGrabBuffer();
            findBand(1);
        }

        if (++uartPtr > 127) uartPtr = 0;
    }
}
