#pragma once

#define VERSION "3.5A-B"

//
// LCD and Touch Panel:
#define LCD1_CS 35
#define LCD2_CS 53
#define SD1_CS 39
#define SD2_CS 44
#define TP1_CS 41
#define TP2_CS 42
#define LCD_BL 5
#define BIAS_EN 4
#define TEMP_S 14
#define TTL_PU 6
#define FT817_V 11
#define RST_OUT 8
#define ATU_TUNE 45
#define ATU_BUSY 43
#define ATTN_INST 24
#define ATTN_ON 22

#define SETUP_LCD1_CS pinMode(LCD1_CS, OUTPUT);
#define LCD1_CS_HIGH digitalWrite(LCD1_CS, HIGH);
#define LCD1_CS_LOW digitalWrite(LCD1_CS, LOW);

#define SETUP_LCD2_CS pinMode(LCD2_CS, OUTPUT);
#define LCD2_CS_HIGH digitalWrite(LCD2_CS, HIGH);
#define LCD2_CS_LOW digitalWrite(LCD2_CS, LOW);

#define SETUP_SD1_CS pinMode(SD1_CS, OUTPUT);
#define SD1_CS_HIGH digitalWrite(SD1_CS, HIGH);
#define SD1_CS_LOW digitalWrite(SD1_CS, LOW);

#define SETUP_SD2_CS pinMode(SD2_CS, OUTPUT);
#define SD2_CS_HIGH digitalWrite(SD2_CS, HIGH);
#define SD2_CS_LOW digitalWrite(SD2_CS, LOW);

#define SETUP_TP1_CS pinMode(TP1_CS, OUTPUT);
#define TP1_CS_HIGH digitalWrite(TP1_CS, HIGH);
#define TP1_CS_LOW digitalWrite(TP1_CS, LOW);

#define SETUP_TP2_CS pinMode(TP2_CS, OUTPUT);
#define TP2_CS_HIGH digitalWrite(TP2_CS, HIGH);
#define TP2_CS_LOW digitalWrite(TP2_CS, LOW);

#define SETUP_LCD_BL pinMode(LCD_BL, OUTPUT);

#define SETUP_BIAS pinMode(BIAS_EN, OUTPUT);
#define BIAS_ON digitalWrite(BIAS_EN, HIGH);
#define BIAS_OFF digitalWrite(BIAS_EN, LOW);

#define SETUP_TTL_PU pinMode(TTL_PU, INPUT);
#define SER_POL 7

#define SETUP_ATU_TUNE pinMode(ATU_TUNE, OUTPUT);
#define ATU_TUNE_HIGH digitalWrite(ATU_TUNE, HIGH);
#define ATU_TUNE_LOW digitalWrite(ATU_TUNE, LOW);

#define SETUP ATU_BUSY pinMode(ATU_TUNE, INPUT);
#define GET ATU_BUSY digitalRead(ATU_BUSY);

#define S_POL_SETUP pinMode(SER_POL, OUTPUT);
#define S_POL_NORM digitalWrite(SER_POL, LOW);
#define S_POL_REV digitalWrite(SER_POL, HIGH);

#define SETUP_ATTN_ON pinMode(ATTN_ON, OUTPUT);
#define ATTN_ON_HIGH digitalWrite(ATTN_ON, HIGH);
#define ATTN_ON_LOW digitalWrite(ATTN_ON, LOW);

#define SETUP_ATTN_INST pinMode(ATTN_INST, INPUT_PULLUP);
#define ATTN_INST_READ digitalRead(ATTN_INST)

//
// Rear Panel
#define FAN1 27
#define FAN2 29
#define BYP_RLY 31
#define ANT_RLY 33
#define COR_DET 10
#define INPUT_RF 15
#define F_COUNT 47

#define SETUP_FAN1 pinMode(FAN1, OUTPUT);
#define SETUP_FAN2 pinMode(FAN2, OUTPUT);
#define SETUP_F_COUNT pinMode(F_COUNT, INPUT_PULLUP);

#define SETUP_BYP_RLY pinMode(BYP_RLY, OUTPUT);
#define RF_BYPASS digitalWrite(BYP_RLY, LOW);
#define RF_ACTIVE digitalWrite(BYP_RLY, HIGH);

