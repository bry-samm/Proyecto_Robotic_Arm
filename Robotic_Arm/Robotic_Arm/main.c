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

uint8_t filtro_angulos[4] = {0}; // Un filtro por cada canal ADC (3-6)
//************************************************************************************
// Function prototypes
void setup();

void writeEPROM(uint8_t dato, uint8_t direccion);
uint8_t readEPROM(uint8_t direccion);
void enviar_char_UART(char c);
void enviar_num_UART(uint8_t num);

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


void enviar_num_UART(uint8_t num) {
	// Caso es pecial con el número 0 ya que no entra en la lógica 
	if (num == 0) {
		while (!(UCSR0A & (1 << UDRE0))); // Espera a que el registro de datos este vacío
		UDR0 = '0';
		return;
	}

	char digits[3]; // Hasta 3 dígitos para 0-180
	uint8_t i = 0;
	//Convierte de número a ascii para poder envíarlo correctamente 
	while (num > 0) {
		digits[i++] = (num % 10) + '0'; // % obtiene el último dígito del valor para poder ir guardando en el arreglo díjito 
		num /= 10;
	}

	for (int8_t j = i - 1; j >= 0; j--) { // Recorre los dígitos en orden inverso para enviarlos correctamente.
		while (!(UCSR0A & (1 << UDRE0)));
		UDR0 = digits[j];
	}
}


void enviar_char_UART(char c) {
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = c; // Escribir un caracter para separar ángulos
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
	uint8_t idx = multiplexar_ADC - 3; // índice 0 a 3
	
	// Canal ADC4 invertir valor debido a la conexión física
	if (multiplexar_ADC == 4) {
		ADC_value = 255 - ADC_value;
	}
	
	// Escalar ángulo según el canal
	if (multiplexar_ADC == 3 || multiplexar_ADC == 4 || multiplexar_ADC == 5) {
		if (ADC_value <= 127) {
			angle = (ADC_value * 180UL) / 127; // UL = unsigned long, se coloca ya que puede desbordarse al momento de realizar la operación, entero sin signo de 32 bits
			} else {
			angle = 180;
		}
		} else if (multiplexar_ADC == 6) {
		angle = (ADC_value * 180UL) / 255;
	}
	
	//Filtro la señal
	//Forma original
	//filtro_angulo = alfa * angle + (1-alfa) * filtro_angulo // FIltro de media exponencial
	// Para no usar flotantes ya que alfa debe de ser un número cercano a 0 pero diferente de 1 entonces uso porcentajes
	// alfa = 0.1 ? usamos 10 y 90 como porcentaje (factor 100)
	filtro_angulos[idx] = (angle * 10 + filtro_angulos[idx] * 90) / 100;

	// Calcular pulso PWM
	uint16_t servo_pulse;

	switch (multiplexar_ADC) {
		case 3: // Servo 1 (base)
		servo_pulse = SERVO_MIN + (filtro_angulos[idx] * (SERVO_MAX - SERVO_MIN)) / 180;
		OCR1B = servo_pulse;
		ADMUX = (ADMUX & 0xF0) | 4;
		break;

		case 4: // Servo 2 (antebrazo)
		servo_pulse = SERVO_MIN_2 + (filtro_angulos[idx] * (SERVO_MAX_2 - SERVO_MIN_2)) / 180;
		OCR1A = servo_pulse;
		ADMUX = (ADMUX & 0xF0) | 5;
		break;

		case 5: // Servo 3 (brazo)
		servo_pulse = SERVO_MIN_3 + (filtro_angulos[idx] * (SERVO_MAX_3 - SERVO_MIN_3)) / 180;
		OCR0A = servo_pulse;
		ADMUX = (ADMUX & 0xF0) | 6;
		break;

		case 6: // Servo 4 (garra)
		servo_pulse = SERVO_MIN_4 + (filtro_angulos[idx] * (SERVO_MAX_4 - SERVO_MIN_4)) / 180;
		OCR0B = servo_pulse;
		ADMUX = (ADMUX & 0xF0) | 3;
		break;
	}

	ADCSRA |= (1 << ADSC); // Iniciar nueva conversión
}

#define MAX_BUFFER 20
volatile char buffer[MAX_BUFFER];
volatile uint8_t index = 0; // Ïndice del bufer para ir guardando los caracteres recibidos 

ISR(USART_RX_vect) {
	if (modo != 3) return; // Si no está en modo 3 ignora los valores recibidos y sale de la ISR

	char recibido = UDR0;

	if (recibido == '\n' || recibido == '\r') { // Si se recibe saldo de línea o retorno inidca que se finalizó la transmisión
		buffer[index] = '\0'; // Terminación de la línea
		
		// En esta parte se convierte el valor ascci (texto) a un número
		uint8_t valores[4] = {0}; //Arreglo para guardar los 4 caracteres de los ángulos (180,)
		uint8_t val_idx = 0;
		uint16_t temp = 0; // Se usa para acumular dígitos

		for (uint8_t i = 0; i <= index; i++) { // recorre cada caractér del buffer 
			char c = buffer[i]; // Agarra el valor actual y pasa a procesarlo 

			if (c >= '0' && c <= '9') { // Si el caractér está entre 0 y 9
				temp = temp * 10 + (c - '0'); // Va barriendo los dígitos y multiplicarlos por 10 para hacer unidad, decena y centena
			}
			else if (c == ',' || c == '\0') { // Si hay coma o \0 termina el dígito y lo guarda en el arreglo valores 
				if (val_idx < 4) {
					if (temp > 180) temp = 180; // Limita el ángulo a 180 por que ese valor trabaja el servo
					valores[val_idx++] = temp; 
					temp = 0; // Reinicia para construir nuevo número 
				}
			}
		}

		// Aplicar valores a servos, la lógica es igual que en el ADC_vect
		//PWM = SERVO_MIN + (valor * ancho_pwm) / 180
		OCR1B = SERVO_MIN + (valores[0] * (SERVO_MAX - SERVO_MIN)) / 180;
		OCR1A = SERVO_MIN_2 + (valores[1] * (SERVO_MAX_2 - SERVO_MIN_2)) / 180;
		OCR0A = SERVO_MIN_3 + (valores[2] * (SERVO_MAX_3 - SERVO_MIN_3)) / 180;
		OCR0B = SERVO_MIN_4 + (valores[3] * (SERVO_MAX_4 - SERVO_MIN_4)) / 180;

		// Enviar feedback por UART
		enviar_num_UART(valores[0]);
		enviar_char_UART(',');
		enviar_num_UART(valores[1]);
		enviar_char_UART(',');
		enviar_num_UART(valores[2]);
		enviar_char_UART(',');
		enviar_num_UART(valores[3]);
		enviar_char_UART('\n');

		index = 0;
	}
	else {
		buffer[index++] = recibido; //Si no se recibió un \n o \r guarda el carácter en el buffer y aumenta el índice.
	}
}


ISR(PCINT1_vect) {
	// Verificar estado actual de los botones
	if (!(PINC & (1 << PORTC0))) {  // Si PC0 está presionado (pull-up)
		modo++;
		if (modo >= cant_modo){ // Reiniciar a 1 el modo si se llegó al últmio modo
			modo = 1;
		}
	} else if (!(PINC & (1 << PORTC1))){
		action_button = 1; // Activar bandera de acción
	}
}


