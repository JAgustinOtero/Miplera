#include "Arduino.h"
#include "string.h"
#include <Wire.h>
#include "Motores.h"
#include "Comandos.h"
#include "Display.h"

#define TOTAL_FILAS 720

const int dirPin[CANTIDAD_DE_EJES] = {DIR_PIN_X,DIR_PIN_Y,DIR_PIN_Z};
const int stepPin[CANTIDAD_DE_EJES] = {STEP_PIN_X,STEP_PIN_Y,STEP_PIN_Z};
const int velocidadFunc[CANTIDAD_DE_EJES] = {VELOCIDAD_FUNC_X,VELOCIDAD_FUNC_Y,VELOCIDAD_FUNC_Z};
const int microsteps[CANTIDAD_DE_EJES] = {MICROSTEPS_X,MICROSTEPS_Y,MICROSTEPS_Z};
const int enablePin[CANTIDAD_DE_EJES] = {ENABLE_PIN_X,ENABLE_PIN_Y,ENABLE_PIN_Z};
const bool home[CANTIDAD_DE_EJES] = {HOME_X,true,HOME_Z};
const int max[CANTIDAD_DE_EJES] = {MAXIMO_X,30000,MAXIMO_Z};
double pos[CANTIDAD_DE_EJES] = {0};

int cantidadDatos = 0;
int datoActual = 0;
float datos[TOTAL_FILAS] = {0};
int indiceActual = 0;

int diametroVarilla = 22;
int alturaVarilla = 15;
int largoVarilla = 40;

const int FIN_DE_CARRERA[CANTIDAD_DE_EJES] = {3,0,2};
const int MOTOR_DC = 8;



void setup() {
  initDisplay();

  Serial.begin(115200);
  while (!Serial);

  printBanner();

  initMotores();

  pinMode(FIN_DE_CARRERA[X], INPUT_PULLUP);
  pinMode(FIN_DE_CARRERA[Z], INPUT_PULLUP);
  
}

void loop() {

  //manejo del display
  procesarHardware();
  u8g2.clearBuffer();
  u8g2.setDrawColor(1); 
  estadosDisplay();

  // lectura del puerto serie
  lecturaPuertoSerie();
}

