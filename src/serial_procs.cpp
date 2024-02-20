#include "serial_procs.h"
#include <Arduino.h>
#include <EEPROM.h>
#include "HR500V1.h"
#include "hr500_displays.h"
#include <amp_state.h>
#include "hr500_sensors.h"

long freqLong = 0;
unsigned short oldBand;
unsigned int old_tuner_freq;
const char crlfsemi[] = ";\r\n";
const char HRTM[] = "HRTM";
char freqStr[6];


extern char rxbuff[128]; // 128 byte circular Buffer for storing rx data
extern char workingString[128];
extern char rxbuff2[128]; // 128 byte circular Buffer for storing rx data
extern char workingString2[128];
extern unsigned int uartMsgs, uartMsgs2, readStart, readStart2;
extern byte OBAND;
extern char ATU_STAT;
extern unsigned int t_tot, t_ave;
extern byte I_alert, V_alert, F_alert, R_alert, D_alert;
extern byte OI_alert, OV_alert, OF_alert, OR_alert, OD_alert;
extern char ATU_buff[40];
extern amp_state state;

void SetBand(void);

void DisablePTTDetector(void);

void EnablePTTDetector(void);

void TuneButtonPressed(void);

void uartGrabBuffer() {
    int z = 0;
    char lastChar[1];
    lastChar[0] = 'C';

    while (lastChar[0] != 0x3B) {
        workingString[z] = rxbuff[readStart];
        rxbuff[readStart] = 0x00;
        readStart++;
        lastChar[0] = workingString[z];
        z++;

        if (readStart > 127)readStart = 0;
    } //endwhile

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
        lastChar[0] = workingString2[z];
        z++;

        if (readStart2 > 127) readStart2 = 0;
    } //endwhile

    workingString2[z] = 0x00;
}