#define SETUP_ANT_RLY pinMode(ANT_RLY, OUTPUT);
#define SEL_ANT1 digitalWrite(ANT_RLY, LOW);
#define SEL_ANT2 digitalWrite(ANT_RLY, HIGH);

#define SETUP_COR pinMode(COR_DET, INPUT_PULLUP);

//
// Power Board
#define PTT_DET 11
#define RESET 23
#define LTCADDR B1101111//Table 1 both LOW (7bit address)

#define SETUP_PTT pinMode(PTT_DET, INPUT_PULLUP);
#define SETUP_RESET pinMode(RESET, OUTPUT); digitalWrite(RESET, HIGH); 
#define RESET_PULSE digitalWrite(RESET, LOW); delay(100); digitalWrite(RESET, HIGH); 
#define RESET_PULSE digitalWrite(RESET, LOW); delay(100); digitalWrite(RESET, HIGH); 

//
// LPF Board
#define RELAY_CS 25
#define TRIP_FWD 3
#define TRIP_RFL 2

#define SETUP_RELAY_CS pinMode(RELAY_CS, OUTPUT);
#define RELAY_CS_HIGH digitalWrite(RELAY_CS, HIGH);
#define RELAY_CS_LOW digitalWrite(RELAY_CS, LOW);
#define SETUP_TRIP_FWD pinMode(TRIP_FWD, INPUT_PULLUP);
#define SETUP_TRIP_RFL pinMode(TRIP_RFL, INPUT_PULLUP);

//
//EEPROM LOCATIONS
#define eeband 1
#define eemode 2
#define eeantsel 3
#define eemetsel 15
#define eecelsius 16
#define eeaccbaud 18
#define eecordelay 19
#define eexcvr 20
#define eeusbbaud 21
#define eemcal 22
#define eeatub 23
#define eeattn 24


enum{ mACCbaud,
      mXCVR,
      mMCAL,
      mSETbias,
      mUSBbaud,
      mATWfwl,
      mATTN};
      
#define menu_max 6

extern const char* menu_items[];

extern char id1[];
extern char id2[];
extern char id3[];
extern char id4[];
extern char id5[];
extern char id6[];
extern char id7[];

extern char *item_disp[];

enum{
  fwd_p,
  rfl_p,
  drv_p,
  vswr
};

#define xcvr_max 6
enum{
  xnone,
  xhobby,
  xkx23,
  xic705,
  xft817,
  xelad,
  xxieg
};


extern const char *xcvr_disp[];

const unsigned int d_lin[] = {0,  116, 114, 111, 109, 106, 104, 102, 101, 100,  99};

const unsigned int swr[] = {
    999,999,999,999,999,999,999,999,999,999,999,999,999,999,999,999,
    999,999,968,917,872,831,793,759,728,699,673,648,625,604,584,566,
    548,532,517,503,489,476,464,452,441,431,421,412,403,394,386,378,
    371,363,356,350,343,337,331,326,320,315,310,305,300,296,291,287,
    283,279,275,272,268,264,261,258,254,251,248,245,242,240,237,234,
    232,229,227,224,222,220,218,216,214,211,209,208,206,204,202,200,
    199,197,195,194,192,190,189,187,186,185,183,182,181,179,178,177,
    176,174,173,172,171,170,169,168,167,166,165,164,163,162,161,160,
    159,158,157,156,156,155,154,153,152,152,151,150,149,149,148,147,
    147,146,145,145,144,143,143,142,142,141,140,140,139,139,138,138,
    137,137,136,136,135,135,134,134,133,133,132,132,132,131,131,130,
    130,129,129,129,128,128,128,127,127,126,126,126,125,125,125,124,
    124,124,124,123,123,123,122,122,122,121,121,121,121,120,120,120,
    120,119,119,119,119,118,118,118,118,117,117,117,117,117,116,116,
    116,116,116,115,115,115,115,115,114,114,114,114,114,113,113,113,
    113,113,113,112,112,112,112,112,112,112,111,111,110,106,103,100
    };
 
