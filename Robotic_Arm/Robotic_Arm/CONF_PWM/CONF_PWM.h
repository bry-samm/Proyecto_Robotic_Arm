/*
 * CONF_PWM.h
 *
 * Created: 8/05/2025 15:24:05
 *  Author: bsmor
 */ 


#ifndef CONF_PWM_H_
#define CONF_PWM_H_

//Valores para primer servo (base)
#define SERVO_MIN  9
#define SERVO_MAX  36

//Valores para segundo servo (ante brazo)
#define SERVO_MIN_2  18
#define SERVO_MAX_2  30

//Valores para tercer servo (brazo)
#define SERVO_MIN_3  12
#define SERVO_MAX_3  34

//Valores para el cuarto servo (garra)
#define SERVO_MIN_4  20
#define SERVO_MAX_4  36

//========================================================= BRYAN ======================================================
/*
//Valores para primer servo (base)
#define SERVO_MIN  9
#define SERVO_MAX  36

//Valores para segundo servo (ante brazo)
#define SERVO_MIN_2  9
#define SERVO_MAX_2  36

//Valores para tercer servo (brazo)
#define SERVO_MIN_3  11
#define SERVO_MAX_3  39

//Valores para el cuarto servo (garra)
#define SERVO_MIN_4  20
#define SERVO_MAX_4  36
*/


void initPWM0();
void initPWM1();


#endif /* CONF_PWM_H_ */