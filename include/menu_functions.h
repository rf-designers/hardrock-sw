#pragma once

#include <amp_state.h>

void menu_update(byte item, byte ch_dir);
void menuSelect();
void SetupAccSerial(serial_speed speed);
void SetupUSBSerial(serial_speed speed);