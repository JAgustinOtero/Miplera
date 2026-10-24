#include "Motores.h"
#define MOTOR motor

void initMotores(){
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

  /*MOTOR Z*/
  pinMode(STEP_PIN_Z, OUTPUT);
  pinMode(DIR_PIN_Z, OUTPUT);
  pinMode(ENABLE_PIN_Z, OUTPUT);
  digitalWrite(ENABLE_PIN_Z, LOW); // Habilita el A4988
  digitalWrite(DIR_PIN_Z, HIGH);

  /*MOTOR DC*/
  pinMode(MOTOR_DC, INPUT_PULLUP);
  digitalWrite(MOTOR_DC, LOW);
}

void encenderMotor(){
  digitalWrite(MOTOR_DC, HIGH);
}

void apagarMotor(){
  digitalWrite(MOTOR_DC, LOW);
}

void girarMotorPap(int motor, double distancia){
  bool direccion = !home[motor];
  if(max[MOTOR] < pos[MOTOR] + distancia)
    distancia = max[MOTOR] - pos[motor];
  else if(0 > pos[MOTOR] + distancia)
    distancia = pos[MOTOR];
  pos[MOTOR] = pos[MOTOR] + distancia;
  if(motor == Y) distancia = distancia/360;
  if(distancia < 0){
    distancia = distancia * (-1);
    direccion = home[motor];
  }
  digitalWrite(dirPin[MOTOR], direccion);
  for (double i = 0; i < (microsteps[MOTOR]) * distancia; i++) {
    digitalWrite(stepPin[MOTOR], HIGH);
    delayMicroseconds(1000000/80/velocidadFunc[MOTOR]);
    digitalWrite(stepPin[MOTOR], LOW);
    delayMicroseconds(1000000/80/velocidadFunc[MOTOR]);
  }
  mostrarDatos();
}

void origenMotor(int motor){
  while(digitalRead(FIN_DE_CARRERA[MOTOR])) girarMotorPap(motor, -1);
  girarMotorPap(motor, 5);
  while(digitalRead(FIN_DE_CARRERA[MOTOR])) girarMotorPap(motor, -1);
  pos[MOTOR] = 0;
}



