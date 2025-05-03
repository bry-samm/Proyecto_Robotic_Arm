/*
 * Robotic_Arm.c
 * 
 * Created: 23/04/2025 23:03:32
 * Author: Bryan Morales
 * Description:
 */
//************************************************************************************
// Encabezado (librerías)
#define	F_CPU 16000000
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>

uint8_t		multiplexar_ADC = 0;
uint8_t		ADC_value;

uint8_t		modo = 1;
uint8_t		cant_modo = 4;

//Valores para primer servo
#define SERVO_MIN  9   
#define SERVO_MAX  36 

//Valores para segundo servo
#define SERVO_MIN_2  9   
#define SERVO_MAX_2  36  

//Valores para tercer servo
#define SERVO_MIN_3  9   
#define SERVO_MAX_3  36  

//Valores para el cuarto servo
#define SERVO_MIN_4  9   
#define SERVO_MAX_4  36   

//************************************************************************************
// Function prototypes
void setup();
void initADC();
void initPWM0();
void initPWM1();
void initUART();

void manual();
void save_data();
void escribir_angulo();
//************************************************************************************
// Main Function
int main(void)
{
	setup();
	while (1)
	{
		switch (modo){
			case 1:
			manual();
			break;
			
			case 2:
			save_data();
			break;
			
			case 3:
			escribir_angulo();
			break;
			
		}
	}
}
//************************************************************************************
// NON-INterrupt subroutines
void setup(){
	cli();
	CLKPR = (1 << CLKPCE);
	CLKPR = (1 << CLKPS2); // Prescaler 16 => Configurate to 1MHz
	
	//Configuración de las leds para mostrar modo
	DDRB |= (1 << PORTB3) | (1 << PORTB4);
	PORTB &= ~((1 << PORTB3) | (1 << PORTB4));
	
	// Configuración de botones en PORTC
	DDRC &= ~((1 << PORTC0) | (1 << PORTC1));  // PC0 y PC1 como entradas
	PORTC |= (1 << PORTC0) | (1 << PORTC1);    // Activar pull-ups
	
	// Habilitar interrupciones en PORTC (PC0 y PC1)
	PCICR |= (1 << PCIE1);                         // Habilita grupo de interrupciones de PORTC
	PCMSK1 |= (1 << PCINT8) | (1 << PCINT9);       // Habilita interrupciones en PC0 y PC1

	
	initPWM0();
	initPWM1();
	initADC();
	initUART();
	
	sei();
}

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

void initADC(){
	ADMUX = 0;
	ADMUX	|= (1<<REFS0);  // 5V as reference

	ADMUX	|= (1 << ADLAR); // Left justification
	
	ADMUX	|= (1 << MUX1) | (1<< MUX0); //Select ADC3 to have a start value
	
	ADCSRA	= 0;
	ADCSRA	|= (1 << ADPS1) | (1 << ADPS0); // Sampling frequency = 125kHz "sampling = muestreo"
	ADCSRA	|= (1 << ADIE); // Enable interruption
	ADCSRA	|= (1 << ADEN); // Enable ADC
	
	ADCSRA	|= (1<< ADSC); // Start conversion
}

void initUART(){
	//Step1 : configurate pin PD0 (rx) and PD1 (tx)
	DDRD |= (1 << DDD1);
	DDRD &= ~(1 << DDD0);
	//Step 2: UCSR0A
	UCSR0A |= (1 << U2X0); //double speed
	//Step 3: UCSR0B: enable interrupts, enable recibir, enable transmition
	UCSR0B |= (1 << RXCIE0) | (1 << RXEN0) | (1 << TXEN0);
	//Step 4 : UCSR0C
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
	//Step 5: UBRR0 0 103 => 9600 16kHz
	UBRR0 = 12;
}

void manual(){
	PORTB |= (1 << PORTB3);
	PORTB &= ~(1 << PORTB4);
	
	ADCSRA |= (1 << ADSC);
}

void save_data(){
	PORTB |= (1 << PORTB4);
	PORTB &= ~(1 << PORTB3);
}

