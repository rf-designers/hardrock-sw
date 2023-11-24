#include "serial_procs.h"
#include <Arduino.h>
#include <EEPROM.h>
#include "HR500V1.h"
#include "hr500_displays.h"
#include "serial_procs.h"
#include "hr500_sensors.h"

long freqLong = 0;
unsigned short oldBand;
unsigned int old_tuner_freq;
const char crlfsemi[] = ";\r\n";
const char HRTM[] = "HRTM";
char freqStr[6];


extern byte ANTSEL[11];
extern volatile byte BAND;
extern char rxbuff[128];          // 128 byte circular Buffer for storing rx data
extern char workingString[128];
extern char rxbuff2[128];         // 128 byte circular Buffer for storing rx data
extern char workingString2[128];
extern unsigned int uartMsgs, uartMsgs2, readStart, readStart2;
extern byte OBAND;
extern volatile byte MODE; // 0 - OFF, 1 - PTT
extern byte CELSIUS;
extern byte ATU_P;
extern byte ATU;
extern char ATU_STAT;
extern byte TUNING;
extern unsigned int t_tot, t_ave;
extern byte TX;
extern byte I_alert, V_alert, F_alert, R_alert, D_alert;
extern byte OI_alert, OV_alert, OF_alert, OR_alert, OD_alert;
extern char ATU_buff[40];

void SetBand(void);
void Disable11(void);
void Enable11(void);
void Tune_button(void);

void uartGrabBuffer() {
    int z = 0;
    char lastChar[1];
    lastChar[0] = 'C';

    while (lastChar[0] != 0x3B) {
        workingString[z] = rxbuff[readStart];
        rxbuff[readStart] = 0x00;
        readStart++;
        lastChar[0] =  workingString[z];
        z++;

        if (readStart > 127)readStart = 0;
    }//endwhile

    workingString[z] = 0x00;

}

void uartGrabBuffer2() {
    int z = 0;
    char lastChar[1];
    lastChar[0] = 'C';

    while (lastChar[0] != 0x3B) {
        workingString2[z] = rxbuff2[readStart2];
        rxbuff2[readStart2] = 0x00;
        readStart2++;
        lastChar[0] =  workingString2[z];
        z++;

        if (readStart2 > 127) readStart2 = 0;
    }//endwhile

    workingString2[z] = 0x00;
}


