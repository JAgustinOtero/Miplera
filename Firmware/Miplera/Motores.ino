#include "Motores.h"

void girarMotorPap(int motor,bool dir, double distancia, int velocidad){
  long stepPin = 0, dirPin = 0, microsteps = 0;
  if(motor == X)
  {
    stepPin = STEP_PIN_X;
    dirPin = DIR_PIN_X;
    microsteps = MICROSTEPS_X;
    
  }
  else if(motor == Y)
  {
    stepPin = STEP_PIN_Y;
    dirPin = DIR_PIN_Y;
    microsteps = MICROSTEPS_Y;
  }
  else if(motor == M)
  {
    stepPin = STEP_PIN_M;
    dirPin = DIR_PIN_M;
    microsteps = MICROSTEPS_M;
  }
  else if(motor == Z)
  {
    stepPin = STEP_PIN_Z;
    dirPin = DIR_PIN_Z;
    microsteps = MICROSTEPS_Z;
  }
  digitalWrite(dirPin, dir);
  for (double i = 0; i < (microsteps) * distancia; i++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(1000000/80/velocidad);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(1000000/80/velocidad);
  }
}

void origenMotor(int motor, int stopPin){
  int distanciaInicio = 0;
  int dir = 0, velocidad = 0;
  
  if (motor == X){
    distanciaInicio = posX;
    dir = HOME_X;
    velocidad = VELOCIDAD_FUNC_X + 20;
    posX = 0;
  }
  else if (motor == Z){
    distanciaInicio = posZ;
    dir = HOME_Z;
    velocidad = VELOCIDAD_FUNC_Z + 10;
    posZ = 0;
  }

  girarMotorPap(motor,dir,distanciaInicio - 5,120);

  while(digitalRead(stopPin)){
    girarMotorPap(motor,dir, 1,velocidad);
  }
  
  girarMotorPap(motor,!dir, 5,velocidad);
  
  while(digitalRead(stopPin)){
    girarMotorPap(motor,dir, 1,velocidad);
  }
}

void encenderMotor(){
  digitalWrite(MOTOR_DC, HIGH);
}

void apagarMotor(){
  digitalWrite(MOTOR_DC, LOW);
}

void initMotores(){
  pinMode(MOTOR_DC, INPUT_PULLUP);
  /*MOTOR X*/

  pinMode(STEP_PIN_X, OUTPUT);
  pinMode(DIR_PIN_X, OUTPUT);
  pinMode(ENABLE_PIN_X, OUTPUT);
  digitalWrite(ENABLE_PIN_X, LOW); // Habilita el A4988
  digitalWrite(DIR_PIN_X, HIGH);

  /*MOTOR Y*/
  pinMode(STEP_PIN_Y, OUTPUT);
  pinMode(DIR_PIN_Y, OUTPUT);
  pinMode(ENABLE_PIN_Y, OUTPUT);
  digitalWrite(ENABLE_PIN_Y, LOW); // Habilita el A4988
  digitalWrite(DIR_PIN_Y, HIGH);

    /*MOTOR Y2*/
  pinMode(STEP_PIN_M, OUTPUT);
  pinMode(DIR_PIN_M, OUTPUT);
  pinMode(ENABLE_PIN_M, OUTPUT);
  digitalWrite(ENABLE_PIN_M, LOW); // Habilita el A4988
  digitalWrite(DIR_PIN_M, HIGH);

  /*MOTOR Z*/
  pinMode(STEP_PIN_Z, OUTPUT);
  pinMode(DIR_PIN_Z, OUTPUT);
  pinMode(ENABLE_PIN_Z, OUTPUT);
  digitalWrite(ENABLE_PIN_Z, LOW); // Habilita el A4988
  digitalWrite(DIR_PIN_Z, HIGH);

  /*MOTOR DC*/
  digitalWrite(MOTOR_DC, LOW);
  
}