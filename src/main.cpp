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

const unsigned char PS_32 = (1 << ADPS2) | (1 << ADPS0);
const unsigned char PS_128 = (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

XPT2046_Touchscreen ts1(TP1_CS);
XPT2046_Touchscreen ts2(TP2_CS);

int Ana_Read(byte Pin);
void Send_RLY(byte data);

long M_CORR = 100;

byte FAN_SP = 0;
char ATU_STAT;
char ATTN_P = 0;
byte ATTN_ST = 0;
volatile byte BAND = 6;
byte OBAND = 0;
byte CELSIUS = 1;
byte ANTSEL[11] = {1,1,1,1,1,1,1,1,1,1,1};
volatile int flagTCH = 0; // countdown for TS touching. this gets decremented in timer ISR
byte ACC_Baud;
byte USB_Baud;
byte menu_choice = 0;
byte ATU_P = 0;
byte ATU = 0;
byte MeterSel = 0; //1 - FWD; 2 - RFL; 3 - DRV; 4 - VDD; 5 - IDD
byte OMeterSel;
unsigned int temp_utp, temp_dtp;
byte menuSEL = 0;
byte Bias_Meter = 0;
int MAX_CUR = 20;
byte XCVR = 0;

volatile byte MODE = 0; // 0 - OFF, 1 - PTT
byte oMODE;
byte TX = 0;
byte SCREEN = 0;
volatile byte SR_DATA = 0;
int TICK = 0;
int RD_ave = 0, FD_ave = 0;
int FT8CNT = 0;

volatile unsigned int f_tot = 0, f_ave = 0;
volatile unsigned int f_avg[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
volatile byte f_i = 0;
volatile unsigned int r_tot = 0;
volatile unsigned int d_tot = 0;

volatile unsigned int f_pep = 0;
volatile unsigned int r_pep = 0;
volatile unsigned int d_pep = 0;

unsigned int t_avg[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
unsigned int t_tot = 0, t_ave;
byte t_i = 0;

volatile byte curCOR, curPTT;
volatile byte flagPTT = 0;
volatile byte PTT = 0;
volatile byte flagDIS = 0; // Countdown timer for re-enabling PTT detector interrupt. This gets updated in the timer ISR
volatile int RL_off = 0;
byte OLD_COR = 0;

byte I_alert = 0, V_alert = 0, F_alert = 0, R_alert = 0, D_alert = 0;
byte OI_alert = 0, OV_alert = 0, OF_alert = 0, OR_alert = 0, OD_alert = 0;

byte ADCvinMSB, ADCvinLSB, curSenseMSB, curSenseLSB;
unsigned int ADCvin, ADCcur;

unsigned int uartPtr = 0, uartPtr2 = 0;
unsigned int uartMsgs = 0, uartMsgs2 = 0, readStart = 0, readStart2 = 0;
char rxbuff[128];          // 128 byte circular Buffer for storing rx data
char workingString[128];
char rxbuff2[128];         // 128 byte circular Buffer for storing rx data
char workingString2[128];

char ATU_buff[40], ATU_cmd[8], ATU_cbuf[32], ATU_ver[8];
byte TUNING = 0;



// Enable interrupt on state change of D11 (PTT)
void Enable11(void) {
    PCICR |= (1 << PCIE0);
    PCMSK0 |= (1 << PCINT5);  //Set PCINT0 (digital input 11) to trigger an interrupt on state change.
}

// Disable interrupt for state change of D11 (PTT)
void Disable11(void) {
    PCMSK0 &= ~(1 << PCINT5);
}

// This interrupt driven function reads data from the wattmeter
void timerISR(void) {
    long s_calc;

    if (TX == 1) {
        // read forward power
        s_calc = long(analogRead(12)) * M_CORR;
        f_avg[f_i] = s_calc / long(100);

        f_ave += f_avg[f_i++];         // Add in the new sample

        if (f_i > 24) f_i = 0;          // Update the index value

        f_ave -= f_avg[f_i];           // Subtract off the 51st value
        unsigned int ftmp = f_ave / 25;

        if (ftmp > f_tot) {
            f_tot = ftmp;
            f_pep = 75;
        } else {
            if (f_pep > 0)f_pep--;
            else if (f_tot > 0) f_tot--;
        }

        // read reflected power
        s_calc = long(analogRead(13)) * M_CORR;
        unsigned int rtmp = s_calc / long(100);

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
    dtmp = (dtmp * long(d_lin[BAND]))/long(100);

    if (dtmp > d_tot) {
        d_tot = dtmp;
        d_pep = 75;
    } else {
        if (d_pep > 0)d_pep--;
        else if (d_tot > 0) d_tot--;
    }


    // handle touching repeat
    if (flagTCH > 0) --flagTCH;

    // handle reenabling of PTT detection
    if (flagDIS > 0) {
        --flagDIS;

        if (flagDIS == 0) {
            if (MODE != 0) Enable11();
        }
    }

    if (RL_off > 0) --RL_off;
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



void Switch_to_TX(void) {
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

void Switch_to_RX (void) {
    DrawRxButtons(GBLUE);
    DrawTxPanel(GREEN);
    trip_clear();
    RESET_PULSE
    Tft.LCD_SEL = 0;
    Tft.lcd_fill_rect(70, 203, 36, 16, MGRAY);
    Tft.drawString((uint8_t *)RL_TXT, 70, 203, 2, LGRAY);
    strcpy(ORL_TXT, "   ");
}

byte FT817det(void) {
    int ftv = Ana_Read(FT817_V);

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

byte Eladdet(void) {
    int ftv = Ana_Read(FT817_V);

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

byte Xiegudet(void) {
    int ftv = Ana_Read(FT817_V);

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
            BAND = last_band;
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

void SetBand(void) {
    byte sr_data = 0;

    if (BAND > 10) return;

    if (TX == 1) return;

    switch (BAND) {
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

    if (ATU_P == 1 && SCREEN == 0) {
        Tft.LCD_SEL = 1;
        Tft.lcd_fill_rect(121, 142, 74, 21, MGRAY);
    }

    SR_DATA = sr_data;

    if (BAND != OBAND) {
        Send_RLY(SR_DATA);
        Tft.LCD_SEL = 1;
        DrawBand(OBAND, MGRAY);
        OBAND = BAND;
        int dcolor = ORANGE;

        if (BAND == 0) dcolor = RED;

        DrawBand(BAND, dcolor);
        EEPROM.write(eeband, BAND);
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

void SET_XCVR(byte s_xcvr) {
    if (s_xcvr == xhobby || s_xcvr == xkx23) S_POL_REV
        else S_POL_NORM

            if (s_xcvr == xhobby) {
                pinMode(TTL_PU, OUTPUT);
                digitalWrite(TTL_PU, HIGH);
            } else {
                digitalWrite(TTL_PU, LOW);
                pinMode(TTL_PU, INPUT);
            }

    if (s_xcvr == xft817 || s_xcvr == xelad || s_xcvr == xxieg) {
        item_disp[mACCbaud] = (char *)"  XCVR MODE ON  ";
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

void Send_RLY(byte data) {
    noInterrupts();
    RELAY_CS_LOW
    SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE3));
    SPI.transfer(data);
    SPI.endTransaction();
    RELAY_CS_HIGH
    interrupts();
}
int Ana_Read(byte Pin) {
    noInterrupts();
    int a = analogRead(Pin);
    interrupts();
    return a;
}

void setup() {
    int c;

    SETUP_RELAY_CS  RELAY_CS_HIGH
    SETUP_LCD1_CS  LCD1_CS_HIGH
    SETUP_LCD2_CS  LCD2_CS_HIGH
    SETUP_SD1_CS  SD1_CS_HIGH
    SETUP_SD2_CS  SD2_CS_HIGH
    SETUP_TP1_CS  TP1_CS_HIGH
    SETUP_TP2_CS  TP2_CS_HIGH
    SETUP_LCD_BL  analogWrite(LCD_BL, 255);
    SETUP_BYP_RLY RF_BYPASS
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

    pinMode (RST_OUT, OUTPUT);
    digitalWrite (RST_OUT, HIGH);

    BAND = EEPROM.read(eeband);

    if (BAND > 10) BAND = 5;

    MODE = EEPROM.read(eemode);

    if (MODE < 0 || MODE > 1) MODE = 0;

    Disable11();

    if (MODE != 0) Enable11();

    ATU = EEPROM.read(eeatub);

    if (ATU < 0 || ATU > 1) ATU = 0;

    for (byte i = 1 ; i < 11 ; i++) {
        ANTSEL[i] = EEPROM.read(eeantsel+i);

        if (ANTSEL[i] == 1) {
            SEL_ANT1;
        } else if (ANTSEL[i] == 2) {
            SEL_ANT2;
        } else {
            SEL_ANT1;
            ANTSEL[i] = 1;
            EEPROM.write(eeantsel+i,1);
        }
    }

    ACC_Baud = EEPROM.read(eeaccbaud);

    if (ACC_Baud < 0 || ACC_Baud > 3) ACC_Baud = 2;

    Set_Ser2(ACC_Baud);

    USB_Baud = EEPROM.read(eeusbbaud);

    if (USB_Baud < 0 || USB_Baud > 5) USB_Baud = 2;

    Set_Ser(USB_Baud);

    CELSIUS = EEPROM.read(eecelsius);

    if (CELSIUS != 1 && CELSIUS != 0) CELSIUS = 1;

    MeterSel = EEPROM.read(eemetsel);

    if (MeterSel < 1 || MeterSel > 5) MeterSel = 1;

    OMeterSel = MeterSel;

    XCVR = EEPROM.read(eexcvr);

    if (XCVR < 0 || XCVR > xcvr_max) XCVR = 0;

    strcpy (item_disp[mXCVR],xcvr_disp[XCVR]);
    SET_XCVR(XCVR);

    byte MCAL = EEPROM.read(eemcal);

    if (MCAL < 75 || MCAL > 125) MCAL = 100;

    M_CORR = long(MCAL);
    sprintf(item_disp[mMCAL], "      %3li       ", M_CORR);

    if (ATTN_INST_READ == LOW) {
        ATTN_P = 1;
        ATTN_ST = EEPROM.read(eeattn);

        if (ATTN_ST > 1) ATTN_ST = 0;

        if (ATTN_ST == 1) {
            ATTN_ON_HIGH;
            item_disp[mATTN] = (char *)" ATTENUATOR IN  ";
        } else {
            ATTN_ON_LOW;
            item_disp[mATTN] = (char *)" ATTENUATOR OUT ";
        }
    } else {
        ATTN_P = 0;
        ATTN_ST = 0;
        item_disp[mATTN] = (char *)" NO ATTENUATOR  ";
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
    trip_clear();

    Tft.LCD_SEL = 0;
    Tft.lcd_init(GRAY);
    Tft.LCD_SEL = 1;
    Tft.lcd_init(GRAY);

    SetFanSpeed(0);
    DrawMeter();

    Serial3.begin(19200); //ATU
    Serial3.println(" ");
    strcpy(ATU_cmd, "*I");
    strcpy(ATU_ver, "---");

    if (ATU_exch() > 10) {
        strncpy(ATU_cbuf, ATU_buff, 9);

        if (strcmp(ATU_cbuf, "HR500 ATU") == 0) {
            ATU_P = 1;
            strcpy(ATU_cmd, "*V");
            c = ATU_exch();
            strncpy(ATU_ver, ATU_buff, c - 1);
        }
    }

    Serial3.println(" ");
    strcpy(ATU_cmd, "*I");
    strcpy(ATU_ver, "---");

    if (ATU_exch() > 10) {
        strncpy(ATU_cbuf, ATU_buff, 9);

        if (strcmp(ATU_cbuf, "HR500 ATU") == 0) {
            ATU_P = 1;
            strcpy(ATU_cmd, "*V");
            c = ATU_exch();
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
    if (MODE == 0) return; // Mode is STBY

    if (SCREEN == 1) return; // Menu is active

    if (BAND == 0) return; // Band is undefined

    if (TUNING == 1) return; //ATU is working

    curPTT = digitalRead(PTT_DET);
    flagPTT = 1;
    Disable11();
    flagDIS = 20;

    if (curPTT == 1 && PTT == 0) {
        PTT = 1;
        RF_ACTIVE
        byte SRSend_RLY = SR_DATA + 0x10;
        RELAY_CS_LOW
        SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE3));
        SPI.transfer(SRSend_RLY);
        SPI.endTransaction();
        RELAY_CS_HIGH
        BIAS_ON
        TX = 1;
    } else {
        PTT = 0;
        BIAS_OFF
        TX = 0;
        byte SRSend_RLY = SR_DATA;
        RELAY_CS_LOW
        SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE3));
        SPI.transfer(SRSend_RLY);
        SPI.endTransaction();
        RELAY_CS_HIGH
        RF_BYPASS
    }

}

void loop() {
    int r_col;

    if (++a_count == 10) {
        unsigned int f_yel = 600, f_red = 660;
        a_count = 0;

        if (BAND == 10) {
            f_yel = 410;
            f_red = 482;
        }

        if (f_tot > f_yel && F_alert == 1) {
            F_alert = 2;
        }

        if (f_tot > f_red) {
            F_alert = 3;
            trip_set();
        }

        if (F_alert != OF_alert) {
            OF_alert = F_alert;
            r_col = GREEN;

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
            trip_set();
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

        if (TX == 0) D_alert = 0;

        if (D_alert != OD_alert) {
            OD_alert = D_alert;
            unsigned int r_col = GREEN;

            if (ATTN_ST == 1) r_col = DGRAY;

            if (D_alert == 2) r_col = YELLOW;

            if (D_alert == 3) {
                r_col = RED;

                if (TX == 1) trip_set();
            }

            Tft.LCD_SEL = 0;
            Tft.lcd_fill_rect(148, 34, 25, 10, r_col);
        }

        int dc_vol = Read_Voltage();

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

        int dc_cur = Read_Current();
        int MC1 = 180 * MAX_CUR;
        int MC2 = 200 * MAX_CUR;

        if (dc_cur > MC1 && I_alert == 1) {
            I_alert = 2;;
        }

        if (dc_cur > MC2) {
            I_alert = 3;
            trip_set();
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

        if (MeterSel == 1) {
            unsigned int f_pw = Read_Power(fwd_p);
            F_bar = constrain(map(f_pw, 0, 500, 19, 300), 10, 309);
        } else if (MeterSel == 2) {
            unsigned int r_pw = Read_Power(rfl_p);
            F_bar = constrain(map(r_pw, 0, 50, 19, 300), 10, 309);
        } else if (MeterSel == 3) {
            unsigned int d_pw = Read_Power(drv_p);
            F_bar = constrain(map(d_pw, 0, 100, 19, 300), 10, 309);
        } else if (MeterSel == 4) {
            F_bar = constrain(map(Read_Voltage(), 0, 2400, 19, 299), 19, 309);
        } else { //MeterSel == 5
            F_bar = constrain(map(Read_Current(), 0, 4000, 19, 299), 19, 309);
        }

        Tft.LCD_SEL = 0;

        while (F_bar != OF_bar) {
            if (OF_bar < F_bar) Tft.lcd_draw_v_line(OF_bar++, 101, 12, GREEN);

            if (OF_bar > F_bar) Tft.lcd_draw_v_line(--OF_bar, 101, 12, MGRAY);
        }

        if (Bias_Meter == 1) {
            int Bcurr = Read_Current() * 5;

            if (Bcurr != oBcurr) {
                oBcurr = Bcurr;
                Tft.LCD_SEL = 1;
                Tft.drawString((uint8_t *)Bias_buff, 65, 80, 2, MGRAY);
                sprintf(Bias_buff, "  %d mA", Bcurr);
                Tft.drawString((uint8_t *)Bias_buff, 65, 80, 2, WHITE);
            }
        }

        if (s_disp++ == 20 && f_tot > 250 && TX == 1) {
            int VSWR = Read_Power(vswr);

            s_disp = 0;

            if (VSWR > 9) sprintf(RL_TXT, "%d.%d", VSWR/10, VSWR % 10);

            if (strcmp(ORL_TXT, RL_TXT) != 0) {
                Tft.LCD_SEL = 0;
                Tft.lcd_fill_rect(70, 203, 36, 16, MGRAY);
                Tft.drawString((uint8_t *)RL_TXT, 70, 203, 2, WHITE);
                strcpy(ORL_TXT, RL_TXT);
            }
        }

        if (t_disp++ == 200) {
            t_disp = 0;
            t_avg[t_i] = constrain( Ana_Read(14), 5, 2000);
            t_tot += t_avg[t_i++];         // Add in the new sample

            if (t_i > 10) t_i = 0;          // Update the index value

            t_tot -= t_avg[t_i];           // Subtract off the 51st value
            t_ave = t_tot / 5;

            unsigned int t_color = GREEN;

            if (t_ave > 500) t_color = YELLOW;

            if (t_ave> 650) t_color = RED;

            if (t_ave> 700 && TX == 1) trip_set();

            if (CELSIUS == 0) t_read = ((t_ave * 9) / 5) + 320;
            else t_read = t_ave;

            t_read /= 10;

            if (t_read != otemp) {
                otemp = t_read;
                Tft.LCD_SEL = 0;
                Tft.drawString((uint8_t *)TEMPbuff, 237, 203, 2, DGRAY);

                if (CELSIUS == 0) sprintf(TEMPbuff, "%d&F", t_read);
                else sprintf(TEMPbuff, "%d&C", t_read);

                Tft.drawString((uint8_t *)TEMPbuff, 237, 203, 2, t_color);
            }
        }
    }


    if (flagPTT == 1) {
        flagPTT = 0;

        if (PTT == 1) {
            if (MODE != 0) Switch_to_TX();

            curPTT = digitalRead(PTT_DET);

            if (curPTT == 0 && MODE == 1) Switch_to_RX();
        } else {
            if (MODE != 0) Switch_to_RX();

            curPTT = digitalRead(PTT_DET);

            if (curPTT == 1) Switch_to_TX();
        }
    }

    byte sampCOR = digitalRead(COR_DET);

    if (OLD_COR != sampCOR) {
        OLD_COR = sampCOR;

        if (sampCOR == 1) {
            if (XCVR != xft817 && TX == 1) ReadFreq();

            if (BAND != OBAND) {
                BIAS_OFF
                RF_BYPASS
                TX = 0;
                SetBand();
            }
        }
    }

    if (TX == 0 && XCVR == xft817) {
        byte nFT817 = FT817det();

        while (nFT817 != FT817det())nFT817 = FT817det();

        if (nFT817 != 0) FTband = nFT817;

        if (FTband != BAND) {
            BAND = FTband;
            SetBand();
        }
    }

    if (TX == 0 && XCVR == xxieg) {
        byte nXieg = Xiegudet();

        while (nXieg != Xiegudet())nXieg = Xiegudet();

        if (nXieg != 0) FTband = nXieg;

        if (FTband != BAND) {
            BAND = FTband;
            SetBand();
        }
    }

    if (TX == 0 && XCVR == xelad) {
        byte nElad = Eladdet();

        while (nElad != Eladdet())nElad = Eladdet();

        if (nElad != 0) FTband = nElad;

        if (FTband != BAND) {
            BAND = FTband;
            SetBand();
        }
    }

    if (ts1.touched()) {
        byte ts_key = getTS(1);

        switch (ts_key) {
            case 10:
                MeterSel = 1;
                break;

            case 11:
                MeterSel = 2;
                break;

            case 12:
                MeterSel = 3;
                break;

            case 13:
                MeterSel = 4;
                break;

            case 14:
                MeterSel = 5;
                break;

            case 18:
            case 19:
                if (CELSIUS == 0) CELSIUS = 1;
                else CELSIUS = 0;

                EEPROM.write(eecelsius, CELSIUS);
                break;
        }

        if (OMeterSel != MeterSel) {
            DrawButtonUp(OMeterSel);
            DrawButtonDn(MeterSel);
            OMeterSel = MeterSel;
            EEPROM.write(eemetsel, MeterSel);
        }

        while (ts1.touched());
    }

    if (ts2.touched() && flagTCH == 0) {
        byte ts_key = getTS(2);
        flagTCH = 300;

        if (SCREEN == 0) {
            switch (ts_key) {
                case 5:
                case 6:
                    if (TX == 0) {
                        if (++MODE == 2) MODE = 0;

                        EEPROM.write(eemode, MODE);
                        DrawMode();
                        Disable11();

                        if (MODE == 1) Enable11();
                    }

                    break;

                case 8:
                    if (TX == 0) {
                        if (++BAND >= 11) BAND = 1;

                        SetBand();
                    }

                    break;

                case 9:
                    if (TX == 0) {
                        if (--BAND == 0) BAND = 10;

                        if (BAND == 0xff) BAND = 10;

                        SetBand();
                    }

                    break;

                case 15:
                case 16:
                    if (TX == 0) {
                        if (++ANTSEL[BAND] == 3) ANTSEL[BAND] = 1;

                        EEPROM.write(eeantsel+BAND, ANTSEL[BAND]);

                        if (ANTSEL[BAND] == 1) SEL_ANT1;

                        if (ANTSEL[BAND] == 2) SEL_ANT2;

                        DrawAnt();
                    }

                    break;

                case 18:
                case 19:
                    if (ATU_P == 1 && TX == 0) {
                        if (++ATU == 2) ATU = 0;

                        DrawATU();
                    }

                    break;

                case 12:
                    if (ATU_P == 0) {
                        Tft.LCD_SEL = 1;
                        Tft.lcd_clear_screen(GRAY);
                        DrawMenu();
                        SCREEN = 1;
                    }

                    break;

                case 7:
                    if (ATU_P == 1) {
                        Tft.LCD_SEL = 1;
                        Tft.lcd_clear_screen(GRAY);
                        DrawMenu();
                        SCREEN = 1;
                    }

                    break;

                case 17:
                    if (ATU_P == 1) {
                        Tune_button();
                    }
            }
        } else {
            switch (ts_key) {
                case 0:
                case 1:
                    if (menuSEL == 0) {
                        Tft.LCD_SEL = 1;
                        Tft.drawString((uint8_t *)menu_items[menu_choice], 65, 20, 2, MGRAY);
                        Tft.drawString((uint8_t *)item_disp[menu_choice], 65, 80, 2, MGRAY);

                        if (menu_choice-- == 0) menu_choice = menu_max;

                        Tft.drawString((uint8_t *)menu_items[menu_choice], 65, 20, 2, WHITE);
                        Tft.drawString((uint8_t *)item_disp[menu_choice], 65, 80, 2, LGRAY);
                    }

                    break;

                case 3:
                case 4:
                    if (menuSEL == 0) {
                        Tft.LCD_SEL = 1;
                        Tft.drawString((uint8_t *)menu_items[menu_choice], 65, 20, 2, MGRAY);
                        Tft.drawString((uint8_t *)item_disp[menu_choice], 65, 80, 2, MGRAY);

                        if (++menu_choice > menu_max) menu_choice = 0;

                        Tft.drawString((uint8_t *)menu_items[menu_choice], 65, 20, 2, WHITE);
                        Tft.drawString((uint8_t *)item_disp[menu_choice], 65, 80, 2, LGRAY);
                    }

                    break;

                case 5:
                case 6:
                    if (menuSEL == 1) menuFunction(menu_choice, 0);

                    break;

                case 8:
                case 9:
                    if (menuSEL == 1) menuFunction(menu_choice, 1);

                    break;

                case 7:
                case 12:
                    menuSelect();
                    break;

                case 17:
                    Tft.LCD_SEL = 1;
                    Tft.lcd_clear_screen(GRAY);
                    SCREEN = 0;

                    if (menu_choice == mSETbias) {
                        BIAS_OFF
                        Send_RLY(SR_DATA);
                        MODE = oMODE;
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

    char atu_bsy = digitalRead(ATU_BUSY);

    if (TUNING == 1 && atu_bsy == 0) Tune_End();

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
        rxbuff[uartPtr] = Serial.read();     // Storing read data

        if (rxbuff[uartPtr] == 0x3B) {
            uartGrabBuffer();
            findBand(1);
        }

        if (++uartPtr > 127) uartPtr = 0;
    }
}
