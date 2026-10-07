#define STEP_PIN_X   26
#define DIR_PIN_X    28
#define ENABLE_PIN_X  24
#define MICROSTEPS_X 400

#define STEP_PIN_Y   60
#define DIR_PIN_Y    61
#define ENABLE_PIN_Y  56
#define MICROSTEPS_Y 3200*4

#define STEP_PIN_M   46
#define DIR_PIN_M    48
#define ENABLE_PIN_M  62
#define MICROSTEPS_M 3200

#define STEP_PIN_Z   54
#define DIR_PIN_Z    55
#define ENABLE_PIN_Z  38
#define MICROSTEPS_Z 100

#define HOME_X true
#define HOME_Z false

#define MAXIMO_X 400
#define MAXIMO_Z 30

#define X 0
#define Y 1
#define Z 2
#define M 3

#define VELOCIDAD_TEST 100
#define VELOCIDAD_FUNC_Z 4
#define VELOCIDAD_FUNC_X 100
#define VELOCIDAD_FUNC_Y 5
#define VELOCIDAD_FUNC_M 5

void initMotores();

void girarMotorPap(int ,bool , double , int );

void origenMotor(int , int );

void encenderMotor();

void apagarMotor();
