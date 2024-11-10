#ifndef HARDROCK_SW_DISPLAY_BOARD_H
#define HARDROCK_SW_DISPLAY_BOARD_H

#include <Arduino.h>
#include <SPI.h>
#include "HR500_pins.h"

enum class write_type {
    command,
    data
};

struct lcd_comm {
    virtual ~lcd_comm() = default;
    static constexpr unsigned int width = 320;
    static constexpr unsigned int height = 240;

    virtual void init_cs() = 0;
    virtual void set_cs() = 0;
    virtual void clear_cs() = 0;

    void write_byte(uint8_t data, write_type cmd) {
        if (cmd == write_type::data) {
            __LCD_DC_SET();
        } else {
            __LCD_DC_CLR();
        }
        // noInterrupts();
        {
            clear_cs();
            SPI.beginTransaction(SPISettings(120000000, MSBFIRST, SPI_MODE3));
            SPI.transfer(data);
            SPI.endTransaction();
            set_cs();
        }
        // interrupts();
    }

    void write_word(uint16_t data) {
        __LCD_DC_SET();
        // noInterrupts();
        {
            clear_cs();
            SPI.beginTransaction(SPISettings(120000000, MSBFIRST, SPI_MODE3));
            //            SPI.transfer16(hwData);
            // TODO: replace with SPI.transfer(*, size)?
            SPI.transfer(data >> 8);
            SPI.transfer(data & 0xFF);
            SPI.endTransaction();
            set_cs();
        }
        // interrupts();
    }

    void write_register(const uint8_t reg_address, const uint8_t value) {
        write_byte(reg_address, write_type::command);
        write_byte(value, write_type::data);
    }

    void set_cursor(const uint16_t x, const uint16_t y) {
        if (x > 242 || y > 322) {
            return;
        }

        write_register(0x02, x >> 8);
        write_register(0x03, x & 0xFF); // column Start
        write_register(0x06, y >> 8);
        write_register(0x07, y & 0xFF); // row Start
    }

    // clear the lcd with the specified color.
    void clear_screen(const uint16_t color) {
        fill_rect(0, 0, width, height, color);
    }

    void fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
        noInterrupts();
        // 0, 0 is bottom left
        //        set_addr_window(240 - y, x, h, w);
        set_addr_window(240 - y - h, x, h, w);
        write_byte(0x22, write_type::command);

        __LCD_DC_SET();
        clear_cs();

