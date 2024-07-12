#include "transceivers.h"

#define FT817_V 11

extern int analog_read(byte pin);

byte FT817det() {
    const int ftv = analog_read(FT817_V);

    if (ftv < 95) return 10;
    if (ftv > 99 && ftv < 160) return 9;
    if (ftv > 164 && ftv < 225) return 7;
    if (ftv > 229 && ftv < 285) return 6;
    if (ftv > 289 && ftv < 345) return 5;
    if (ftv > 349 && ftv < 410) return 4;
    if (ftv > 414 && ftv < 475) return 3;
    if (ftv > 479 && ftv < 508) return 2;
    if (ftv < 605) return 1;

    return 0;
}

byte Eladdet() {
    const int ftv = analog_read(FT817_V);

    if (ftv < 118) return 10;
    if (ftv > 130 && ftv < 200) return 9;
    if (ftv > 212 && ftv < 282) return 7;
    if (ftv > 295 && ftv < 365) return 6;
    if (ftv > 380 && ftv < 450) return 5;
    if (ftv > 462 && ftv < 532) return 4;
    if (ftv > 545 && ftv < 615) return 3;
    if (ftv > 630 && ftv < 700) return 2;
    if (ftv < 780) return 1;

    return 0;
}

byte Xiegudet() {
    const int ftv = analog_read(FT817_V);

    if (ftv < 136) return 10;
    if (ftv < 191) return 9;
    if (ftv < 246) return 8;
    if (ftv < 300) return 7;
    if (ftv < 355) return 6;
    if (ftv < 410) return 5;
    if (ftv < 465) return 4;
    if (ftv < 520) return 3;
    if (ftv < 574) return 2;

    return 1;
}
