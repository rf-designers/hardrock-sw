#ifndef LCD_h
#define LCD_h

#include <Arduino.h>
#include <avr/pgmspace.h>
#include <SPI.h>

//Basic Colors
#define WHITE          0xFFFFUL
#define LGRAY  		   0xD69AUL
#define GRAY  		   0x738EUL
#define MGRAY  		   0x52AAUL
#define DGRAY  		   0x4228UL
#define BLACK          0x0000UL

#define BLUE           0x001FUL
#define LBLUE          0x843FUL
#define DBLUE		   0X000cUL
#define GBLUE		   0X07FFUL

#define BRED           0XF81FUL
#define GRED 		   0XFFE0UL
#define RED            0xF800UL
#define BRRED 		   0XFC07UL
#define DRED		   0xc800UL

#define MAGENTA        0xF81FUL
#define GREEN          0x07E0UL
#define CYAN           0x7FFFUL
#define YELLOW         0xFFE0UL
#define ORANGE         0xFF00UL
#define BROWN 		   0XBC40UL
#define PURPLE		   0xb17fUL

#define LCD_CMD      0
#define LCD_DATA     1

#define FONT_1206    12
#define FONT_1608    16

#define LCD_WIDTH    320
#define LCD_HEIGHT   240
//TFT resolution 240*320

