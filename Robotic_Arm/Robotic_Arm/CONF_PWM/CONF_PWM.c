/*
 * CONF_PWM.c
 *
 * Created: 8/05/2025 15:23:42
 *  Author: bsmor
 */ 
#include <avr/io.h>

void initPWM1(){
	DDRB |= (1 << DDB1); // PB1 as output (OC1A)
	DDRB |= (1 << DDB2); // PB2 as output (OC1B)
	TCCR1A = 0;
	TCCR1A |= (1 << COM1A1); // Set as non inverted OC1A
	TCCR1A |= (1 << COM1B1); // Set as non inverted OC1B
	TCCR1A |= (1 << WGM10); // Set mode 5 => Fast PWM and top = 0xFF
	
	TCCR1B = 0;
	TCCR1B |= (1 << WGM12);
	TCCR1B |= (1 << CS11) | (1 << CS10); // Prescaler 64
}

void initPWM0(){
	DDRD |= (1 << DDD6); // PD6 as output (OC0A)
	DDRD |= (1 << DDD5); // PD5 as output (OC0B)
	TCCR0A = 0;
	TCCR0A |= (1 << COM0A1); // Set as non inverted OC0A
	TCCR0A |= (1 << COM0B1); // Set as non inverted OC0B
	TCCR0A |= (1 << WGM01) | (1 << WGM00); // Set mode 3 => Fast PWM and top = 0xFF
	
	TCCR0B = 0;
	TCCR0B |= (1 << CS01) | (1 << CS00); // Prescaler 64
}