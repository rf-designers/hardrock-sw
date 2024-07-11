#ifndef HARDROCK_SW_DISPLAY_BOARD_H
#define HARDROCK_SW_DISPLAY_BOARD_H

#include <Arduino.h>
#include <SPI.h>
#include "HR500_pins.h"
#include "HR500.h"

enum class write_type {
    command,
    data
};

struct lcd_comm {
    const unsigned int width = 320;
    const unsigned int height = 240;

    virtual void init_cs() = 0;
    virtual void set_cs() = 0;
    virtual void clear_cs() = 0;

    inline void lcd_write_byte(uint8_t chByte, write_type chCmd) {
        if (chCmd == write_type::data) {
            __LCD_DC_SET();
        } else {
            __LCD_DC_CLR();
        }
//        noInterrupts();
        {
            clear_cs();
            SPI.beginTransaction(SPISettings(120000000, MSBFIRST, SPI_MODE3));
            SPI.transfer(chByte);
            SPI.endTransaction();
            set_cs();
        }
//        interrupts();
    }

    inline void lcd_write_word(uint16_t hwData) {
        __LCD_DC_SET();
//        noInterrupts();
        {
            clear_cs();
            SPI.beginTransaction(SPISettings(120000000, MSBFIRST, SPI_MODE3));
            // TODO: replace with SPI.transfer(*, size)?
            SPI.transfer(hwData >> 8);
            SPI.transfer(hwData & 0xFF);
            SPI.endTransaction();
            set_cs();
        }
//        interrupts();
    }

    //write a word(two bytes) to the specified register of lcd.
    //chRegister address of the register of lcd.
    //hwValue value is written to the specified register.
    void lcd_write_register(uint8_t chRegister, uint8_t chValue) {
        lcd_write_byte(chRegister, write_type::command);
        lcd_write_byte(chValue, write_type::data);
    }

    //set the specified position of cursor on lcd.
    //hwXpos specify x position
    //hwYpos specify y position
    void lcd_set_cursor(uint16_t hwXpos, uint16_t hwYpos) {
        if (hwXpos > 242 || hwYpos > 322) {
            return;
        }

        lcd_write_register(0x02, hwXpos >> 8);
        lcd_write_register(0x03, hwXpos & 0xFF); //Column Start
        lcd_write_register(0x06, hwYpos >> 8);
        lcd_write_register(0x07, hwYpos & 0xFF); //Row Start
    }

    //clear the lcd with the specified color.
    void lcd_clear_screen(uint16_t hwColor) {
        uint32_t i, wCount = width;

        wCount *= height;

        noInterrupts();
        {
            lcd_set_cursor(0, 0);
            lcd_write_byte(0x22, write_type::command);

            __LCD_DC_SET();
            clear_cs();
            for (i = 0; i < wCount; i++) {
                lcd_write_word(hwColor);
            }
            set_cs();
        }

        interrupts();
    }

