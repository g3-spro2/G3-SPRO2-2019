/*
M1:Claw 1
M2:Claw 2
M3:Claw 3
M4:Body 1
M5:Body 2
CCW:Extend/Open
CW:Retract/Close
*/
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
void error_stop_all();

//Variable declarations
unsigned char state = 1, direction = 0;									//Direction 1:Forward 0:Backward
volatile unsigned char proxym1=0;										//0 if the proximity sensor did not detect anything, 1 if it does
volatile unsigned char proxym2=0;
volatile unsigned char proxym3=0;
volatile unsigned char edge1 = 0;
volatile unsigned char edge2 = 0;
int desired_closed_claw_angle1 = 512;
int desired_closed_claw_angle2 = 512;
int desired_closed_claw_angle3 = 512;
int desired_open_claw_angle1 = 512;
int desired_open_claw_angle2 = 512;
int desired_open_claw_angle3 = 512;

#define claw_angle1 0															//Addresses to the potentiometers for the adc_read function. Potentiometer 1 connected to pin PC0, 2 to PC1, 3 to PC2
#define claw_angle2 1
#define claw_angle3 2

int main(void)
{
	lcd_init();
	i2c_init();
	adc_init();
	motor_init_pwm(PWM_FREQUENCY_1500);
	interrupt_init();
	sensors_init();
	
	motor_set_state(M1,STOP);
	motor_set_state(M2,STOP);
	motor_set_state(M3,STOP);
	motor_set_state(M4,STOP);
	motor_set_state(M5,STOP);

	while(1)
	{
		switch(direction)
		{
			case 1:		//State 1
				if (direction)
				{
					motor_set_state(M1,CCW);
					motor_set_pwm(M1,0,0x1FF);
					while (adc_read(claw_angle1)>desired_closed_claw_angle1);
					motor_set_state(M1,BRAKE);
					
					motor_set_state(M4,CCW);
					motor_set_pwm(M4,0,0x1FF);
					
					while (!(proxym1||edge1||edge2));
					
					if (proxym1)
					{
						motor_set_state(M4,BRAKE);
						motor_set_state(M1,CW);
						motor_set_pwm(M1,0,0x1FF);
						while (adc_read(claw_angle1)<desired_open_claw_angle1);
						motor_set_state(M1,BRAKE);
						state = 2;
					} 
					else if (edge1)
					{
						motor_set_state(M4,CW);
						motor_set_pwm(M4,0,0x1FF);
						while (!(proxym1||edge1||edge2));
						if (proxym1)
						{
							motor_set_state(M4,BRAKE);
							motor_set_state(M1,CW);
							motor_set_pwm(M1,0,0x1FF);
							direction = 0;
						} 
						else if (edge1){error_stop_all();}
					}
				}
				else if (!direction)
				{
					motor_set_state(M3,CCW);
					motor_set_pwm(M3,0,0x1FF);
					while (adc_read(claw_angle3)>desired_closed_claw_angle3);
					motor_set_state(M3,BRAKE);
					
					motor_set_state(M5,CCW);
					motor_set_pwm(M5,0,0x1FF);
					
					while (!(proxym3||edge1||edge2));
					
					if (proxym3)
					{
						motor_set_state(M5,BRAKE);
						motor_set_state(M3,CW);
						motor_set_pwm(M3,0,0x1FF);
						while (adc_read(claw_angle3)<desired_open_claw_angle3);
						motor_set_state(M3,BRAKE);
						state = 2;
					}
					else if (edge1)
					{
						motor_set_state(M5,CW);
						motor_set_pwm(M5,0,0x1FF);
						while (!(proxym3||edge1||edge2));
						if (proxym3)
						{
							motor_set_state(M5,BRAKE);
							motor_set_state(M3,CW);
							motor_set_pwm(M3,0,0x1FF);
							direction = 0;
						}
						else if (edge1||edge2){error_stop_all();}
					}
				}
			break;

			case 2:		//State 2
				motor_set_state(M2,CCW);
				motor_set_pwm(M2,0,0x1FF);
				while (adc_read(claw_angle2)<desired_open_claw_angle2);
				motor_set_state(M2,BRAKE);
				
				if (direction)
				{
					motor_set_state(M5,CCW);
					motor_set_pwm(M5,0,0x1FF);	
					motor_set_state(M4,CW);
					motor_set_pwm(M4,0,0x1FF);
				}
				else if (!direction)
				{
					motor_set_state(M5,CW);
					motor_set_pwm(M5,0,0x1FF);
					motor_set_state(M4,CCW);
					motor_set_pwm(M4,0,0x1FF);
				}
				while (!(proxym2||edge1||edge2));

				if (proxym2)
				{
					motor_set_state(M4,BRAKE);
					motor_set_state(M5,BRAKE);
					motor_set_state(M2,CW);
					motor_set_pwm(M2,0,0x1FF);
					while (adc_read(claw_angle2)>desired_closed_claw_angle2);
					motor_set_state(M2,BRAKE);
					state = 3;
				} 
				else if (edge1||edge2)
				{
					error_stop_all();
				}
			break;
			
			case 3:		//State 3
			if (direction)
			{
				motor_set_state(M3,CCW);
				motor_set_pwm(M3,0,0x1FF);
				while (adc_read(claw_angle3)<desired_open_claw_angle3);
				motor_set_state(M3,BRAKE);
				
				motor_set_state(M5,CW);
				motor_set_pwm(M5,0,0x1FF);
				
				while (!(proxym3||edge1||edge2));
				
				if (proxym3)
				{
					motor_set_state(M5,BRAKE);
					motor_set_state(M3,CW);
					motor_set_pwm(M3,0,0x1FF);
					while(adc_read(claw_angle3)>desired_closed_claw_angle3);
					motor_set_state(M3,BRAKE);
					state = 1;
				} 
				else if()
				{
					error_stop_all();
				}
			}
			else if (!direction)
			{
				motor_set_state(M1,CCW);
				motor_set_pwm(M1,0,0x1FF);
				while (adc_read(claw_angle1)<desired_open_claw_angle1);
				motor_set_state(M1,BRAKE);
				
				motor_set_state(M4,CW);
				motor_set_pwm(M4,0,0x1FF);
				
				while (!(proxym1||edge1||edge2));
				
				if (proxym1)
				{
					motor_set_state(M4,BRAKE);
					motor_set_state(M1,CW);
					motor_set_pwm(M1,0,0x1FF);
					while(adc_read(claw_angle1)>desired_closed_claw_angle1);
					motor_set_state(M1,BRAKE);
					state = 1;
				}
				else if()
				{
					error_stop_all();
				}
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
	PCMSK2 |= 0xF0;														//(0xFC) Setting up all the pins on portD (PCINT[23:16]) as an interrupt, except the first two (PCINT[17:16])
	PCICR |= (1<<PCIE2);												//Set PCIE2 to 1 if the external interrupt will be on PCINT[23:16]
	EICRA |= (1<<ISC10 | 1<<ISC00);										//Set up INT0 and INT1 (PD2,PD3) as external interrupts
	EIMSK |= (1<<INT0 | 1<<INT1);										//Enabling INT0 and INT1 interrupts
	sei();																//Setting the SREG I-flag
}

void sensors_init(){													//Initializing the sensor pins
	DDRD &= ~(1<<PORTD4 | 1<<PORTD5 | 1<<PORTD6);						//Connecting Proximity sensors 1,2,3 to PD4,5,6 (no need for pull up or down resistor here)
	DDRD &= ~(1<<PORTD2 | 1<<PORTD3);									//Connecting the 2 Buttons to PD5,6
	PORTD |= 0xFF;														//Pullup resistors for all pin on portD
	//TODO: Do we need pull up or pull down resistor for the 2 switches?
}

ISR(PCINT2_vect){														//Interrupt for the sensor readings (They have to be connected from PD0-PD7)
	if (~PIND & (1 << PIND4)) {proxym1 = 1;}							//If the voltage gets pulled down, it means that the proximity sensor senses something
	else{proxym1 = 0;}

	if (~PIND & (1 << PIND5)) {proxym2 = 1;}
	else{proxym2 = 0;}

	if (~PIND & (1 << PIND6)) {proxym3 = 1;}
	else{proxym3 = 0;}
}

ISR(INT0_vect){															//Interrupt for edge detection 1
	if (~PIND & (1 << PIND2)) {edge1 = 1;}
	else{edge1 = 0;}
}

ISR(INT1_vect){															//Interrupt for edge detection 2
	if (~PIND & (1 << PIND3)) {edge2 = 1;}
	else{edge2 = 0;}
}

void error_stop_all(){
	motor_set_state(M1,BRAKE);
	motor_set_state(M2,BRAKE);
	motor_set_state(M3,BRAKE);
	motor_set_state(M4,BRAKE);
	motor_set_state(M5,BRAKE);
	while(1);
}