        auto nr_pixels = static_cast<unsigned long>(w) * static_cast<unsigned long>(h);
        for (unsigned long i = 0; i < nr_pixels; i++) {
            SPI.transfer(color >> 8);
            SPI.transfer(color & 0xFF);
        }
        set_cs();
        interrupts();
    }

    void init(unsigned int S_COLOR) {
        noInterrupts();

        __XPT2046_CS_DISABLE(); //.kbv
        __LCD_DC_OUT();
        __LCD_DC_SET();

        __LCD_DC_CLR();
        init_cs();
        set_cs();

        __LCD_BKL_OUT();
        __LCD_BKL_OFF();

        //Driving ability Setting
        //lcd_write_register(0x00,0xFF); //read ID
        //lcd_write_register(0x61,0xFF); //read ID
        //lcd_write_register(0x62,0xFF); //read ID
        //lcd_write_register(0x63,0xFF); //read ID

        write_register(0xEA, 0x00); //PTBA[15:8]
        write_register(0xEB, 0x20); //PTBA[7:0]
        write_register(0xEC, 0x0C); //STBA[15:8]
        write_register(0xED, 0xC4); //STBA[7:0]
        write_register(0xE8, 0x38); //OPON[7:0]
        write_register(0xE9, 0x10); //OPON1[7:0]
        write_register(0xF1, 0x01); //OTPS1B
        write_register(0xF2, 0x10); //GEN
        //Gamma 2.2 Setting
        write_register(0x40, 0x01); //
        write_register(0x41, 0x00); //
        write_register(0x42, 0x00); //
        write_register(0x43, 0x10); //
        write_register(0x44, 0x0E); //
        write_register(0x45, 0x24); //
        write_register(0x46, 0x04); //
        write_register(0x47, 0x50); //
        write_register(0x48, 0x02); //
        write_register(0x49, 0x13); //
        write_register(0x4A, 0x19); //
        write_register(0x4B, 0x19); //
        write_register(0x4C, 0x16); //
        write_register(0x50, 0x1B); //
        write_register(0x51, 0x31); //
        write_register(0x52, 0x2F); //
        write_register(0x53, 0x3F); //
        write_register(0x54, 0x3F); //
        write_register(0x55, 0x3E); //
        write_register(0x56, 0x2F); //
        write_register(0x57, 0x7B); //
        write_register(0x58, 0x09); //
        write_register(0x59, 0x06); //
        write_register(0x5A, 0x06); //
        write_register(0x5B, 0x0C); //
        write_register(0x5C, 0x1D); //
        write_register(0x5D, 0xCC); //
        //Power Voltage Setting
        write_register(0x1B, 0x1B); //VRH=4.65V
        write_register(0x1A, 0x01); //BT (VGH~15V,VGL~-10V,DDVDH~5V)
        write_register(0x24, 0x2F); //VMH(VCOM High voltage ~3.2V)
        write_register(0x25, 0x57); //VML(VCOM Low voltage -1.2V)
        //****VCOM offset**///
        write_register(0x23, 0x88); //for Flicker adjust //can reload from OTP
        //Power on Setting
        write_register(0x18, 0x34); //I/P_RADJ,N/P_RADJ, Normal mode 60Hz
        write_register(0x19, 0x01); //OSC_EN='1', start Osc
        write_register(0x01, 0x00); //DP_STB='0', out deep sleep
        write_register(0x1F, 0x88); // GAS=1, VOMG=00, PON=0, DK=1, XDK=0, DVDH_TRI=0, STB=0
        delay(5);
        write_register(0x1F, 0x80); // GAS=1, VOMG=00, PON=0, DK=0, XDK=0, DVDH_TRI=0, STB=0
        delay(5);
        write_register(0x1F, 0x90); // GAS=1, VOMG=00, PON=1, DK=0, XDK=0, DVDH_TRI=0, STB=0
        delay(5);
        write_register(0x1F, 0xD0); // GAS=1, VOMG=10, PON=1, DK=0, XDK=0, DDVDH_TRI=0, STB=0
        delay(5);
        //262k/65k color selection
        write_register(0x17, 0x05); //default 0x06 262k color // 0x05 65k color
        //SET PANEL
        write_register(0x36, 0x00); //SS_P, GS_P,REV_P,BGR_P
        //Display ON Setting
        write_register(0x28, 0x38); //GON=1, DTE=1, D=1000
        delay(40);
        write_register(0x28, 0x3F); //GON=1, DTE=1, D=1100

        write_register(0x16, 0x18);
        //Set GRAM Area
        write_register(0x02, 0x00);
        write_register(0x03, 0x00); //Column Start
        write_register(0x04, 0x00);
        write_register(0x05, 0xEF); //Column End
        write_register(0x06, 0x00);
        write_register(0x07, 0x00); //Row Start
        write_register(0x08, 0x01);
        write_register(0x09, 0x3F); //Row End
        interrupts();
        delay(1);
        //lcd_write_register(0x00,0xFF); //read ID
        //lcd_write_register(0x61,0xFF); //read ID
        //lcd_write_register(0x62,0xFF); //read ID
        //lcd_write_register(0x63,0xFF); //read ID
        clear_screen(S_COLOR);
        __LCD_BKL_ON();

        //        // ------------------------------------------------------------------
        //
        //        //Driving ability Setting
        //        write_register(0xEA, 0x00); //PTBA[15:8]
        //        write_register(0xEB, 0x20); //PTBA[7:0]
        //        write_register(0xEC, 0x0C); //STBA[15:8]
        //        write_register(0xED, 0xC4); //STBA[7:0]
        //        write_register(0xE8, 0x38); //OPON[7:0]
        //        write_register(0xE9, 0x10); //OPON1[7:0]
        //        write_register(0xF1, 0x01); //OTPS1B
        //        write_register(0xF2, 0x10); //GEN
        //        //Gamma 2.2 Setting
        //        write_register(0x40, 0x01); //
        //        write_register(0x41, 0x00); //
        //        write_register(0x42, 0x00); //
        //        write_register(0x43, 0x10); //
        //        write_register(0x44, 0x0E); //
        //        write_register(0x45, 0x24); //
        //        write_register(0x46, 0x04); //
        //        write_register(0x47, 0x50); //
        //        write_register(0x48, 0x02); //
        //        write_register(0x49, 0x13); //
        //        write_register(0x4A, 0x19); //
        //        write_register(0x4B, 0x19); //
        //        write_register(0x4C, 0x16); //
        //        write_register(0x50, 0x1B); //
        //        write_register(0x51, 0x31); //
        //        write_register(0x52, 0x2F); //
        //        write_register(0x53, 0x3F); //
        //        write_register(0x54, 0x3F); //
        //        write_register(0x55, 0x3E); //
        //        write_register(0x56, 0x2F); //
        //        write_register(0x57, 0x7B); //
        //        write_register(0x58, 0x09); //
        //        write_register(0x59, 0x06); //
        //        write_register(0x5A, 0x06); //
        //        write_register(0x5B, 0x0C); //
        //        write_register(0x5C, 0x1D); //
        //        write_register(0x5D, 0xCC); //
        //        //Power Voltage Setting
        //        write_register(0x1B, 0x1B); //VRH=4.65V
        //        write_register(0x1A, 0x01); //BT (VGH~15V,VGL~-10V,DDVDH~5V)
        //        write_register(0x24, 0x2F); //VMH(VCOM High voltage ~3.2V)
        //        write_register(0x25, 0x57); //VML(VCOM Low voltage -1.2V)
        //        //****VCOM offset**///
        //        write_register(0x23, 0x88); //for Flicker adjust //can reload from OTP
        //        //Power on Setting
        //        write_register(0x18, 0x34); //I/P_RADJ,N/P_RADJ, Normal mode 60Hz
        //        write_register(0x19, 0x01); //OSC_EN='1', start Osc
        //        write_register(0x01, 0x00); //DP_STB='0', out deep sleep
        //        write_register(0x1F, 0x88);// GAS=1, VOMG=00, PON=0, DK=1, XDK=0, DVDH_TRI=0, STB=0
        //        delay(5);
        //        write_register(0x1F, 0x80);// GAS=1, VOMG=00, PON=0, DK=0, XDK=0, DVDH_TRI=0, STB=0
        //        delay(5);
        //        write_register(0x1F, 0x90);// GAS=1, VOMG=00, PON=1, DK=0, XDK=0, DVDH_TRI=0, STB=0
        //        delay(5);
        //        write_register(0x1F, 0xD0);// GAS=1, VOMG=10, PON=1, DK=0, XDK=0, DDVDH_TRI=0, STB=0
        //        delay(5);
        //        //262k/65k color selection
        //        write_register(0x17, 0x05); //default 0x06 262k color // 0x05 65k color
        //        //SET PANEL
        //        write_register(0x36, 0x00); //SS_P, GS_P,REV_P,BGR_P
        //        //Display ON Setting
        //        write_register(0x28, 0x38); //GON=1, DTE=1, D=1000
        //        delay(40);
        //        write_register(0x28, 0x3F); //GON=1, DTE=1, D=1100
        //
        //        write_register(0x16, 0x18);
        //        //Set GRAM Area
        //        write_register(0x02, 0x00);
        //        write_register(0x03, 0x00); //Column Start
        //        write_register(0x04, 0x00);
        //        write_register(0x05, 0xEF); //Column End
        //        write_register(0x06, 0x00);
        //        write_register(0x07, 0x00); //Row Start
        //        write_register(0x08, 0x01);
        //        write_register(0x09, 0x3F); //Row End
        //
        //        clear_screen(S_COLOR);
        //        __LCD_BKL_ON();
    }


    void set_addr_window(const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h) {
        const auto x_end = x + w - 1;
        const auto y_end = y + h - 1;

        write_register(0x02, x >> 8); // column address start 2
        write_register(0x03, x & 0xFF); // column address start 1
        write_register(0x04, x_end >> 8); // column address end 2
        write_register(0x05, x_end & 0xFF); // column address end 1

        write_register(0x06, y >> 8); // row address start 2
        write_register(0x07, y & 0xFF); // row address start 1
        write_register(0x08, y_end >> 8); // row address end 2
        write_register(0x09, y_end & 0xFF); // row address end 1

        write_byte(0x22, write_type::command); // RAMWR
    }

    void reset() {
        __XPT2046_CS_DISABLE(); //.kbv
        __LCD_BKL_OFF();

        write_register(0xEA, 0x00); //PTBA[15:8]
        write_register(0xEB, 0x20); //PTBA[7:0]
        write_register(0xEC, 0x0C); //STBA[15:8]
        write_register(0xED, 0xC4); //STBA[7:0]
        write_register(0xE8, 0x38); //OPON[7:0]
        write_register(0xE9, 0x10); //OPON1[7:0]
        write_register(0xF1, 0x01); //OTPS1B
        write_register(0xF2, 0x10); //GEN
        //Gamma 2.2 Setting
        write_register(0x40, 0x01); //
        write_register(0x41, 0x00); //
        write_register(0x42, 0x00); //
        write_register(0x43, 0x10); //
        write_register(0x44, 0x0E); //
        write_register(0x45, 0x24); //
        write_register(0x46, 0x04); //
        write_register(0x47, 0x50); //
        write_register(0x48, 0x02); //
        write_register(0x49, 0x13); //
        write_register(0x4A, 0x19); //
        write_register(0x4B, 0x19); //
        write_register(0x4C, 0x16); //
        write_register(0x50, 0x1B); //
        write_register(0x51, 0x31); //
        write_register(0x52, 0x2F); //
        write_register(0x53, 0x3F); //
        write_register(0x54, 0x3F); //
        write_register(0x55, 0x3E); //
        write_register(0x56, 0x2F); //
        write_register(0x57, 0x7B); //
        write_register(0x58, 0x09); //
        write_register(0x59, 0x06); //
        write_register(0x5A, 0x06); //
        write_register(0x5B, 0x0C); //
        write_register(0x5C, 0x1D); //
        write_register(0x5D, 0xCC); //
        //Power Voltage Setting
        write_register(0x1B, 0x1B); //VRH=4.65V
        write_register(0x1A, 0x01); //BT (VGH~15V,VGL~-10V,DDVDH~5V)
        write_register(0x24, 0x2F); //VMH(VCOM High voltage ~3.2V)
        write_register(0x25, 0x57); //VML(VCOM Low voltage -1.2V)
        //****VCOM offset**///
        write_register(0x23, 0x88); //for Flicker adjust //can reload from OTP
        //Power on Setting
        write_register(0x18, 0x34); //I/P_RADJ,N/P_RADJ, Normal mode 60Hz
        write_register(0x19, 0x01); //OSC_EN='1', start Osc
        write_register(0x01, 0x00); //DP_STB='0', out deep sleep
        write_register(0x1F, 0x88); // GAS=1, VOMG=00, PON=0, DK=1, XDK=0, DVDH_TRI=0, STB=0
        delay(5);
        write_register(0x1F, 0x80); // GAS=1, VOMG=00, PON=0, DK=0, XDK=0, DVDH_TRI=0, STB=0
        delay(5);
        write_register(0x1F, 0x90); // GAS=1, VOMG=00, PON=1, DK=0, XDK=0, DVDH_TRI=0, STB=0
        delay(5);
        write_register(0x1F, 0xD0); // GAS=1, VOMG=10, PON=1, DK=0, XDK=0, DDVDH_TRI=0, STB=0
        delay(5);
        //262k/65k color selection
        write_register(0x17, 0x05); //default 0x06 262k color // 0x05 65k color
        //SET PANEL
        write_register(0x36, 0x00); //SS_P, GS_P,REV_P,BGR_P
        //Display ON Setting
        write_register(0x28, 0x38); //GON=1, DTE=1, D=1000
        delay(40);
        write_register(0x28, 0x3F); //GON=1, DTE=1, D=1100

        write_register(0x16, 0x18);
        __LCD_BKL_OFF();
        //Set GRAM Area
        write_register(0x02, 0x00);
        write_register(0x03, 0x00); //Column Start
        write_register(0x04, 0x00);
        write_register(0x05, 0xEF); //Column End
        write_register(0x06, 0x00);
        write_register(0x07, 0x00); //Row Start
        write_register(0x08, 0x01);
        write_register(0x09, 0x3F); //Row End
        delay(25);
        __LCD_BKL_ON();
    }
};