void escribir_angulo(){
	PORTB |= (1 << PORTB3) | (1 << PORTB4); // Ambos LEDs
}

//************************************************************************************
// Interrupt subroutines

ISR(ADC_vect){
	
	if (modo != 1) return; // Solo ejecutar si estamos en modo manual
	
	multiplexar_ADC = ADMUX & 0x0F; //Create a mask
	ADC_value = ADCH;
	if (multiplexar_ADC == 3){
		uint8_t angle = (ADC_value * 180) / 255;
		OCR1A = SERVO_MIN + (angle * (SERVO_MAX - SERVO_MIN) / 180);
		// Change ADC
		ADMUX = (ADMUX & 0xF0) | 4; // I made the "&" with 0xF0 because in the high bits are the configuration of the MUX and i want to save this values
	}
	else if (multiplexar_ADC == 4)
	{
		uint8_t angle2 = (ADC_value * 180) / 255;
		OCR1B = SERVO_MIN_2 + (angle2 * (SERVO_MAX_2 - SERVO_MIN_2) / 180);
		// Change ADC
		ADMUX = (ADMUX & 0xF0) | 5; // I do not write ADMUX |= (ADMUX & 0xF0) | 3; because y want to erase de prevous configuration of the MUX ###################
	}
	else if (multiplexar_ADC == 5){

		uint8_t angle3 = (ADC_value * 180) / 255;
		OCR0A = SERVO_MIN_3 + (angle3 * (SERVO_MAX_3 - SERVO_MIN_3) / 180);
		// Change ADC
		ADMUX = (ADMUX & 0xF0) | 6; 
	}
	else if (multiplexar_ADC == 6){
		
		uint8_t angle4 = (ADC_value * 180) / 255;
		OCR0B = SERVO_MIN_4 + (angle4 * (SERVO_MAX_4 - SERVO_MIN_4) / 180);
		
		ADMUX = (ADMUX & 0xF0) | 3;
	}

	ADCSRA |= (1 << ADSC); // Start new conversion
}


#define MAX_BUFFER 20
volatile char buffer[MAX_BUFFER];
volatile uint8_t index = 0;

ISR(USART_RX_vect) {
	
	if (modo != 3) return; // Solo ejecutar 
	
	char recibido = UDR0;

	// Cuando llega fin de línea (enter)
	if (recibido == '\n' || recibido == '\r') {
		buffer[index] = '\0';  // Terminar cadena

		// Variables temporales
		uint8_t valores[4] = {0};
		uint8_t val_idx = 0;
		uint16_t temp = 0;

		for (uint8_t i = 0; i <= index; i++) {
			char c = buffer[i];

			if ((c >= '0') && (c <= '9')) {
				temp = temp * 10 + (c - '0');
			}
			else if (c == ',' || c == '\0') {
				if (val_idx < 4) {
					if (temp > 180) temp = 180; // Límite máximo para servo
					valores[val_idx++] = temp;
					temp = 0;
				}
			}
		}

		// Convertir a OCR valores
		OCR1A = SERVO_MIN + (valores[0] * (SERVO_MAX - SERVO_MIN)) / 180;
		OCR1B = SERVO_MIN_2 + (valores[1] * (SERVO_MAX_2 - SERVO_MIN_2)) / 180;
		OCR0A = SERVO_MIN_3 + (valores[2] * (SERVO_MAX_3 - SERVO_MIN_3)) / 180;
		OCR0B = SERVO_MIN_4 + (valores[3] * (SERVO_MAX_4 - SERVO_MIN_4)) / 180;

		index = 0; // Reiniciar buffer
	}
	else {
		// Acumular caracteres si hay espacio
		if (index < MAX_BUFFER - 1) {
			buffer[index++] = recibido;
		}
	}
}


ISR(PCINT1_vect) {
	// Verificar estado actual de los botones
	if (!(PINC & (1 << PORTC0))) {  // Si PC0 está presionado (pull-up)
		modo++;
		if (modo >= cant_modo){
			modo = 1;
		}
	}
}
