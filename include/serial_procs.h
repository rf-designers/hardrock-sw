#pragma once

void uartGrabBuffer();
void uartGrabBuffer2();
void findBand(short uart);
void UART_send(char uart, char *message);
void UART_send_num(char uart, int num);
void UART_send_char(char uart, char num);
void UART_send_line(char uart);
void UART_send_cr(char uart);