struct lcd1 final : lcd_comm {
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

struct lcd2 final : lcd_comm {
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
    explicit display_board(lcd_comm* comm)
        : lcd{comm} {
    }

    void lcd_init(unsigned int S_COLOR) const {
        lcd->init(S_COLOR);
    }

    void draw_point(uint16_t x, uint16_t y, uint16_t color) const;
    void display_char(uint16_t x, uint16_t y,
                      uint8_t chr, uint8_t fnt,
                      uint16_t color) const;
    void display_string(uint16_t x, uint16_t y,
                        const uint8_t* text,
                        uint8_t fnt, uint16_t color) const;
    void draw_char(uint8_t ascii, uint16_t x, uint16_t y, uint16_t size, uint16_t color) const;

    void fill_rect(uint16_t x, uint16_t y,
                   uint16_t w, uint16_t h,
                   uint16_t color) const;

    void draw_v_line(uint16_t x, uint16_t y, uint16_t length, uint16_t color) const;

    void draw_h_line(uint16_t x,
                     uint16_t y,
                     uint16_t width,
                     uint16_t color) const;
    void draw_string(const char* text, uint16_t x, uint16_t y, uint16_t size, uint16_t color) const;


    void clear_screen(uint16_t color) const;
    void lcd_reset() const;

private:
    lcd_comm* lcd;
};


#endif //HARDROCK_SW_DISPLAY_BOARD_H
