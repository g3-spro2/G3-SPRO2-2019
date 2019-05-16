/*
 * PCA9685.h
 * Author:		sh
 * Created:		18-02-2015 11:00:00
 * Modified:	13-05-2015
 * Version:		1.2
 * Released:	20160510
 */ 



#ifndef PCA9685_H_
#define PCA9685_H_


// You have to use one of the two defines for PCF8574 depending on whether it is an A type or not.
//#define PCF8574_ADR 0x40	// Address for PCF8574 (Chip without A in typenumber)
#define PCF8574_ADR 0x70 	// Address for PCF8574A (A type of chip)


#define PCA_ADR 0xC0
#define PWM_FREQUENCY_1500 3
#define PWM_FREQUENCY_200 30
#define PWM_FREQUENCY_100 60
#define PWM_FREQUENCY_60 100
#define PWM_FREQUENCY_50 121
#define M1 1
#define M2 2
#define M3 3
#define M4 4
#define M5 5
#define M6 6
#define M7 7
#define M8 8

#define PWM_REGISTER(n) (0x06 + n*4)

#define M1_PWM PWM_REGISTER(8)
#define M1_IN1 PWM_REGISTER(10)
#define M1_IN2 PWM_REGISTER(9)

#define M2_PWM PWM_REGISTER(13)
#define M2_IN1 PWM_REGISTER(11)
#define M2_IN2 PWM_REGISTER(12)

#define M3_PWM PWM_REGISTER(2)
#define M3_IN1 PWM_REGISTER(4)
#define M3_IN2 PWM_REGISTER(3)

#define M4_PWM PWM_REGISTER(7)
#define M4_IN1 PWM_REGISTER(5)
#define M4_IN2 PWM_REGISTER(6)

#define M5_PWM PWM_REGISTER(0)
#define M6_PWM PWM_REGISTER(1)
#define M7_PWM PWM_REGISTER(14)
#define M8_PWM PWM_REGISTER(15)
#define M5_IN1 0
#define M5_IN2 1
#define M6_IN1 2
#define M6_IN2 3
#define M7_IN1 4
#define M7_IN2 5
#define M8_IN1 6
#define M8_IN2 7

#define LOW 0
#define HIGH 1
#define STOP 0
#define CW 1
#define CCW 2
#define BRAKE 3

void motor_set1438_controlpin(unsigned char pin_adr, unsigned char level);
void motor_set8574_controlpin(unsigned char bit_number, unsigned char level);
void motor_set_state(unsigned char motor_number, unsigned char state);
void motor_set_pwm(unsigned char motor_number, unsigned int on_value, unsigned int off_value);
void motor_init_pwm(unsigned char frequency);


#endif