void findBand(char uart) {
    char* found;
    char* workStringPtr;
    int i, wsl;

    if (uart == 1) {
        workStringPtr = workingString;
    } else {
        workStringPtr = workingString2;
    }

    // convert working string to uppercase...
    wsl = strlen(workStringPtr);

    for (i = 0; i <= wsl; i++) workStringPtr[i] = toupper(workStringPtr[i]);

    found = strstr(workStringPtr, "FA");

    if (found == 0) {
        found = strstr(workStringPtr, "IF");
    }

    if (found != 0) {
        for (i = 0; i <= 5; i++) {
            freqStr[i] = found[i + 4];
        }

        freqStr[i] = 0;
        freqLong = atol(freqStr);

        if (freqLong >= 1750 && freqLong <= 2100) {
            state.band = 10;
        } else if (freqLong >= 3200 && freqLong <= 4200) {
            state.band = 9;
        } else if (freqLong >= 5000 && freqLong <= 5500) {
            state.band = 8;
        } else if (freqLong >= 6900 && freqLong <= 7500) {
            state.band = 7;
        } else if (freqLong >= 10000 && freqLong <= 10200) {
            state.band = 6;
        } else if (freqLong >= 13900 && freqLong <= 14500) {
            state.band = 5;
        } else if (freqLong >= 18000 && freqLong <= 18200) {
            state.band = 4;
        } else if (freqLong >= 20900 && freqLong <= 21500) {
            state.band = 3;
        } else if (freqLong >= 24840 && freqLong <= 25100) {
            state.band = 2;
        } else if (freqLong >= 27900 && freqLong <= 29800) {
            state.band = 1;
        } else {
            state.band = 0;
        }

        if (state.band != OBAND) SetBand();
    }

    found = strstr(workStringPtr, "HR");

    if (found != 0) {
        //set band:
        found = strstr(workStringPtr, "HRBN");

        if (found != 0) {
            if (found[4] == ';') {
                UART_send(uart, "HRBN");
                UART_send_num(uart, state.band);
                UART_send_line(uart);
            } else if (found[5] == ';') {
                state.band = found[4] - 0x30;
            } else {
                state.band = (found[4] - 0x30) * 10 + (found[5] - 0x30);
            }

            if (state.band > 10) state.band = 0;
            if (state.band != OBAND) SetBand();
        }

        //set mode:
        found = strstr(workStringPtr, "HRMD");

        if (found != 0) {
            if (found[4] == ';') {
                UART_send(uart, (char *) "HRMD");
                UART_send_num(uart, state.mode);
                UART_send_line(uart);
            }

            if (found[4] == '0') {
                state.mode = 0;
                EEPROM.write(eemode, state.mode);
                DrawMode();
                DisablePTTDetector();
            }

            if (found[4] == '1') {
                state.mode = 1;
                EEPROM.write(eemode, state.mode);
                DrawMode();
                EnablePTTDetector();
            }
        }

        //set antenna:
        found = strstr(workStringPtr, "HRAN");

        if (found != 0) {
            if (found[4] == ';') {
                UART_send(uart, (char *) "HRAN");
                UART_send_num(uart, state.antForBand[state.band]);
                UART_send_line(uart);
            }

            if (found[4] == '1') {
                state.antForBand[state.band] = 1;
                EEPROM.write(eeantsel + state.band, state.antForBand[state.band]);
                DrawAnt();
            }

            if (found[4] == '2') {
                state.antForBand[state.band] = 2;
                EEPROM.write(eeantsel + state.band, state.antForBand[state.band]);
                DrawAnt();
            }
        }

        // set temp F or C:
        found = strstr(workStringPtr, "HRTS");
        if (found != nullptr) {
            if (found[4] == ';') {
                UART_send(uart, "HRTS");
                if (state.tempInCelsius) {
                    UART_send(uart, "C");
                } else {
                    UART_send(uart, "F");
                }
                UART_send_line(uart);
            }

            if (found[4] == 'F') {
                state.tempInCelsius = false;
                EEPROM.write(eecelsius, state.tempInCelsius ? 1 : 0);
            }

            if (found[4] == 'C') {
                state.tempInCelsius = true;
                EEPROM.write(eecelsius, state.tempInCelsius ? 1 : 0);
            }
        }

        // read volts:
        found = strstr(workStringPtr, "HRVT");

        if (found != 0) {
            char vbuff[4];
            UART_send(uart, (char *) "HRVT");
            sprintf(vbuff, "%2d", ReadVoltage() / 40);
            UART_send(uart, vbuff);
            UART_send_line(uart);
        }

        //read ATU_P:
        found = strstr(workStringPtr, "HRAP");

        if (found != 0) {
            UART_send(uart, "HRAP");
            UART_send_num(uart, state.atuIsPresent);
            UART_send_line(uart);
        }

        //bypass activate the ATU:
        found = strstr(workStringPtr, "HRTB");

        if (found != 0) {
            if (found[4] == ';') {
                UART_send(uart, "HRTB");
                UART_send_num(uart, state.atuActive);
                UART_send_line(uart);
            }

            if (found[4] == '1') {
                state.atuActive = true;
                DrawATU();
            }

            if (found[4] == '0') {
                state.atuActive = false;
                DrawATU();
            }
        }

        //Press the TUNE button
        found = strstr(workStringPtr, "HRTU");

        if (found != 0 && state.atuIsPresent == 1) {
            TuneButtonPressed();
        }

        //Tune Result
        found = strstr(workStringPtr, "HRTR");

        if (found != 0) {
            UART_send(uart, (char *) "HRTR");
            UART_send_char(uart, ATU_STAT);
            UART_send_line(uart);
        }

        //Is ATU Tuning?
        found = strstr(workStringPtr, "HRTT");

        if (found != 0) {
            UART_send(uart, (char *) "HRTT");
            UART_send_num(uart, state.isTuning);
            UART_send_line(uart);
        }

        // read temp:
        found = strstr(workStringPtr, "HRTP");
        if (found != nullptr) {
            char tbuff[4];
            int tread;

            if (state.tempInCelsius) {
                tread = t_ave;
            } else {
                tread = ((t_ave * 9) / 5) + 320;
            }

            tread /= 10;
            UART_send(uart, "HRTP");

            if (state.tempInCelsius) {
                sprintf(tbuff, "%dC", tread);
            } else {
                sprintf(tbuff, "%dF", tread);
            }

            UART_send(uart, tbuff);
            UART_send_line(uart);
        }

        /*read power
         HRPWF; = Forward
         HRPWR; = Reverse
         HRPWD; = Drive
         HRPWV; = VSWR */
        found = strstr(workStringPtr, "HRPW");
        if (found != nullptr) {
            byte psel = 0, pwf = 0;

            const char schr = found[4];
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
                char tbuff[10];
                unsigned int reading = ReadPower(psel);
                sprintf(tbuff, "HRPW%c%03d", schr, reading);
                UART_send(uart, tbuff);
                UART_send_line(uart);
            }
        }

        // report status
        found = strstr(workStringPtr, "HRST");
        if (found != nullptr) {
            char tbuff[80];

            const auto stF = ReadPower(fwd_p);
            const auto stR = ReadPower(rfl_p);
            const auto stD = ReadPower(drv_p);
            const auto stS = ReadPower(vswr);
            const auto stV = ReadVoltage() / 40;
            const auto stI = ReadCurrent() / 20;
            auto stT = t_ave / 10;
            if (!state.tempInCelsius) {
                stT = ((stT * 9) / 5) + 32;
            }

            sprintf(tbuff, "HRST-%03d-%03d-%03d-%03d-%03d-%03d-%03d-%01d-%02d-%01d-%01d%01d%01d%01d%01d%01d%01d%01d-",
                    stF, stR, stD, stS, stV, stI, stT, state.mode, state.band, state.antForBand[state.band],
                    state.txIsOn, F_alert,
                    R_alert, D_alert,
                    V_alert, I_alert, state.isTuning, state.atuActive);
            UART_send(uart, tbuff);
            UART_send_line(uart);
        }

        // respond and acknowlege:
        found = strstr(workStringPtr, "HRAA");
        if (found != nullptr) {
            UART_send(uart, "HRAA");
            UART_send_line(uart);
        }

        // get new firmware
        found = strstr(workStringPtr, "HRFW");
        if (found != nullptr) {
            Serial.end();
            delay(50);
            Serial.begin(115200);

            while (!Serial.available());

            delay(50);
            digitalWrite(8, LOW);
        }

        // communicate with ATU
        found = strstr(workStringPtr, "HRTM");
        if (found != nullptr && state.atuIsPresent) {
            char tm_buff[20];
            byte tmb_p = 0;

            while (found[tmb_p + 4] != ';' && tmb_p < 18) {
                tm_buff[tmb_p] = found[tmb_p + 4];
                tmb_p++;
            }

            tm_buff[tmb_p] = 0;
            Serial3.setTimeout(75);
            Serial3.print('*');
            Serial3.println(tm_buff);
            const size_t b_len = Serial3.readBytesUntil(13, ATU_buff, 40);
            ATU_buff[b_len] = 0;

            if (b_len > 0) {
                UART_send(uart, "HRTM");
                UART_send(uart, ATU_buff);
                UART_send_line(uart);
            }
        }
    }
}

void UART_send(char uart, const char* message) {
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