void findBand(short uart) {
    char *found;
    char *workStringPtr;
    int i, wsl;

    if (uart == 1) {
        workStringPtr = workingString;
    } else {
        workStringPtr = workingString2;
    }

    // convert working string to uppercase...
    wsl = strlen(workStringPtr);

    for (i = 0; i <= wsl; i++) workStringPtr[i] = toupper(workStringPtr[i]);

    found = strstr(workStringPtr,"FA");

    if (found == 0) {
        found = strstr(workStringPtr,"IF");
    }

    if (found != 0) {
        for (i = 0; i <= 5; i++) {
            freqStr[i] = found[i + 4];
        }

        freqStr[i] = 0;
        freqLong = atol(freqStr);

        if (freqLong >= 1750 && freqLong <= 2100) {
            BAND = 10;
        } else if (freqLong >= 3200 && freqLong <= 4200) {
            BAND = 9;
        } else if (freqLong >= 5000 && freqLong <= 5500) {
            BAND = 8;
        } else if (freqLong >= 6900 && freqLong <= 7500) {
            BAND = 7;
        } else if (freqLong >= 10000 && freqLong <= 10200) {
            BAND = 6;
        } else if (freqLong >= 13900 && freqLong <= 14500) {
            BAND = 5;
        } else if (freqLong >= 18000 && freqLong <= 18200) {
            BAND = 4;
        } else if (freqLong >= 20900 && freqLong <= 21500) {
            BAND = 3;
        } else if (freqLong >= 24840 && freqLong <= 25100) {
            BAND = 2;
        } else if (freqLong >= 27900 && freqLong <= 29800) {
            BAND = 1;
        } else {
            BAND = 0;
        }//endif

        if (BAND != OBAND) SetBand();
    }

    found = strstr(workStringPtr,"HR");

    if (found != 0) {

        //set band:
        found = strstr(workStringPtr,"HRBN");

        if (found != 0) {
            if (found[4] == ';') {
                UART_send(uart, (char *)"HRBN");
                UART_send_num(uart, BAND);
                UART_send_line(uart);
            } else if (found[5] == ';') BAND = found[4] - 0x30;
            else BAND = (found[4] - 0x30) * 10 + (found[5] - 0x30);

            if (BAND > 10) BAND = 0;

            if (BAND != OBAND) SetBand();
        }

        //set mode:
        found = strstr(workStringPtr,"HRMD");

        if (found != 0) {
            if (found[4] == ';') {
                UART_send(uart, (char *)"HRMD");
                UART_send_num(uart, MODE);
                UART_send_line(uart);
            }

            if (found[4] == '0') {
                MODE = 0;
                EEPROM.write(eemode, MODE);
                DrawMode();
                Disable11();
            }

            if (found[4] == '1') {
                MODE = 1;
                EEPROM.write(eemode, MODE);
                DrawMode();
                Enable11();
            }
        }

        //set antenna:
        found = strstr(workStringPtr,"HRAN");

        if (found != 0) {
            if (found[4] == ';') {
                UART_send(uart, (char *)"HRAN");
                UART_send_num(uart, ANTSEL[BAND]);
                UART_send_line(uart);
            }

            if (found[4] == '1') {
                ANTSEL[BAND] = 1;
                EEPROM.write(eeantsel+BAND, ANTSEL[BAND]);
                DrawAnt();
            }

            if (found[4] == '2') {
                ANTSEL[BAND] = 2;
                EEPROM.write(eeantsel+BAND, ANTSEL[BAND]);
                DrawAnt();
            }
        }

        //set temp F or C:
        found = strstr(workStringPtr,"HRTS");

        if (found != 0) {
            if (found[4] == ';') {
                UART_send(uart, (char *)"HRTS");

                if (CELSIUS == 0) UART_send(uart, (char *)"F");
                else UART_send(uart, (char *)"C");

                UART_send_line(uart);
            }

            if (found[4] == 'F') {
                CELSIUS = 0;
                EEPROM.write(eecelsius, CELSIUS);
            }

            if (found[4] == 'C') {
                CELSIUS = 1;
                EEPROM.write(eecelsius, CELSIUS);
            }
        }

        //read volts:
        found = strstr(workStringPtr,"HRVT");

        if (found != 0) {
            char vbuff[4];
            UART_send(uart, (char *)"HRVT");
            sprintf(vbuff, "%2d", Read_Voltage()/40);
            UART_send(uart, vbuff);
            UART_send_line(uart);
        }

        //read ATU_P:
        found = strstr(workStringPtr,"HRAP");

        if (found != 0) {
            UART_send(uart, (char *)"HRAP");
            UART_send_num(uart, ATU_P);
            UART_send_line(uart);
        }

        //bypass activate the ATU:
        found = strstr(workStringPtr,"HRTB");

        if (found != 0) {
            if (found[4] == ';') {
                UART_send(uart, (char *)"HRTB");
                UART_send_num(uart, ATU);
                UART_send_line(uart);
            }

            if (found[4] == '1') {
                ATU = 1;
                DrawATU();
            }

            if (found[4] == '0') {
                ATU = 0;
                DrawATU();
            }
        }

        //Press the TUNE button
        found = strstr(workStringPtr,"HRTU");

        if (found != 0 && ATU_P == 1) {
            Tune_button();
        }

        //Tune Result
        found = strstr(workStringPtr,"HRTR");

        if (found != 0) {
            UART_send(uart, (char *)"HRTR");
            UART_send_char(uart, ATU_STAT);
            UART_send_line(uart);
        }

        //Is ATU Tuning?
        found = strstr(workStringPtr,"HRTT");

        if (found != 0) {
            UART_send(uart, (char *)"HRTT");
            UART_send_num(uart, TUNING);
            UART_send_line(uart);
        }

        //read temp:
        found = strstr(workStringPtr,"HRTP");

        if (found != 0) {
            char tbuff[4];
            int tread;

            if (CELSIUS == 0) tread = ((t_ave * 9) / 5) + 320;
            else tread = t_ave;

            tread /= 10;
            UART_send(uart, (char *)"HRTP");

            if (CELSIUS == 0) sprintf(tbuff, "%dF", tread);
            else sprintf(tbuff, "%dC", tread);

            UART_send(uart, tbuff);
            UART_send_line(uart);
        }

        /*read power
         HRPWF; = Forward
         HRPWR; = Reverse
         HRPWD; = Drive
         HRPWV; = VSWR */
        found = strstr(workStringPtr,"HRPW");

        if (found != 0) {
            byte psel, pwf = 0;
            char schr, tbuff[10];

            schr = found[4];

            if (schr == 'F') {
                psel = fwd_p;
                pwf = 1;
            }

            if (schr == 'R') {
                psel = rfl_p;
                pwf = 1;
            }

            if (schr == 'D') {
                psel = drv_p;
                pwf = 1;
            }

            if (schr == 'V') {
                psel = vswr;
                pwf = 1;
            }

            if (pwf == 1) {
                unsigned int reading = Read_Power(psel);
                sprintf(tbuff, "HRPW%c%03d", schr, reading);
                UART_send(uart, tbuff);
                UART_send_line(uart);
            }
        }

        //report status
        found = strstr(workStringPtr,"HRST");

        if (found != 0) {
            char tbuff[80];

            unsigned int stF = Read_Power(fwd_p);
            unsigned int stR = Read_Power(rfl_p);
            unsigned int stD = Read_Power(drv_p);
            unsigned int stS = Read_Power(vswr);
            unsigned int stV = Read_Voltage()/40;
            unsigned int stI = Read_Current()/20;
            unsigned int stT = t_ave/10;

            if (CELSIUS == 0) stT = ((stT * 9) / 5) + 32;

            sprintf(tbuff, "HRST-%03d-%03d-%03d-%03d-%03d-%03d-%03d-%01d-%02d-%01d-%01d%01d%01d%01d%01d%01d%01d%01d-",
                    stF, stR, stD, stS, stV, stI, stT, MODE, BAND, ANTSEL[BAND], TX, F_alert, R_alert, D_alert, V_alert, I_alert, TUNING, ATU);
            UART_send(uart, tbuff);
            UART_send_line(uart);
        }

        //respond and acknowlege:
        found = strstr(workStringPtr,"HRAA");

        if (found != 0) {
            UART_send(uart, (char *)"HRAA");
            UART_send_line(uart);
        }

        //get new firmware
        found = strstr(workStringPtr,"HRFW");

        if (found != 0) {
            Serial.end();
            delay(50);
            Serial.begin(115200);

            while (!Serial.available());

            delay(50);
            digitalWrite(8, LOW);
        }

        //communicate with ATU
        found = strstr(workStringPtr,"HRTM");

        if (found != 0 && ATU_P == 1) {
            char tm_buff[20];
            byte tmb_p = 0;
            size_t b_len;

            while (found[tmb_p + 4] != ';' && tmb_p < 18) {
                tm_buff[tmb_p] = found[tmb_p + 4];
                tmb_p++;
            }

            tm_buff[tmb_p] = 0;
            Serial3.setTimeout(75);
            Serial3.print('*');
            Serial3.println(tm_buff);
            b_len = Serial3.readBytesUntil(13, ATU_buff, 40);
            ATU_buff[b_len] = 0;

            if (b_len > 0) {
                UART_send(uart, (char *)"HRTM");
                UART_send(uart, ATU_buff);
                UART_send_line(uart);
            }
        }
    }
}

void UART_send(char uart, char *message) {
    if (uart == 1) Serial.print(message);

    if (uart == 2) Serial2.print(message);
}

void UART_send_num(char uart, int num) {
    if (uart == 1) Serial.print(num);

    if (uart == 2) Serial2.print(num);
}

void UART_send_char(char uart, char num) {
    if (uart == 1) Serial.print(num);

    if (uart == 2) Serial2.print(num);
}

void UART_send_line(char uart) {
    if (uart == 1) Serial.println(";");

    if (uart == 2) Serial2.println(";");
}

void UART_send_cr(char uart) {
    if (uart == 1) Serial.println();

    if (uart == 2) Serial2.println();
}