#define MIN_X	0
#define MIN_Y	0
#define MAX_X	319
#define MAX_Y	239
//
//#include "HR500_pins.h"
//#define __LCD_WRITE_BYTE(__DATA)       SPI.transfer(__DATA)
//#include <avr/pgmspace.h>
//
//class TFT {
//public:
//    uint8_t LCD_SEL = 0;
//
//    void lcd_write_byte(uint8_t chByte, uint8_t chCmd) {
//        if (chCmd) {
//            __LCD_DC_SET();
//        } else {
//            __LCD_DC_CLR();
//        }
//        noInterrupts();
//        if (LCD_SEL == 0)
//            __LCD_CS1_CLR();
//        else
//            __LCD_CS2_CLR();
//
//        SPI.beginTransaction(SPISettings(120000000, MSBFIRST, SPI_MODE3));
//        __LCD_WRITE_BYTE(chByte);
//        SPI.endTransaction();
//        __LCD_CS1_SET();
//        __LCD_CS2_SET();
//
//        interrupts();
//    }
//
//    inline void lcd_write_word(uint16_t hwData) {
//        __LCD_DC_SET();
//        noInterrupts();
//        if (LCD_SEL == 0)
//            __LCD_CS1_CLR();
//        else
//            __LCD_CS2_CLR();
//        SPI.beginTransaction(SPISettings(120000000, MSBFIRST, SPI_MODE3));
//        __LCD_WRITE_BYTE(hwData >> 8);
//        __LCD_WRITE_BYTE(hwData & 0xFF);
//        SPI.endTransaction();
//        __LCD_CS1_SET();
//        __LCD_CS2_SET();
//        interrupts();
//    }
//
//
//    //write a word(two bytes) to the specified register of lcd.
//    //chRegister address of the register of lcd.
//    //hwValue value is written to the specified register.
//    void lcd_write_register(uint8_t chRegister, uint8_t chValue) {
//        lcd_write_byte(chRegister, LCD_CMD);
//        lcd_write_byte(chValue, LCD_DATA);
//    }
//
//    //set the specified position of cursor on lcd.
//    //hwXpos specify x position
//    //hwYpos specify y position
//    void lcd_set_cursor(uint16_t hwXpos, uint16_t hwYpos) {
//        if (hwXpos > 242 || hwYpos > 322) {
//            return;
//        }
//
//        lcd_write_register(0x02, hwXpos >> 8);
//        lcd_write_register(0x03, hwXpos & 0xFF); //Column Start
//        lcd_write_register(0x06, hwYpos >> 8);
//        lcd_write_register(0x07, hwYpos & 0xFF); //Row Start
//    }
//
//    //clear the lcd with the specified color.
//    void lcd_clear_screen(uint16_t hwColor) {
//        uint32_t i, wCount = LCD_WIDTH;
//
//        wCount *= LCD_HEIGHT;
//
//        lcd_set_cursor(0, 0);
//        lcd_write_byte(0x22, LCD_CMD);
//
//        __LCD_DC_SET();
//        if (LCD_SEL == 0)
//            __LCD_CS1_CLR();
//        else
//            __LCD_CS2_CLR();
//
//        for (i = 0; i < wCount; i++) {
//            __LCD_WRITE_BYTE(hwColor >> 8);
//            __LCD_WRITE_BYTE(hwColor & 0xFF);
//        }
//        __LCD_CS1_SET();
//        __LCD_CS2_SET();
//    }
//
//    void drawBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color);
//
//    void drawXBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color);
//
//    void drawRGBBitmap(int16_t x, int16_t y, uint16_t* bitmap, int16_t w, int16_t h);
//
//    void lcd_init(unsigned int S_COLOR);
//    void lcd_reset(void);
//    void lcd_draw_point(uint16_t hwXpos, uint16_t hwYpos, uint16_t hwColor);
//    void lcd_display_char(uint16_t hwXpos, //specify x position.
//                          uint16_t hwYpos, //specify y position.
//                          uint8_t chChr, //a char is display.
//                          uint8_t chSize, //specify the size of the char
//                          uint16_t hwColor); //specify the color of the char
//    //display a number at the specified position on lcd.
//    void lcd_display_num(uint16_t hwXpos, //specify x position.
//                         uint16_t hwYpos, //specify y position.
//                         uint32_t chNum, //a number is display.
//                         uint8_t chLen, //length ot the number
//                         uint8_t chSize, //specify the size of the number
//                         uint16_t hwColor); //specify the color of the number
//    //display a string at the specified position on lcd.
//    void lcd_display_string(uint16_t hwXpos, //specify x position.
//                            uint16_t hwYpos, //specify y position.
//                            const uint8_t* pchString, //a pointer to string
//                            uint8_t chSize, // the size of the string
//                            uint16_t hwColor); // specify the color of the string
//    void lcd_draw_line(uint16_t hwXpos0, //specify x0 position.
//                       uint16_t hwYpos0, //specify y0 position.
//                       uint16_t hwXpos1, //specify x1 position.
//                       uint16_t hwYpos1, //specify y1 position.
//                       uint16_t hwColor); //specify the color of the line
//    void lcd_line_wide(uint16_t hwXpos0, //specify x0 position.
//                       uint16_t hwYpos0, //specify y0 position.
//                       uint16_t hwXpos1, //specify x1 position.
//                       uint16_t hwYpos1, //specify y1 position.
//                       uint16_t hwColor); //specify the color of the line
//    void lcd_draw_circle(uint16_t hwXpos, //specify x position.
//                         uint16_t hwYpos, //specify y position.
//                         uint16_t hwRadius, //specify the radius of the circle.
//                         uint16_t hwColor); //specify the color of the circle.
//    void lcd_fill_rect(uint16_t hwXpos, //specify x position.
//                       uint16_t hwYpos, //specify y position.
//                       uint16_t hwWidth, //specify the width of the rectangle.
//                       uint16_t hwHeight, //specify the height of the rectangle.
//                       uint16_t hwColor); //specify the color of rectangle.
//    void lcd_draw_v_line(uint16_t hwXpos, //specify x position.
//                         uint16_t hwYpos, //specify y position.
//                         uint16_t hwHeight, //specify the height of the vertical line.
//                         uint16_t hwColor); //specify the color of the vertical line.
//    void lcd_draw_h_line(uint16_t hwXpos, //specify x position.
//                         uint16_t hwYpos, //specify y position.
//                         uint16_t hwWidth, //specify the width of the horizonal line.
//                         uint16_t hwColor); //specify the color of the horizonal line.
//    void lcd_draw_rect(uint16_t hwXpos, //specify x position.
//                       uint16_t hwYpos, //specify y position.
//                       uint16_t hwWidth, //specify the width of the rectangle.
//                       uint16_t hwHeight, //specify the height of the rectangle.
//                       uint16_t hwColor); //specify the color of rectangle.
//    void drawChar(uint8_t ascii, uint16_t poX, uint16_t poY, uint16_t size, uint16_t fgcolor);
//    void drawString(const char* string, uint16_t poX, uint16_t poY, uint16_t size, uint16_t fgcolor);
//    void drawHChar(uint8_t ascii, uint16_t poX, uint16_t poY, uint16_t size, uint16_t fgcolor);
//    void drawHString(uint8_t* string, uint16_t poX, uint16_t poY, uint16_t size, uint16_t fgcolor);
//};
//
////extern TFT Tft;
//
#endif
//
///*********************************************************************************************************
//  END FILE
//*********************************************************************************************************/
