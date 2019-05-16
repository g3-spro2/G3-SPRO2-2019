/*
    File:       lcd.c
    
    C header file for HD44780 LCD module using a 74HC164 serial in, parallel 
    out shift register to operate the DEM20231 LCD in 8-bit mode. 
	The putchr function provides an interface the stdout filestream. 
	Erik Henneberg - 2014.06.03, AD 2014.10   
*/
#ifndef F_CPU
#define F_CPU 16E6 // CPU clock 16 MHz
#endif

#include "lcd.h"
#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

int putchr(char c, FILE *stream);

FILE mystdout = FDEV_SETUP_STREAM(putchr, NULL, _FDEV_SETUP_WRITE);

/*
 * Loads a byte into the shift register, 74HC164.
 *
 * Parameters: out_byte   The byte to load into the shift register
*/
void lcd_load_byte(uint8_t out_byte)
{
	LCD_PORT &= ~_BV(LCD_CLOCK_PIN);		// CLK = 0
	for(unsigned char i=0; i<8; i++)		// 8 shift reg. bits
	{
		if (out_byte & 0x80)				// bit high
			LCD_PORT |=_BV(LCD_RSDS_PIN); 
		else
			LCD_PORT &= ~_BV(LCD_RSDS_PIN);	// bit low
		out_byte = out_byte << 1;
		LCD_PORT |= _BV(LCD_CLOCK_PIN);		// strobe CLK
		LCD_PORT &= ~_BV(LCD_CLOCK_PIN);
	}
}

/* Strobes the E signal on the LCD to read the byte from the 74HC164. */
void strobe_E(void)
{
	LCD_PORT |= _BV(LCD_ENABLE_PIN);		// E = 1
	LCD_PORT |= _BV(LCD_ENABLE_PIN);		// E = 1 twice to extend pulse
	LCD_PORT &= ~_BV(LCD_ENABLE_PIN);		// E = 0
}

void displayControl(uint8_t cmd_byte)
{
	lcd_load_byte(cmd_byte);
	LCD_PORT &= ~_BV(LCD_RSDS_PIN);  // RS = 0
	strobe_E();
	_delay_us(40);					// default "after command" delay
}

void lcd_init()
{
  DDRB = 0x31;
	stdout = &mystdout;		// address of stdout 
	_delay_ms(30);			// 30ms power on delay for the DEM20231 LCD
	displayControl(0x38);	// display off
	displayControl(0x08);	// display off
	displayControl(0x01);	// Clear display
	_delay_ms(2);			// 2ms clear LCD delay for the DEM20231 LCD	
	displayControl(0x06);	// entry mode set
	lcd_home();				// cursor home
	displayControl(0x0C);	// display ON
}

/*
 * Loads the byte into the shift register and then sends it to the LCD as a char
 * Parameters:   c               The byte (character) to display
*/
void lcd_putc( char c)
{
	lcd_load_byte(c);
	LCD_PORT |= _BV(LCD_RSDS_PIN);		// RS = 1
	strobe_E();
	_delay_us(44);					// default "after char out" delay
}

/* putchr provides an interface to avr gcc stdio stdout to be used
   for formatted output with printf
*/
int putchr(char c, FILE *stream)
{
	lcd_putc(c);
 	return 0;	
}

/* Moves the cursor to the home position. */
void lcd_home()
{
	displayControl(0x02);
	_delay_ms(2);			/* 2ms LCD home delay for the DEM20231 LCD */
}

//***** Function:  lcd_gotoxy *****
// moves the display cursor
// Input:  row, position
// Output: void

void lcd_gotoxy(uint8_t row, uint8_t position)
{
	unsigned char controlByte;
	controlByte = 0x80 + position - 1;
	switch (row)
	{
	 case 1 : displayControl(controlByte); break;
	 case 2 : displayControl(0x40+controlByte); break;
//	 case 3 : displayControl(0x14+controlByte); break;
//	 case 4 : displayControl(0x54+controlByte); break;
	 default : break;
	} 
}

//***** Function:  lcd_clear *****
// clears the display contents
// Input:  void
// Output: void

void lcd_clear(void)
{
	displayControl(0x01);
	_delay_ms(2);			/* 2ms clear LCD delay for the DEM20231 LCD */
}

