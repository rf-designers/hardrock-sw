#include "serial_procs.h"
#include <Arduino.h>
#include <EEPROM.h>
#include "HR500V1.h"
#include "hr500_displays.h"
#include <amp_state.h>
#include <atu_functions.h>

#include "hr500_sensors.h"
// #include <ArduinoLowPower.h>
#include <avr/sleep.h>
#include <avr/power.h>

long freqLong = 0;
char freqStr[7];

volatile uint8_t uartIdx = 0, uart2Idx = 0;
volatile uint8_t readStart = 0, readStart2 = 0;
volatile char rxbuff[128]; // 128 byte circular Buffer for storing rx data
volatile char workingString[128];
volatile char rxbuff2[128]; // 128 byte circular Buffer for storing rx data
volatile char workingString2[128];


//extern char rxbuff[128]; // 128 byte circular Buffer for storing rx data
//extern char workingString[128];
//extern char rxbuff2[128]; // 128 byte circular Buffer for storing rx data
//extern char workingString2[128];
//extern uint8_t readStart, readStart2;
extern char ATU_STAT;
extern char ATU_buff[40];
extern amplifier amp;

extern void go_to_sleep();

void uart_grab_buffer() {
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

void uart_grab_buffer2() {
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


void handle_usb_message(char uart) {
    char *workStringPtr;
    int i;

    if (uart == 1) {
        workStringPtr = const_cast<char *>(workingString);
    } else {
        workStringPtr = const_cast<char *>(workingString2);
    }

    // convert working string to uppercase...
    const int wsl = strlen(workStringPtr);
    for (i = 0; i <= wsl; i++)
        workStringPtr[i] = toupper(workStringPtr[i]);

    char *found = strstr(workStringPtr, "FA");
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
            amp.state.band = 10;
        } else if (freqLong >= 3200 && freqLong <= 4200) {
            amp.state.band = 9;
        } else if (freqLong >= 5000 && freqLong <= 5500) {
            amp.state.band = 8;
        } else if (freqLong >= 6900 && freqLong <= 7500) {
            amp.state.band = 7;
        } else if (freqLong >= 10000 && freqLong <= 10200) {
            amp.state.band = 6;
        } else if (freqLong >= 13900 && freqLong <= 14500) {
            amp.state.band = 5;
        } else if (freqLong >= 18000 && freqLong <= 18200) {
            amp.state.band = 4;
        } else if (freqLong >= 20900 && freqLong <= 21500) {
            amp.state.band = 3;
        } else if (freqLong >= 24840 && freqLong <= 25100) {
            amp.state.band = 2;
        } else if (freqLong >= 27900 && freqLong <= 29800) {
            amp.state.band = 1;
        } else {
            amp.state.band = 0;
        }

        if (amp.state.band != amp.state.oldBand)
            amp.set_band();
    }

    found = strstr(workStringPtr, "HR");
    if (found != nullptr) {
        //set band:
        found = strstr(workStringPtr, "HRBN");
        if (found != nullptr) {
            if (found[4] == ';') {
                UART_send(uart, "HRBN");
                UART_send_num(uart, amp.state.band);
                UART_send_line(uart);
            } else if (found[5] == ';') {
                amp.state.band = found[4] - 0x30;
            } else {
                amp.state.band = (found[4] - 0x30) * 10 + (found[5] - 0x30);
            }

            if (amp.state.band > 10) amp.state.band = 0;
            if (amp.state.band != amp.state.oldBand) amp.set_band();
        }

        // set mode:
        found = strstr(workStringPtr, "HRMD");
        if (found != nullptr) {
            if (found[4] == ';') {
                UART_send(uart, "HRMD");
                UART_send_num(uart, mode_to_eeprom(amp.state.mode));
                UART_send_line(uart);
            }

            if (found[4] == '0') {
                amp.state.mode = mode_type::standby;
                EEPROM.write(eemode, mode_to_eeprom(amp.state.mode));
                draw_ptt_mode();
                amp.disable_ptt_detector();
            }

            if (found[4] == '1') {
                amp.state.mode = mode_type::ptt;
                EEPROM.write(eemode, mode_to_eeprom(amp.state.mode));
                draw_ptt_mode();
                amp.enable_ptt_detector();
            }
        }

        // set antenna:
        found = strstr(workStringPtr, "HRAN");
        if (found != nullptr) {
            if (found[4] == ';') {
                UART_send(uart, "HRAN");
                UART_send_num(uart, amp.state.antForBand[amp.state.band]);
                UART_send_line(uart);
            }

            if (found[4] == '1') {
                amp.state.antForBand[amp.state.band] = 1;
                EEPROM.write(eeantsel + amp.state.band, amp.state.antForBand[amp.state.band]);
                draw_ant();
            }

            if (found[4] == '2') {
                amp.state.antForBand[amp.state.band] = 2;
                EEPROM.write(eeantsel + amp.state.band, amp.state.antForBand[amp.state.band]);
                draw_ant();
            }
        }

        // set temp F or C:
        found = strstr(workStringPtr, "HRTS");
        if (found != nullptr) {
            if (found[4] == ';') {
                UART_send(uart, "HRTS");
                if (amp.state.tempInCelsius) {
                    UART_send(uart, "C");
                } else {
                    UART_send(uart, "F");
                }
                UART_send_line(uart);
            }

            if (found[4] == 'F') {
                amp.state.tempInCelsius = false;
                EEPROM.write(eecelsius, amp.state.tempInCelsius ? 1 : 0);
            }

            if (found[4] == 'C') {
                amp.state.tempInCelsius = true;
                EEPROM.write(eecelsius, amp.state.tempInCelsius ? 1 : 0);
            }
        }

        // read volts:
        found = strstr(workStringPtr, "HRVT");
        if (found != nullptr) {
            char vbuff[4];
            UART_send(uart, "HRVT");
            sprintf(vbuff, "%2d", read_voltage() / 40);
            UART_send(uart, vbuff);
            UART_send_line(uart);
        }

        // read ATU_P:
        found = strstr(workStringPtr, "HRAP");
        if (found != nullptr) {
            UART_send(uart, "HRAP");
            UART_send_num(uart, amp.atu.is_present());
            UART_send_line(uart);
        }

        // bypass activate the ATU:
        found = strstr(workStringPtr, "HRTB");
        if (found != nullptr) {
            if (found[4] == ';') {
                UART_send(uart, "HRTB");
                UART_send_num(uart, amp.atu.is_active());
                UART_send_line(uart);
            }
            if (found[4] == '1') {
                amp.atu.set_active(true);
                draw_atu();
            }
            if (found[4] == '0') {
                amp.atu.set_active(false);
                draw_atu();
            }
        }

        // Press the TUNE button
        found = strstr(workStringPtr, "HRTU");
        if (found != nullptr && amp.atu.is_present()) {
            on_tune_button_pressed();
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
            UART_send_num(uart, amp.atu.is_tuning());
            UART_send_line(uart);
        }

        // read temp:
        found = strstr(workStringPtr, "HRTP");
        if (found != nullptr) {
            char tbuff[4];
            int tread;

            if (amp.state.tempInCelsius) {
                tread = amp.state.temperature.get();
            } else {
                tread = ((amp.state.temperature.get() * 9) / 5) + 320;
            }

            tread /= 10;
            UART_send(uart, "HRTP");

            if (amp.state.tempInCelsius) {
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
                unsigned int reading = read_power(psel);
                sprintf(tbuff, "HRPW%c%03d", schr, reading);
                UART_send(uart, tbuff);
                UART_send_line(uart);
            }
        }

        // report status
        found = strstr(workStringPtr, "HRST");
        if (found != nullptr) {
            char tbuff[80];

            const auto stF = read_power(power_type::fwd_p);
            const auto stR = read_power(power_type::rfl_p);
            const auto stD = read_power(power_type::drv_p);
            const auto stS = read_power(power_type::vswr);
            const auto stV = read_voltage() / 40;
            const auto stI = read_current() / 20;
            auto stT = amp.state.temperature.get() / 10;
            if (!amp.state.tempInCelsius) {
                stT = ((stT * 9) / 5) + 32;
            }

            sprintf(tbuff, "HRST-%03d-%03d-%03d-%03d-%03d-%03d-%03d-%01d-%02d-%01d-%01d%01d%01d%01d%01d%01d%01d%01d-",
                    stF, stR, stD, stS, stV, stI, stT, mode_to_eeprom(amp.state.mode), amp.state.band,
                    amp.state.antForBand[amp.state.band],
                    amp.state.tx_is_on ? 1 : 0,
                    amp.state.alerts[alert_fwd_pwr],
                    amp.state.alerts[alert_rfl_pwr],
                    amp.state.alerts[alert_drive_pwr],
                    amp.state.alerts[alert_vdd],
                    amp.state.alerts[alert_idd],
                    amp.atu.is_tuning() ? 1 : 0,
                    amp.atu.is_active() ? 1 : 0);
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
            prepare_for_fw_update();
        }

        // communicate with ATU
        found = strstr(workStringPtr, "HRTM");
        if (found != nullptr && amp.atu.is_present()) {
            char atuCmd[20] = {'*'};
            size_t cmdIdx = 1;

            while (found[cmdIdx + 3] != ';' && cmdIdx < 18) {
                atuCmd[cmdIdx] = found[cmdIdx + 3];
                cmdIdx++;
            }
            atuCmd[cmdIdx] = 0;

            char atuResponse[40];
            auto readBytes = amp.atu.query(atuCmd, atuResponse, 40);
            if (readBytes > 0) {
                UART_send(uart, "HRTM");
                UART_send(uart, atuResponse);
                UART_send_line(uart);
            }
        }

        // debug ATU comm
        found = strstr(workStringPtr, "HRAD");
        if (found != nullptr) {
            UART_send(uart, "Last ATU response has ");
            UART_send_num(uart, amp.atu.last_response_size);
            UART_send(uart, " bytes. Content: [");
            for (size_t i = 0; i < amp.atu.last_response_size; i++) {
                if (isprint(amp.atu.last_response[i])) {
                    UART_send_char(uart, amp.atu.last_response[i]);
                } else {
                    UART_send(uart, "(");
                    UART_send_num(uart, amp.atu.last_response[i]);
                    UART_send(uart, ")");
                }
            }
            UART_send(uart, "]");
            UART_send_line(uart);
        }

        found = strstr(workStringPtr, "HRXX");
        if (found != nullptr) {
            go_to_sleep();
        }

    }
}

void UART_send(const char uart, const char *message) {
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

void handle_acc_comms() {
    while (Serial2.available()) {
        rxbuff2[uart2Idx] = Serial2.read();
        if (rxbuff2[uart2Idx] == ';') {
            uart_grab_buffer2();
            handle_usb_message(2);
        }

        if (++uart2Idx > 127)
            uart2Idx = 0;
    }
}

void handle_usb_comms() {
    while (Serial.available()) {
        rxbuff[uartIdx] = Serial.read(); // Storing read data
        if (rxbuff[uartIdx] == ';') {
            uart_grab_buffer();
            handle_usb_message(1);
        }

        if (++uartIdx > 127)
            uartIdx = 0;
    }
}
