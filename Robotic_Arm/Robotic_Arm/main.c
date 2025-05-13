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
#include <util/delay.h>
#include "CONF_PWM/CONF_PWM.h"
#include "CONF_ADC/CONF_ADC.h"
#include "CONF_UART/CONF_UART.h"


uint8_t	multiplexar_ADC = 0;	// Indica que ADC se está usando 
uint8_t	ADC_value;				// Guardar valor del ADC
uint8_t	modo = 1;				// Modo actual
uint8_t	cant_modo = 4;			// Cantidad de modos +1
uint8_t action_button;			// Bandera cuando se presiona el botón de ejecutar
uint8_t entrada = 1;			// Saber si se inició por primera 
uint8_t save_temporal = 0x00;	// Direcciones de la eprom

uint8_t addr = 0;				// Direcciones a leer
uint8_t pasos = 0;				// Pasos ejecutados

# define NUM_MAX_PASOS 5		// Cantidad máxima de posiciones
uint8_t cant_posiciones;		// Cantidad de posiciones guardadas
uint8_t modo_repro_iniciado = 0;// Conocer si finalizo de reproducir
//************************************************************************************
// Function prototypes
void setup();

void writeEPROM(uint8_t dato, uint8_t direccion);
uint8_t readEPROM(uint8_t direccion);

void manual();
void reproducir_data();
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
			reproducir_data();
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


void writeEPROM(uint8_t dato, uint8_t direccion){
	// Esperar a que termine la escritura anterior
	while (EECR & (1 << EEPE));
	// Asignar dirección de escritura
	EEAR = direccion;
	// Asignar dato a "escribir"
	EEDR = dato;
	// Setear en 1 el "master write enable"
	EECR |= (1 << EEMPE);
	// Empezar a escribir
	EECR |= (1 << EEPE);
}

uint8_t readEPROM(uint8_t direccion){
	// Esperar a que termine la escritura anterior
	while (EECR & (1 << EEPE));
	// Asignar dirección de escritura
	EEAR = direccion;
	// Empezar a leer
	EECR |= (1 << EERE);
	return EEDR;
}

void manual(){
	//Mostrar el modo actual 
	PORTB |= (1 << PORTB3);
	PORTB &= ~(1 << PORTB4);
	
	ADCSRA |= (1 << ADSC);
	
	//Esto se ejecuta cuando entra por primera vez al modo manual para reiniciar la dirección
	if (entrada == 0){
		save_temporal = 0x00; // Reinicia la dirección
	}
	if (entrada == 0 && action_button == 1){ // Ejecutar solo si se entró por primera vez y se activó el botón
		cant_posiciones = 0; //Reinicia variable para saber cuantas posiciones lleva guardadas
	}

	if (action_button == 1 && cant_posiciones < NUM_MAX_PASOS) { // Si se activa el botón y las posiciones son menores a las indicadas, seguir guardando
		// Parpadeo de leds
		PORTB &= ~(1 << PORTB3);
		_delay_ms(3);
		PORTB |= (1 << PORTB3);
		
		//Guardar en la EPROM el valor del OCR, aumenta la dirección para guardar la siguiente posición
		writeEPROM(OCR1A, save_temporal++);
		writeEPROM(OCR1B, save_temporal++);
		writeEPROM(OCR0A, save_temporal++);
		writeEPROM(OCR0B, save_temporal++);
		entrada = 1; //Se coloca en 1 para que no reinicie a la dirección 0
		action_button = 0; // Se apaga el botón de acción
		cant_posiciones++; // Aumento posiciones
	}
}


void reproducir_data() {
	//Mostrar en los leds el modo actual
	PORTB |= (1 << PORTB4);
	PORTB &= ~(1 << PORTB3);
	entrada = 0;

	
	if (!modo_repro_iniciado && action_button == 1) {
		//Reinicio valores
		addr = 0;
		pasos = 0; 
		modo_repro_iniciado = 1;
		action_button = 0;
	}

	// Ejecutar solo si se inició el modo de reproducción
	if (modo_repro_iniciado == 1) {
		if (pasos < cant_posiciones) { //Compara para reproducir unicamente la cantidad de posiciones guardadas
			OCR1A = readEPROM(addr++);
			OCR1B = readEPROM(addr++);
			OCR0A = readEPROM(addr++);
			OCR0B = readEPROM(addr++);
			
			pasos++;
			_delay_ms(100); // Tiempo entre posiciones
			} else {
			modo_repro_iniciado = 0; // Se completó la reproducción
		}
	}
}



void escribir_angulo(){
	PORTB |= (1 << PORTB3) | (1 << PORTB4); // Ambos LEDs
	//Reinicio variables de otros modos (redundante)
	pasos= 0;
	entrada = 0;
	save_temporal = 0;
	addr = 0;
	modo_repro_iniciado = 0;
	
}

