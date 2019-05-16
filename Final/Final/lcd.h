/*
    File:       lcd.h
    
    C header file for HD44780 LCD module using a 74HCT164 serial in, parallel 
    out out shift register to operate the DEM20231 LCD in 8-bit mode.  
*/
#ifndef LCD_H_INCLUDED
#define LCD_H_INCLUDED

#include <inttypes.h>

/* AVR port and pins connected to '164 and/or LCD */
#define LCD_PORT                PORTB
#define LCD_ENABLE_PIN          0
#define LCD_RSDS_PIN            5
#define LCD_CLOCK_PIN           4

/* functions */
void lcd_init(void);
void lcd_send_cmd(void);
void lcd_send_char(void);
void lcd_home(void);
void lcd_gotoxy(uint8_t, uint8_t);
void lcd_putc(char c);
void lcd_clear(void);

#endif