#include "serial_procs.h"
#include <Arduino.h>
#include <EEPROM.h>
#include "HR500V1.h"
#include "hr500_displays.h"
#include <amp_state.h>
#include <atu_functions.h>

#include "hr500_sensors.h"

long freqLong = 0;
unsigned short oldBand;
unsigned int old_tuner_freq;
const char crlfsemi[] = ";\r\n";
const char HRTM[] = "HRTM";
char freqStr[7];


extern char rxbuff[128]; // 128 byte circular Buffer for storing rx data
extern char workingString[128];
extern char rxbuff2[128]; // 128 byte circular Buffer for storing rx data
extern char workingString2[128];
extern uint8_t readStart, readStart2;
extern byte OBAND;
extern char ATU_STAT;
extern unsigned int t_tot, t_ave;
extern byte I_alert, V_alert, F_alert, R_alert, D_alert;
extern byte OI_alert, OV_alert, OF_alert, OR_alert, OD_alert;
extern char ATU_buff[40];
extern amp_state state;

void setBand();

void disablePTTDetector();

void EnablePTTDetector();

void TuneButtonPressed();

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


void handleSerialMessage(char uart) {
    char* workStringPtr;
    int i;

    if (uart == 1) {
        workStringPtr = workingString;
    } else {
        workStringPtr = workingString2;
    }

    // convert working string to uppercase...
    const int wsl = strlen(workStringPtr);
    for (i = 0; i <= wsl; i++)
        workStringPtr[i] = toupper(workStringPtr[i]);

    char* found = strstr(workStringPtr, "FA");
    if (found == nullptr) {
        found = strstr(workStringPtr, "IF");
    }
    if (found != nullptr) {
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

        if (state.band != OBAND)
            setBand();
    }

    found = strstr(workStringPtr, "HR");
    if (found != nullptr) {
        //set band:
        found = strstr(workStringPtr, "HRBN");
        if (found != nullptr) {
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
            if (state.band != OBAND) setBand();
        }

        // set mode:
        found = strstr(workStringPtr, "HRMD");
        if (found != nullptr) {
            if (found[4] == ';') {
                UART_send(uart, "HRMD");
                UART_send_num(uart, modeToEEPROM(state.mode));
                UART_send_line(uart);
            }

            if (found[4] == '0') {
                state.mode = mode_type::standby;
                EEPROM.write(eemode, modeToEEPROM(state.mode));
                DrawMode();
                disablePTTDetector();
            }

            if (found[4] == '1') {
                state.mode = mode_type::ptt;
                EEPROM.write(eemode, modeToEEPROM(state.mode));
                DrawMode();
                EnablePTTDetector();
            }
        }

        // set antenna:
        found = strstr(workStringPtr, "HRAN");
        if (found != nullptr) {
            if (found[4] == ';') {
                UART_send(uart, "HRAN");
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
        if (found != nullptr) {
            char vbuff[4];
            UART_send(uart, "HRVT");
            sprintf(vbuff, "%2d", ReadVoltage() / 40);
            UART_send(uart, vbuff);
            UART_send_line(uart);
        }

        // read ATU_P:
        found = strstr(workStringPtr, "HRAP");
        if (found != nullptr) {
            UART_send(uart, "HRAP");
            UART_send_num(uart, state.isAtuPresent);
            UART_send_line(uart);
        }

        // bypass activate the ATU:
        found = strstr(workStringPtr, "HRTB");
        if (found != nullptr) {
            if (found[4] == ';') {
                UART_send(uart, "HRTB");
                UART_send_num(uart, state.isAtuActive);
                UART_send_line(uart);
            }
            if (found[4] == '1') {
                state.isAtuActive = true;
                DrawATU();
            }
            if (found[4] == '0') {
                state.isAtuActive = false;
                DrawATU();
            }
        }

        // Press the TUNE button
        found = strstr(workStringPtr, "HRTU");
        if (found != nullptr && state.isAtuPresent) {
            TuneButtonPressed();
        }

        // Tune Result
        found = strstr(workStringPtr, "HRTR");
        if (found != nullptr) {
            UART_send(uart, "HRTR");
            UART_send_char(uart, ATU_STAT);
            UART_send_line(uart);
        }

        // Is ATU Tuning?
        found = strstr(workStringPtr, "HRTT");
        if (found != nullptr) {
            UART_send(uart, "HRTT");
            UART_send_num(uart, state.isAtuTuning);
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
            auto psel = power_type::fwd_p;
            byte pwf = 0;

            const char schr = found[4];
            if (schr == 'F') {
                psel = power_type::fwd_p;
                pwf = 1;
            }

            if (schr == 'R') {
                psel = power_type::rfl_p;
                pwf = 1;
            }

            if (schr == 'D') {
                psel = power_type::drv_p;
                pwf = 1;
            }

            if (schr == 'V') {
                psel = power_type::vswr;
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

            const auto stF = ReadPower(power_type::fwd_p);
            const auto stR = ReadPower(power_type::rfl_p);
            const auto stD = ReadPower(power_type::drv_p);
            const auto stS = ReadPower(power_type::vswr);
            const auto stV = ReadVoltage() / 40;
            const auto stI = ReadCurrent() / 20;
            auto stT = t_ave / 10;
            if (!state.tempInCelsius) {
                stT = ((stT * 9) / 5) + 32;
            }

            sprintf(tbuff, "HRST-%03d-%03d-%03d-%03d-%03d-%03d-%03d-%01d-%02d-%01d-%01d%01d%01d%01d%01d%01d%01d%01d-",
                    stF, stR, stD, stS, stV, stI, stT, modeToEEPROM(state.mode), state.band, state.antForBand[state.band],
                    state.txIsOn ? 1 : 0, F_alert,
                    R_alert, D_alert,
                    V_alert, I_alert, state.isAtuTuning ? 1 : 0, state.isAtuActive ? 1 : 0);
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
            PrepareForFWUpdate();
        }

        // communicate with ATU
        found = strstr(workStringPtr, "HRTM");
        if (found != nullptr && state.isAtuPresent) {
            static char atuCmd[20] = {'*'};
            size_t cmdIdx = 1;

            while (found[cmdIdx + 3] != ';' && cmdIdx < 18) {
                atuCmd[cmdIdx] = found[cmdIdx + 3];
                cmdIdx++;
            }
            atuCmd[cmdIdx] = 0;

            static char atuResponse[40];
            auto readBytes = ATUQuery(atuCmd, atuResponse, 40);
            if (readBytes > 0) {
                UART_send(uart, "HRTM");
                UART_send(uart, atuResponse);
                UART_send_line(uart);
            }
        }
    }
}

void UART_send(const char uart, const char* message) {
    if (uart == 1) Serial.print(message);
    if (uart == 2) Serial2.print(message);
}

void UART_send_num(const char uart, const int num) {
    if (uart == 1) Serial.print(num);
    if (uart == 2) Serial2.print(num);
}

void UART_send_char(const char uart, const char num) {
    if (uart == 1) Serial.print(num);
    if (uart == 2) Serial2.print(num);
}

void UART_send_line(const char uart) {
    if (uart == 1) Serial.println(";");
    if (uart == 2) Serial2.println(";");
}

void UART_send_cr(const char uart) {
    if (uart == 1) Serial.println();
    if (uart == 2) Serial2.println();
}