//************************************************************************************
// Interrupt subroutines

ISR(ADC_vect) {
	if (modo != 1) return;

	multiplexar_ADC = ADMUX & 0x0F;
	ADC_value = ADCH; // Valor ADC de 8 bits
	uint8_t angle;
	
	// Canal ADC4 invertir valor debido a la conexión física
	if (multiplexar_ADC == 4) {
		ADC_value = 255 - ADC_value;
	}

	// Escalar el rango útil del potenciómetro (0-127 la mitad) -> 0-180° del servo
	if (ADC_value <= 127) {
		angle = (ADC_value * 180UL) / 127;
		} else {
		angle = 180;
	}

	// Calcular el pulso PWM para el servo
	uint16_t servo_pulse;

	switch(multiplexar_ADC) {
		case 3: // Canal ADC3 -> Servo 1
		servo_pulse = SERVO_MIN + (angle * (SERVO_MAX - SERVO_MIN)) / 180;
		OCR1B = (uint16_t)servo_pulse;
		ADMUX = (ADMUX & 0xF0) | 4; // Pasar al sigueinte ADC
		break;

		case 4: // Canal ADC4 -> Servo 2 (inversión aplicada arriba)
		servo_pulse = SERVO_MIN_2 + (angle * (SERVO_MAX_2 - SERVO_MIN_2)) / 180;
		OCR1A = (uint16_t)servo_pulse;
		ADMUX = (ADMUX & 0xF0) | 5;
		break;

		case 5: // Canal ADC5 -> Servo 3
		servo_pulse = SERVO_MIN_3 + (angle * (SERVO_MAX_3 - SERVO_MIN_3)) / 180;
		OCR0A = (uint16_t)servo_pulse;
		ADMUX = (ADMUX & 0xF0) | 6;
		break;

		case 6: // Canal ADC6 -> Servo 4
		servo_pulse = SERVO_MIN_4 + (angle * (SERVO_MAX_4 - SERVO_MIN_4)) / 180;
		OCR0B = (uint16_t)servo_pulse;
		ADMUX = (ADMUX & 0xF0) | 3; // Regresar al primer ADC (ADC3)
		break;
	}

	ADCSRA |= (1 << ADSC); // Iniciar nueva conversión
}



#define MAX_BUFFER 20
volatile char buffer[MAX_BUFFER]; // El buffer se encarga de contener los valores recibidos
volatile uint8_t index = 0; // Tiene el número de caracteres almacenados 

ISR(USART_RX_vect) {
	
	if (modo != 3) return; // Solo ejecutar 
	
	char recibido = UDR0;

	// Cuando llega fin de línea (enter)
	if (recibido == '\n' || recibido == '\r') {
		
		buffer[index] = '\0';  // Terminar cadena cuando se manda un "enter" el cual es el valor nulo 

		// Variables temporales
		uint8_t valores[4] = {0};
		uint8_t val_idx = 0;
		uint16_t temp = 0;

		for (uint8_t i = 0; i <= index; i++) { // Esto lo que hace es barrer todo el string
			char c = buffer[i]; // Extrae caracter actual y lo guarda en c 

		// Iniciamos convirtiendo los caracteres en números, del 0 al 9 
			if ((c >= '0') && (c <= '9')) { 
				temp = temp * 10 + (c - '0');
			}
		// Manejamos el otro caso, significa que terminó de leer el número 
			else if (c == ',' || c == '\0') {
				
		// Doble verifiación, si la cadena no ha superado los 4 números y si el valor es menor a los 180°
				if (val_idx < 4) {
					if (temp > 180) temp = 180; // Límite máximo para servo
					valores[val_idx++] = temp; // Guarda el número en el nuevo arreglo e incrementa para guardar el siguiente número 
					temp = 0; // Reinicia "temp" para empezar a construir y guardar el nuevo número
				}
			}
		}

		// Convertir a OCR valores (mismo en en el MUX)
		OCR1B = SERVO_MIN + (valores[0] * (SERVO_MAX - SERVO_MIN)) / 180;
		OCR1A = SERVO_MIN_2 + (valores[1] * (SERVO_MAX_2 - SERVO_MIN_2)) / 180;
		OCR0A = SERVO_MIN_3 + (valores[2] * (SERVO_MAX_3 - SERVO_MIN_3)) / 180;
		OCR0B = SERVO_MIN_4 + (valores[3] * (SERVO_MAX_4 - SERVO_MIN_4)) / 180;

		index = 0; // Reiniciar buffer
	}
	else { 
		// Esta parte es como un recolector de catacteres 
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
	} else if (!(PINC & (1 << PORTC1))){
		action_button = 1;
	}
}


