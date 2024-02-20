#pragma once

void uartGrabBuffer();
void uartGrabBuffer2();
void findBand(char uart);
void UART_send(char uart, const char *message);
void UART_send_num(char uart, int num);
void UART_send_char(char uart, char num);
void UART_send_line(char uart);
void UART_send_cr(char uart);
