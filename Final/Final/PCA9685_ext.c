/*
 * PCA9685.c
 * Extended version - 8 motors
 * Author: sh
 * Created:		18-02-2015 11:00:00
 * Modified:	09-04-2015
 * Version:		1.2
 * Release:		20160510
 */ 

#include "PCA9685_ext.h"
#include "i2cmaster.h"

void motor_set8574_controlpin(unsigned char bit_number, unsigned char level)
{
	static unsigned char out_byte = 0;
	if(level == LOW)
		out_byte &= ~(1<<bit_number);
	if(level==HIGH)
		out_byte |= (1<<bit_number);
	i2c_start(PCF8574_ADR + I2C_WRITE);
	i2c_write(out_byte);
	i2c_stop();
}


void motor_set1438_controlpin(unsigned char pin_adr, unsigned char level)
{
	i2c_start(PCA_ADR + I2C_WRITE);
	i2c_write(pin_adr);
	if(level == LOW)
	{
		i2c_write(0x00); i2c_write(0x00); i2c_write(0x10);
	}
	if(level==HIGH)
	{
		i2c_write(0x10); i2c_write(0x00); i2c_write(0x00);
	}
	i2c_stop();
}

void motor_set_state(unsigned char motor_number, unsigned char state)
{
	unsigned char in1=0, in2=0;
	
	switch(motor_number)
	{
		case M1:	in1=M1_IN1+1; in2=M1_IN2+1; break; // High byte of reg
		case M2: 	in1=M2_IN1+1; in2=M2_IN2+1; break; // High byte of reg
		case M3:	in1=M3_IN1+1; in2=M3_IN2+1; break; // High byte of reg
		case M4:	in1=M4_IN1+1; in2=M4_IN2+1; break; // High byte of reg
		case M5:	in1=M5_IN1; in2=M5_IN2; break;		// Set bit position
		case M6:	in1=M6_IN1; in2=M6_IN2; break;		// Set bit position
		case M7:	in1=M7_IN1; in2=M7_IN2; break;		// Set bit position
		case M8:	in1=M8_IN1; in2=M8_IN2; break;		// Set bit position
	}
	if(motor_number < 5)			// M1 - M4
	{
		switch(state)
		{
		
			case CCW:		motor_set1438_controlpin(in1, LOW); motor_set1438_controlpin(in2, HIGH); break;  // IN1=0, IN2=1
			case CW:		motor_set1438_controlpin(in1, HIGH); motor_set1438_controlpin(in2, LOW); break;	// IN1=1, IN2=0
			case BRAKE:		motor_set1438_controlpin(in1, HIGH); motor_set1438_controlpin(in2, HIGH);break;	// IN1=1, IN2=1
			case STOP:
			default:		motor_set1438_controlpin(in1, LOW); motor_set1438_controlpin(in2, LOW);	// IN1=0, IN2=0
		}
	}
	else							// M5 - M8
	{
		switch(state)
		{
			case CCW:		motor_set8574_controlpin(in1, LOW); motor_set8574_controlpin(in2, HIGH); break;  // IN1=0, IN2=1
			case CW:		motor_set8574_controlpin(in1, HIGH); motor_set8574_controlpin(in2, LOW); break;	// IN1=1, IN2=0
			case BRAKE:		motor_set8574_controlpin(in1, HIGH); motor_set8574_controlpin(in2, HIGH);break;	// IN1=1, IN2=1
			case STOP:
			default:		motor_set8574_controlpin(in1, LOW); motor_set8574_controlpin(in2, LOW);	// IN1=0, IN2=0
		}
	}
}

void motor_set_pwm(unsigned char motor_number, unsigned int on_value, unsigned int off_value)
{
	unsigned char pwm_reg=0;
	if(on_value > 0xFFF) on_value = 0;
	if(off_value > 0xFFF) off_value = 0;
	
	switch(motor_number)
	{
		case M1:	pwm_reg = M1_PWM; break;
		case M2:	pwm_reg = M2_PWM; break;
		case M3:	pwm_reg = M3_PWM; break;
		case M4:	pwm_reg = M4_PWM; break;
		case M5:	pwm_reg = M5_PWM; break;
		case M6:	pwm_reg = M6_PWM; break;
		case M7:	pwm_reg = M7_PWM; break;
		case M8:	pwm_reg = M8_PWM; break;
	}
	i2c_start(PCA_ADR + I2C_WRITE);
	i2c_write(pwm_reg);
	i2c_write(on_value&0x0FF);
	on_value >>= 8;
	i2c_write(on_value);
	i2c_write(off_value & 0x0FF);
	off_value >>= 8;
	i2c_write(off_value);
	i2c_stop();
}

void motor_init_pwm(unsigned char frequency)  // INIT PCA9685 AND PCA8574
{
	i2c_start(PCA_ADR + I2C_WRITE);
	i2c_write(0x00);
	i2c_write(0b00110000);					// sleep and autoinc
	i2c_stop();
	
	i2c_start(PCA_ADR + I2C_WRITE);
	i2c_write(0xFE);
	i2c_write(frequency);						
	
	i2c_start(PCA_ADR + I2C_WRITE);
	i2c_write(0x00);
	i2c_write(0b00100000);					// autoinc
	i2c_stop();	
	
	i2c_start(PCF8574_ADR + I2C_WRITE);		// All PCF8574 outputs low
	i2c_write(0x00);
	i2c_stop();
}