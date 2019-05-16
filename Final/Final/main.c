#define F_CPU 16E6
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <util/delay.h>

#include "PCA9685_ext.h"
#include "ds1621.h"
#include "i2cmaster.h"
#include "lcd.h"

//Function declarations
int adc_read(unsigned char);
void adc_init();
void sensors_init();
void interrupt_init();

//Variable declarations
unsigned char state = 0, direction = 0;
volatile unsigned char proxym1=0;										//0 if the proximity sensor did not detect anything, 1 if it does
volatile unsigned char proxym2=0;
volatile unsigned char proxym3=0;

#define POT1 0															//Addresses to the potentiometers for the adc_read function. Potentiometer 1 connected to pin PC0, 2 to PC1, 3 to PC2
#define POT2 1
#define POT3 2

int main(void)
{
	lcd_init();
	i2c_init();
	adc_init();
	motor_init_pwm(PWM_FREQUENCY_1500);
	interrupt_init();
	sensors_init();

	while(1)
	{
		
		switch(direction)
		{
			case 0:			//Forwards
			switch(state)
			{
				case 0:		//Extending body 1
				//if no bar found retract body 1 back, direction++
				
				
				break;
				case 1:		//Moving middle body
				//If no bar is found go back while checking, if bar is still not found, send error message somehow
				
				
				break;
				case 2:		//Retracting body 3
				//If no bar is found go back while checking, if bar is still not found, send error message somehow
				
				break;
			}
			break;
			case 1:				//Backwards
			switch(state)
			{
				case 0:			//Extending body 3
				//if no bar found retract body 3 back, direction++
				
				break;
				case 1: //Moving middle body
				//If no bar is found go back while checking, if bar is still not found, send error message somehow
				
				break;
				case 2: //Retracting body 1
				//If no bar is found go back while checking, if bar is still not found, send error message somehow
				
				break;
			}
			break;
		}
	}
	
	return 0;
}
}

int adc_read(unsigned char adc_channel){
	ADMUX &= 0xF0;														//Clear any previously used channel, but keep internal reference
	ADMUX |= adc_channel;												//Set the desired channel
	ADCSRA |= (1<<ADSC);												//Start a conversion
	while ( (ADCSRA & (1<<ADSC)) );										//Now wait for the conversion to complete
	return ADC;															//Now we have the result, so we return it to the calling function as a 16 bit unsigned int

}

void adc_init(){														//Setting up the ADC
	ADMUX = (1<<REFS0);													// Select Vref = AVcc
	ADCSRA = (1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)|(1<<ADEN);				//set prescaler to 128 (because we want 10bit resolution, and it requires clock frequency between 50kHz and 200kHz) and turn on the ADC module
}

void interrupt_init(){													//Setup for the interrupt(s) (PCINT2 external interrupt only right now)
	PCICR |= (1<<PCIE2);												//Set PCIE2 to 1 if the external interrupt will be on PCINT[23:16]
	PCMSK2 |= 0xF0;														//(0xFC) Setting up all the pins on portD (PCINT[23:16]) as an interrupt, except the first two (PCINT[17:16])
	sei();																//Setting the SREG I-flag
}

void sensors_init(){													//Initializing the sensor pins
	DDRD &= ~(1<<PORTD2 | 1<<PORTD3 | 1<<PORTD4);						//Connecting Proximity sensors 1,2,3 to PD2,3,4 (no need for pull up or down resistor here)
	DDRD &= ~(1<<PORTD5 | 1<<PORTD6);									//Connecting the 2 Buttons to PD5,6
	//TODO: Do we need pull up or pull down resistor for the 2 switches?
}

ISR(PCINT2_vect){														//Interrupt for the sensor readings (They have to be connected from PD0-PD7)
	if (~PIND & (1 << PIND2)) {proxym1 = 1;}							//If the voltage gets pulled down, it means that the proximity sensor senses something
	else{proxym1 = 0;}

	if (~PIND & (1 << PIND3)) {proxym2 = 1;}
	else{proxym2 = 0;}

	if (~PIND & (1 << PIND4)) {proxym3 = 1;}
	else{proxym3 = 0;}
	
	//TODO: Logic for switches
}