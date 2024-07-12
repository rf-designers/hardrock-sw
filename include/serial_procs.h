#pragma once

void uart_grab_buffer();
void uart_grab_buffer2();
void handle_usb_message(char uart);
void UART_send(char uart, const char *message);
void UART_send_num(char uart, int num);
void UART_send_char(char uart, char num);
void UART_send_line(char uart);
void UART_send_cr(char uart);