    void init(unsigned int S_COLOR) {
        noInterrupts();

        __XPT2046_CS_DISABLE(); //.kbv
        __LCD_DC_OUT();
        __LCD_DC_SET();

        init_cs();
        set_cs();

        __LCD_BKL_OUT();
        __LCD_BKL_OFF();

        //Driving ability Setting
        //lcd_write_register(0x00,0xFF); //read ID
        //lcd_write_register(0x61,0xFF); //read ID
        //lcd_write_register(0x62,0xFF); //read ID
        //lcd_write_register(0x63,0xFF); //read ID

        lcd_write_register(0xEA, 0x00); //PTBA[15:8]
        lcd_write_register(0xEB, 0x20); //PTBA[7:0]
        lcd_write_register(0xEC, 0x0C); //STBA[15:8]
        lcd_write_register(0xED, 0xC4); //STBA[7:0]
        lcd_write_register(0xE8, 0x38); //OPON[7:0]
        lcd_write_register(0xE9, 0x10); //OPON1[7:0]
        lcd_write_register(0xF1, 0x01); //OTPS1B
        lcd_write_register(0xF2, 0x10); //GEN
        //Gamma 2.2 Setting
        lcd_write_register(0x40, 0x01); //
        lcd_write_register(0x41, 0x00); //
        lcd_write_register(0x42, 0x00); //
        lcd_write_register(0x43, 0x10); //
        lcd_write_register(0x44, 0x0E); //
        lcd_write_register(0x45, 0x24); //
        lcd_write_register(0x46, 0x04); //
        lcd_write_register(0x47, 0x50); //
        lcd_write_register(0x48, 0x02); //
        lcd_write_register(0x49, 0x13); //
        lcd_write_register(0x4A, 0x19); //
        lcd_write_register(0x4B, 0x19); //
        lcd_write_register(0x4C, 0x16); //
        lcd_write_register(0x50, 0x1B); //
        lcd_write_register(0x51, 0x31); //
        lcd_write_register(0x52, 0x2F); //
        lcd_write_register(0x53, 0x3F); //
        lcd_write_register(0x54, 0x3F); //
        lcd_write_register(0x55, 0x3E); //
        lcd_write_register(0x56, 0x2F); //
        lcd_write_register(0x57, 0x7B); //
        lcd_write_register(0x58, 0x09); //
        lcd_write_register(0x59, 0x06); //
        lcd_write_register(0x5A, 0x06); //
        lcd_write_register(0x5B, 0x0C); //
        lcd_write_register(0x5C, 0x1D); //
        lcd_write_register(0x5D, 0xCC); //
        //Power Voltage Setting
        lcd_write_register(0x1B, 0x1B); //VRH=4.65V
        lcd_write_register(0x1A, 0x01); //BT (VGH~15V,VGL~-10V,DDVDH~5V)
        lcd_write_register(0x24, 0x2F); //VMH(VCOM High voltage ~3.2V)
        lcd_write_register(0x25, 0x57); //VML(VCOM Low voltage -1.2V)
        //****VCOM offset**///
        lcd_write_register(0x23, 0x88); //for Flicker adjust //can reload from OTP
        //Power on Setting
        lcd_write_register(0x18, 0x34); //I/P_RADJ,N/P_RADJ, Normal mode 60Hz
        lcd_write_register(0x19, 0x01); //OSC_EN='1', start Osc
        lcd_write_register(0x01, 0x00); //DP_STB='0', out deep sleep
        lcd_write_register(0x1F, 0x88); // GAS=1, VOMG=00, PON=0, DK=1, XDK=0, DVDH_TRI=0, STB=0
        delay(5);
        lcd_write_register(0x1F, 0x80); // GAS=1, VOMG=00, PON=0, DK=0, XDK=0, DVDH_TRI=0, STB=0
        delay(5);
        lcd_write_register(0x1F, 0x90); // GAS=1, VOMG=00, PON=1, DK=0, XDK=0, DVDH_TRI=0, STB=0
        delay(5);
        lcd_write_register(0x1F, 0xD0); // GAS=1, VOMG=10, PON=1, DK=0, XDK=0, DDVDH_TRI=0, STB=0
        delay(5);
        //262k/65k color selection
        lcd_write_register(0x17, 0x05); //default 0x06 262k color // 0x05 65k color
        //SET PANEL
        lcd_write_register(0x36, 0x00); //SS_P, GS_P,REV_P,BGR_P
        //Display ON Setting
        lcd_write_register(0x28, 0x38); //GON=1, DTE=1, D=1000
        delay(40);
        lcd_write_register(0x28, 0x3F); //GON=1, DTE=1, D=1100

        lcd_write_register(0x16, 0x18);
        //Set GRAM Area
        lcd_write_register(0x02, 0x00);
        lcd_write_register(0x03, 0x00); //Column Start
        lcd_write_register(0x04, 0x00);
        lcd_write_register(0x05, 0xEF); //Column End
        lcd_write_register(0x06, 0x00);
        lcd_write_register(0x07, 0x00); //Row Start
        lcd_write_register(0x08, 0x01);
        lcd_write_register(0x09, 0x3F); //Row End
        interrupts();
        delay(1);
        //lcd_write_register(0x00,0xFF); //read ID
        //lcd_write_register(0x61,0xFF); //read ID
        //lcd_write_register(0x62,0xFF); //read ID
        //lcd_write_register(0x63,0xFF); //read ID
        lcd_clear_screen(S_COLOR);
        __LCD_BKL_ON();
    }
};


struct lcd1 : public lcd_comm {
    void init_cs() override {
        __LCD_CS1_OUT();
    }

    void clear_cs() override {
        __LCD_CS1_CLR();
    }

    void set_cs() override {
        __LCD_CS1_SET();
    }
};

struct lcd2 : public lcd_comm {
    void init_cs() override {
        __LCD_CS2_OUT();
    }

    void clear_cs() override {
        __LCD_CS2_CLR();
    }

    void set_cs() override {
        __LCD_CS2_SET();
    }
};


class display_board {
public:

    explicit display_board(lcd_comm *comm) : lcd{comm} {
    }

    void lcd_init(unsigned int S_COLOR) {
        lcd->init(S_COLOR);
    }

    void lcd_draw_point(uint16_t hwXpos, uint16_t hwYpos, uint16_t hwColor);
    void lcd_display_char(uint16_t hwXpos, //specify x position.
                          uint16_t hwYpos, //specify y position.
                          uint8_t chChr, //a char is display.
                          uint8_t chSize, //specify the size of the char
                          uint16_t hwColor); //specify the color of the char
    void draw_char(uint8_t ascii, uint16_t poX, uint16_t poY, uint16_t size, uint16_t fgcolor);
    void lcd_fill_rect(uint16_t hwXpos, //specify x position.
                       uint16_t hwYpos, //specify y position.
                       uint16_t hwWidth, //specify the width of the rectangle.
                       uint16_t hwHeight, //specify the height of the rectangle.
                       uint16_t hwColor); //specify the color of rectangle.
    void lcd_draw_v_line(uint16_t hwXpos, //specify x position.
                         uint16_t hwYpos, //specify y position.
                         uint16_t hwHeight, //specify the height of the vertical line.
                         uint16_t hwColor); //specify the color of the vertical line.
    void lcd_draw_h_line(uint16_t hwXpos, //specify x position.
                         uint16_t hwYpos, //specify y position.
                         uint16_t hwWidth, //specify the width of the horizonal line.
                         uint16_t hwColor); //specify the color of the horizonal line.

    void draw_string(const char *string, uint16_t poX, uint16_t poY, uint16_t size, uint16_t fgcolor);


private:
    lcd_comm *lcd;
};


#endif //HARDROCK_SW_DISPLAY_BOARD_H
