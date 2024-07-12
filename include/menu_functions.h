#pragma once

#include <amp_state.h>

void menu_update(byte item, byte ch_dir);
void menuSelect();
void setup_acc_serial(serial_speed speed);
void setup_usb_serial(serial_speed speed);