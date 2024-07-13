#pragma once

#include <Arduino.h>

struct logger {
    static void log(const char* message) {
        Serial.println(message);
    }
};