/*
M1:Claw 1
M2:Claw 2
M3:Claw 3
M4:Body 1
M5:Body 2
CCW:Extend/Open
CW:Retract/Close
Potentiometer 1 --> PC1
Potentiometer 2 --> PC2
Potentiometer 3 --> PC3
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
void stop_retracting_error();
void open_claws_on_body_x(unsigned char, unsigned int);
void extend_body_x(unsigned char, unsigned int);
void wait_for_input_from_sensors(unsigned char);
void retract_body_x(unsigned char, unsigned int);
void stop_and_close_claws_x(unsigned char, unsigned int);


//Variable declarations
unsigned char state = 1, direction = 0;									//Direction 1:Forward 0:Backward
unsigned char state_variable_x = 0;
volatile unsigned char proxym1=0;										//0 if the proximity sensor did not detect anything, 1 if it does
volatile unsigned char proxym2=0;
volatile unsigned char proxym3=0;
volatile unsigned char edge1 = 0;
volatile unsigned char edge2 = 0;

#define body_retract_speed 0x1FF
#define body_extend_speed 0x1FF
#define claw_close_speed 0x1FF
#define claw_open_speed 0x1FF

#define desired_closed_claw_angle1 512
#define desired_closed_claw_angle2 512
#define desired_closed_claw_angle3 512
#define desired_open_claw_angle1 512
#define desired_open_claw_angle2 512
#define desired_open_claw_angle3 512

#define claw_1 3
#define claw_2 5
#define claw_3 2
#define body_1 4
#define body_3 1

int main(void)
{
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
		switch (state)
		{
		case 1:
			if (direction == 1)
			{state_variable_x = 1;}

			else if(direction == 0)
			{state_variable_x = 3;}
			
			open_claws_on_body_x(state_variable_x, claw_open_speed);
			extend_body_x(state_variable_x,body_extend_speed);
			wait_for_input_from_sensors(1);
			if (proxym1)
			{
				stop_and_close_claws_x(state_variable_x,claw_close_speed);
				state = 2;
			}
			else if(edge1 || edge2)
			{
				retract_body_x(state_variable_x,body_retract_speed);
				wait_for_input_from_sensors(1);
				if (proxym1)
				{
					stop_and_close_claws_x(state_variable_x,claw_close_speed);
					direction = 0;
				}
				else if(edge1 || edge2)
				{stop_retracting_error();}
			}
		break;
		
		case 2:
			open_claws_on_body_x(2,claw_open_speed);
			if (direction == 1)
			{
				retract_body_x(1,body_retract_speed);
				extend_body_x(3,body_extend_speed);
			}
			else if(direction == 0)
			{
				retract_body_x(3,body_retract_speed);
				extend_body_x(1,body_extend_speed);
			}
			wait_for_input_from_sensors(2);
			if (proxym2)
			{
				stop_and_close_claws_x(2,claw_close_speed);
				state = 3;
			} 
			else if(edge1 || edge2)
			{stop_retracting_error();}
		break;
		
		case 3:
			if (direction == 1)
			{state_variable_x = 2;} 
			
			else if(direction == 0)
			{state_variable_x = 1;}
			open_claws_on_body_x(state_variable_x,claw_open_speed);
			extend_body_x(state_variable_x,body_extend_speed);
			wait_for_input_from_sensors(3);
			if (proxym3)
			{
				stop_and_close_claws_x(state_variable_x,claw_close_speed);
				state = 1;
			}
			else if(edge1 || edge2)
			{stop_retracting_error();}
		break;
		}
	}
	return 0;
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
	PORTD |= 0xFF;														//Pull-up resistors for all pin on portD
}

void stop_retracting_error(){
	motor_set_state(M1,BRAKE);
	motor_set_state(M2,BRAKE);
	motor_set_state(M3,BRAKE);
	motor_set_state(M4,BRAKE);
	motor_set_state(M5,BRAKE);
	while(1);
}

void open_claws_on_body_x(unsigned char x, unsigned int speed){

	int desired_open_claw_angle = 0;
	unsigned char claw_pin = 0;
	switch (x)
	{
		case 1:
		desired_open_claw_angle = desired_open_claw_angle1;
		claw_pin = claw_1;
		break;
		
		case 2:
		desired_open_claw_angle = desired_open_claw_angle2;
		claw_pin = claw_2;
		break;
		
		case 3:
		desired_open_claw_angle = desired_open_claw_angle3;
		claw_pin = claw_3;
		break;
	}
	motor_set_state(claw_pin,CCW);
	motor_set_pwm(claw_pin,0,speed);
	while (adc_read(x)<desired_open_claw_angle);
	motor_set_state(claw_pin,BRAKE);
}

void extend_body_x(unsigned char x, unsigned int speed){
	unsigned char body_number = 0;
	if (x == 1)
	{
		body_number = body_1;
	}
	else if(x == 3)
	{
		body_number = body_3;
	}
	motor_set_state(body_number,CCW);
	motor_set_pwm(body_number,0,speed);
}

void wait_for_input_from_sensors(unsigned char state){
	switch (state)
	{
		case 1:
		while (!(proxym1||edge1||edge2));
		break;
		
		case 2:
		while (!(proxym2||edge1||edge2));
		break;
		
		case 3:
		while (!(proxym3||edge1||edge2));
		break;
	}
}

void retract_body_x(unsigned char x, unsigned int speed){
	int body_number = 0;
	if (x == 1)
	{
		body_number = body_1;
	}
	else if(x == 3)
	{
		body_number = body_3;
	}
	motor_set_state(body_number,CW);
	motor_set_pwm(body_number,0,speed);
}

void stop_and_close_claws_x(unsigned char x, unsigned int speed){
	int desired_closed_claw_angle = 0;
	unsigned char claw_pin = 0;
	
	switch (x)
	{
		case 1:
		desired_closed_claw_angle = desired_closed_claw_angle1;
		claw_pin = claw_1;
		break;
		
		case 2:
		desired_closed_claw_angle = desired_closed_claw_angle2;
		claw_pin = claw_2;
		break;
		
		case 3:
		desired_closed_claw_angle = desired_closed_claw_angle3;
		claw_pin = claw_3;
		break;
	}

	motor_set_state(M4,BRAKE);
	motor_set_state(M5,BRAKE);
	motor_set_state(claw_pin,CW);
	motor_set_pwm(claw_pin,0,speed);
	while (adc_read(x)>desired_closed_claw_angle);
	motor_set_state(claw_pin,BRAKE);
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

/*		switch(state)
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
						state = 2;											//Move to next state
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
						else if (edge1){error_stop_all();}					//If an edge is detected, stop every motor and wait
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
						state = 2;											//Move to next state
					}
					else if (edge1)
					{
						motor_set_state(M5,CW);
						motor_set_pwm(M5,0,0x1FF);
						while (!(proxym3||edge1||edge2));
						if (proxym3)
						{
							motor_set_state(M5,BRAKE);
							motor_set_state(M3,CW);							//Close Claw 3
							motor_set_pwm(M3,0,0x1FF);
							while (adc_read(claw_angle3)<desired_open_claw_angle3);	//Wait until claw 3 angle reaches the desired closed angle
							motor_set_state(M3,BRAKE);
							direction = (direction) ? 0: 1;
						}
						else if (edge1||edge2){error_stop_all();}			//If an edge is detected, stop every motor and wait
					}
				}
			break;

			case 2:		//State 2
				motor_set_state(M2,CCW);									//Open Claw 2
				motor_set_pwm(M2,0,0x1FF);
				while (adc_read(claw_angle2)<desired_open_claw_angle2);
				motor_set_state(M2,BRAKE);									//Stop Claw 2
				
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
					motor_set_state(M2,CW);									//Close Claw 2
					motor_set_pwm(M2,0,0x1FF);
					while (adc_read(claw_angle2)>desired_closed_claw_angle2);
					motor_set_state(M2,BRAKE);								//Stop Claw 2
					state = 3;												//Move to next state
				}

				else if (edge1||edge2){error_stop_all();}					//If an edge is detected, stop every motor and wait

			break;
			
			case 3:		//State 3
			if (direction)													//If direction is 1 execute the forward motion
			{
				motor_set_state(M3,CCW);									//Open Claw 3
				motor_set_pwm(M3,0,0x1FF);
				while (adc_read(claw_angle3)<desired_open_claw_angle3);		//Wait until claw 3 angle reaches the desired open angle
				motor_set_state(M3,BRAKE);									//Stop Claw 3
				
				motor_set_state(M5,CW);										//Move body 3
				motor_set_pwm(M5,0,0x1FF);
				
				while (!(proxym3||edge1||edge2));							//Wait until we either detect a bar, or edges
				
				if (proxym3)												//If a bar is detected
				{
					motor_set_state(M5,BRAKE);								//Stop body 3
					motor_set_state(M3,CW);									//Close Claw 3
					motor_set_pwm(M3,0,0x1FF);
					while(adc_read(claw_angle3)>desired_closed_claw_angle3);//Wait until claw angle reaches the desired closed angle
					motor_set_state(M3,BRAKE);								//Stop Claw 3
					state = 1;												//Move to next state
				} 
				else if(edge1||edge2){error_stop_all();}					//If an edge is detected, stop every motor and wait
			}
			else if (!direction)											//If direction is 0 execute the backward motion
			{
				motor_set_state(M1,CCW);									//Open Claw 1
				motor_set_pwm(M1,0,0x1FF);
				while (adc_read(claw_angle1)<desired_open_claw_angle1);		//Wait until claw 1 angle reaches the desired open angle
				motor_set_state(M1,BRAKE);									//Stop Claw 1

				motor_set_state(M4,CW);										//Move body 1
				motor_set_pwm(M4,0,0x1FF);
				
				while (!(proxym1||edge1||edge2));							//Wait until we either detect a bar, or edges
				
				if (proxym1)												//If a bar is detected
				{
					motor_set_state(M4,BRAKE);								//Stop body 1
					motor_set_state(M1,CW);									//Close Claw 1
					motor_set_pwm(M1,0,0x1FF);
					while(adc_read(claw_angle1)>desired_closed_claw_angle1);//Wait until claw angle reaches the desired closed angle
					motor_set_state(M1,BRAKE);								//Stop Claw 1
					state = 1;												//Move to next state
				}
				else if(edge1||edge2){error_stop_all();}					//If an edge is detected, stop every motor and wait
			}
			break;
		}*/
