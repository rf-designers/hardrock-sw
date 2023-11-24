#ifndef HR500_PINS_H_ 
#define HR500_PINS_H_

// macros for the Waveshare Shield pins

#if defined(__AVR_ATmega2560__)
#define __LCD_CS1_DDR      DDRC
#define __LCD_CS1_PORT     PORTC
#define __LCD_CS1_PIN      2
#define __LCD_CS2_DDR      DDRB
#define __LCD_CS2_PORT     PORTB
#define __LCD_CS2_PIN      0
#define __LCD_DC_DDR      DDRC
#define __LCD_DC_PORT     PORTC
#define __LCD_DC_PIN      0
#define __LCD_BKL_DDR     DDRE
#define __LCD_BKL_PORT    PORTE
#define __LCD_BKL_PIN     3
#define __SD_CS_DDR      DDRL
#define __SD_CS_PORT     PORTL
#define __SD_CS_PIN      5
#define __XPT_CS_DDR     DDRL
#define __XPT_CS_PORT    PORTL
#define __XPT_CS_PIN     7
#define __XPT_IRQ_DDR     DDRD
#define __XPT_IRQ_PORT    PORTD
#define __XPT_IRQ_PINR    PIND
#define __XPT_IRQ_PIN     3
#else
#error
#endif

#define __LCD_CS1_OUT()    __LCD_CS1_DDR |= (1<<__LCD_CS1_PIN)
#define __LCD_CS1_CLR()    __LCD_CS1_PORT &=~ (1<<__LCD_CS1_PIN)
#define __LCD_CS1_SET()    __LCD_CS1_PORT |=  (1<<__LCD_CS1_PIN)

#define __LCD_CS2_OUT()    __LCD_CS2_DDR |= (1<<__LCD_CS2_PIN)
#define __LCD_CS2_CLR()    __LCD_CS2_PORT &=~ (1<<__LCD_CS2_PIN)
#define __LCD_CS2_SET()    __LCD_CS2_PORT |=  (1<<__LCD_CS2_PIN)

#define __LCD_DC_OUT()    __LCD_DC_DDR |= (1<<__LCD_DC_PIN)
#define __LCD_DC_CLR()    __LCD_DC_PORT &=~ (1<<__LCD_DC_PIN)
#define __LCD_DC_SET()    __LCD_DC_PORT |=  (1<<__LCD_DC_PIN)

#define __LCD_BKL_OUT()   __LCD_BKL_DDR |= (1<<__LCD_BKL_PIN)
#define __LCD_BKL_OFF()   __LCD_BKL_PORT &=~ (1<<__LCD_BKL_PIN)
#define __LCD_BKL_ON()    __LCD_BKL_PORT |=  (1<<__LCD_BKL_PIN)

#define __SD_CS_OUT()    __SD_CS_DDR |= (1<<__SD_CS_PIN)
#define __SD_CS_CLR()    __SD_CS_PORT &=~ (1<<__SD_CS_PIN)
#define __SD_CS_SET()    __SD_CS_PORT |=  (1<<__SD_CS_PIN)

#define __SD_CS_DISABLE()       { __SD_CS_OUT(); __SD_CS_SET(); }

#define __XPT2046_CS_OUT()    __XPT_CS_DDR |= (1<<__XPT_CS_PIN)
#define __XPT2046_CS_CLR()    __XPT_CS_PORT &=~ (1<<__XPT_CS_PIN)
#define __XPT2046_CS_SET()    __XPT_CS_PORT |=  (1<<__XPT_CS_PIN)
#define __XPT2046_IRQ_INP()    __XPT_IRQ_DDR &= ~(1<<__XPT_IRQ_PIN)
#define __XPT2046_IRQ_CLR()    __XPT_IRQ_PORT &=~ (1<<__XPT_IRQ_PIN)
#define __XPT2046_IRQ_SET()    __XPT_IRQ_PORT |=  (1<<__XPT_IRQ_PIN)

#define __XPT2046_IRQ_IN()      {__XPT2046_IRQ_INP();__XPT2046_IRQ_SET(); }
#define __XPT2046_IRQ_READ()    (__XPT_IRQ_PINR & (1<<__XPT_IRQ_PIN))

#define __XPT2046_CS_DISABLE()   { __XPT2046_CS_OUT(); __XPT2046_CS_SET(); }

#endif
