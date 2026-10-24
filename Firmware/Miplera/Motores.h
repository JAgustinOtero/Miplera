#ifndef MOTORES_H 
#define MOTORES_H

#define CANTIDAD_DE_EJES 3
enum Ejes{X,Y,Z};

// --- CONFIGURACIÓN DE PINES RAMPS 1.6 ---
#define STEP_PIN_X   26
#define DIR_PIN_X    28
#define ENABLE_PIN_X  24
#define MICROSTEPS_X 400

#define STEP_PIN_Y   60
#define DIR_PIN_Y    61
#define ENABLE_PIN_Y  56
#define MICROSTEPS_Y 3200*4

#define STEP_PIN_Z   54
#define DIR_PIN_Z    55
#define ENABLE_PIN_Z  38
#define MICROSTEPS_Z 100

#define HOME_X true
#define HOME_Z false

#define MAXIMO_X 400
#define MAXIMO_Z 30

#define VELOCIDAD_TEST 100
#define VELOCIDAD_FUNC_Z 4
#define VELOCIDAD_FUNC_X 50
#define VELOCIDAD_FUNC_Y 5

// --- PROTOTIPOS DE FUNCIONES ---
void initMotores();
void girarMotorPap(int , double);
void origenMotor(int);
void encenderMotor();
void apagarMotor();

#endif