#line 1 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino"
#include "Arduino.h"
#include "string.h"
#include <Wire.h>
#include "Motores.h"
#include "Comandos.h"

#define TOTAL_FILAS 720

int diametroVarilla = 22;
int alturaVarilla = 15;

const int FIN_DE_CARRERA_X = 3;
const int FIN_DE_CARRERA_Z = 2;
const int MOTOR_DC = 8;


double posX = 0, posY = 0, posZ = 0, posM = 0;


float datos[TOTAL_FILAS];
int indiceActual = 0;
String numeroTemporal = "";
bool cargaCompleta = false;

// Variables para el filtro de seguridad comandos G
int encabezado = -1;
String bufferEncabezado = "";

#line 29 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino"
void mifuncion();
#line 36 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino"
void setup();
#line 56 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino"
void loop();
#line 158 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino"
void procesarNumeroTemporal();
#line 177 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino"
void mostrarDatosLeidos();
#line 200 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino"
void printBanner();
#line 4 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Comandos.ino"
void mostrarDatos();
#line 14 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Comandos.ino"
void comandoG0(String comando);
#line 32 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Comandos.ino"
void comandoG1(String comando);
#line 70 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Comandos.ino"
void comandoG2(String comando);
#line 108 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Comandos.ino"
void comandoS1(String comando);
#line 115 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Comandos.ino"
void comandoG5(String comando);
#line 125 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Comandos.ino"
void comandoHelp();
#line 133 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Comandos.ino"
void inicioPrograma();
#line 3 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Motores.ino"
void girarMotorPap(int motor,bool dir, double distancia, int velocidad);
#line 39 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Motores.ino"
void origenMotor(int motor, int stopPin);
#line 69 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Motores.ino"
void encenderMotor();
#line 73 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Motores.ino"
void apagarMotor();
#line 77 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Motores.ino"
void initMotores();
#line 29 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino"
void mifuncion()
{
  enciendesalid1();
  delay
  apagasalida1();
}

void setup() {

  Serial.begin(115200);
  while (!Serial);
Profile picture
NEW SKETCH

  printBanner();

  initMotores();

  pinMode(FIN_DE_CARRERA_X, INPUT_PULLUP);
  pinMode(FIN_DE_CARRERA_Z, INPUT_PULLUP);
  
  mifuncion();



}

void loop() {
  while (Serial.available() > 0 && !cargaCompleta) {
    char c = Serial.read();

    // ESTADO 1: Validar si llega el comando "G99" al principio
    if (encabezado == -1) {
      // Ignorar saltos de línea iniciales o espacios antes del comando
      if (c == '\n' || c == '\r' || c == ' ') continue; 
      bufferEncabezado += c;
      // Si el búfer acumulado es igual a "G99", desbloqueamos la carga
      if (bufferEncabezado == "G99") {
        encabezado = 99;
        Serial.println("[OK] Comando G99 detectado. Iniciando recepcion de datos...");
        numeroTemporal = ""; // Limpiamos por seguridad
      }
      else if (bufferEncabezado == "G0"){
        encabezado = 0;
        numeroTemporal = ""; // Limpiamos por seguridad
      }
      else if (bufferEncabezado == "G1"){
        encabezado = 1;
        numeroTemporal = ""; // Limpiamos por seguridad
      }
      else if (bufferEncabezado == "G2"){
        encabezado = 2;
        numeroTemporal = ""; // Limpiamos por seguridad
      }
      else if (bufferEncabezado == "G5"){
        encabezado = 5;
        numeroTemporal = ""; // Limpiamos por seguridad
      }
      else if (bufferEncabezado == "S1"){
        encabezado = 3;
        numeroTemporal = ""; // Limpiamos por seguridad
      }
      else if (bufferEncabezado == "INICIO"){
        encabezado = 10;
        numeroTemporal = ""; // Limpiamos por seguridad
      }
      // Si el búfer se llena con texto incorrecto (ej. "G98" o basura), se reinicia el filtro
      else if (bufferEncabezado.length() >= 3) {
        Serial.print("[ERROR] Comando no reconocido: ");
        Serial.println(bufferEncabezado);
        bufferEncabezado = ""; // Resetear y seguir esperando el correcto
      }
      continue; // Saltamos al siguiente carácter del ciclo while
    }

    // ESTADO 2: Carga de datos activa (Solo ocurre si encabezado es true)
    
    // 1. Detectar el final de la transmisión
    if (c == '$') {
      procesarNumeroTemporal();
      cargaCompleta = true;
      mostrarDatosLeidos();
      break;
    }

    if (c == '\n'){
      cargaCompleta = true;
      switch(encabezado){
        case 0:

          comandoG0(numeroTemporal);
        break;
        case 1:
          comandoG1(numeroTemporal);
        break; 
        case 2:
          comandoG2(numeroTemporal);
        break; 
        case 3:
          comandoS1(numeroTemporal);
        break;
        case 5:
          comandoG5(numeroTemporal);
        break;
        case 10:
          inicioPrograma();
        break;
      }
      indiceActual = 0;
      cargaCompleta = false;
      encabezado = -1;
      bufferEncabezado = "";
    }

    // 2. Si es un separador (salto de línea, retorno o espacio), procesamos el número acumulado
    if (c == '\r' || c == ' ') {
      procesarNumeroTemporal();
    } 
    // 3. Si es un carácter válido para un número float (dígitos, punto o signo menos)
    else if (isDigit(c) || c == '.' || c == '-' || c == 'X' || c == 'Y' || c == 'Z' || c == 'O' || c == 'N' || c == 'I' || c == 'C' || c == 'F') {
      numeroTemporal += c;
    }
    // 4. Si entra texto inesperado durante la carga, limpiamos el temporal para no corromper la cifra
    else {
      numeroTemporal = ""; 
    }
  }
}

void procesarNumeroTemporal() {
  numeroTemporal.trim();
  if (numeroTemporal.length() > 0) {
    float valor = numeroTemporal.toFloat();
    
    datos[indiceActual] = valor;
    indiceActual++;
    
    numeroTemporal = "";

    // Protección de desbordamiento
    if (indiceActual >= TOTAL_FILAS) {
      cargaCompleta = true;
      Serial.println("\n[AVISO] Memoria llena. Se alcanzaron las 360 filas.");
      mostrarDatosLeidos();
    }
  }
}

void mostrarDatosLeidos() {
  Serial.println("\n--- DATOS CARGADOS EN MEMORIA CON ÉXITO ---");
  Serial.print("Total de filas reales guardadas: ");
  Serial.println(indiceActual);
  
  for (int i = 0; i < indiceActual; i++) {
    if (i < 5 || i >= indiceActual - 5) {
      Serial.print("Fila [");
      Serial.print(i);
      Serial.print("]: ");
      Serial.println(datos[i], 4);
    } else if (i == 5) {
      Serial.println("... [Datos intermedios ocultos para no saturar la pantalla] ...");
    }
  }
  
  indiceActual = 0;
  cargaCompleta = false;
  encabezado = -1;
  bufferEncabezado = "";
  
}

void printBanner() {
  Serial.println(F("                                                                                         "));
  Serial.println(F("  ███╗   ███╗██╗██████╗ ██╗     ███████╗██████╗  █████╗                                   "));
  Serial.println(F("  ████╗ ████║██║██╔══██╗██║     ██╔════╝██╔══██╗██╔══██╗                                  "));
  Serial.println(F("  ██╔████╔██║██║██████╔╝██║     █████╗  ██████╔╝███████║                                  "));
  Serial.println(F("  ██║╚██╔╝██║██║██╔═══╝ ██║     ██╔══╝  ██╔══██╗██╔══██║                                  "));
  Serial.println(F("  ██║ ╚═╝ ██║██║██║     ███████╗███████╗██║  ██║██║  ██║                                  "));
  Serial.println(F("  ╚═╝     ╚═╝╚═╝╚═╝     ╚══════╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝                                  "));
  Serial.println(F("                                                                                         "));
}


#line 1 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Comandos.ino"
#include "Comandos.h"
#define ALTURA_FUNCIONAMIENTO (alturaVarilla + diametroVarilla/2 - datos[i])

void mostrarDatos(){
  Serial.print("  X:");
  Serial.print(posX);
  Serial.print("  Y:");
  Serial.print(posY);
  Serial.print("  Z:");
  Serial.print(posZ);
  Serial.print("\n");
}

void comandoG0(String comando){
      if(comando[0] == 'X'){
        origenMotor(X,FIN_DE_CARRERA_X);
        posX = 0;
      }

      else if(comando[0] == 'Z'){
        origenMotor(Z,FIN_DE_CARRERA_Z);
        posZ = 0;
      }

      else{
        origenMotor(X,FIN_DE_CARRERA_X);
        origenMotor(Z,FIN_DE_CARRERA_Z);
      }
  mostrarDatos();
}

void comandoG1(String comando){
    double distancia = comando.substring(1).toDouble();
    int motor = 0, velocidad = VELOCIDAD_TEST;
    bool direccion = false;
    if(comando[0] == 'X')
    {
      if(MAXIMO_X <= posX + distancia)
        distancia = MAXIMO_X - posX;
      posX = posX + distancia;
      motor = X;
      direccion = !(HOME_X);
    }
    else if(comando[0] == 'Y')
    {
      posY = posY + distancia;
      distancia = distancia/360;
      motor = Y;
    }
    else if(comando[0] == 'M')
    {
      distancia = distancia/360;
      posM = posM + distancia;
      motor = M;
    }
    else if(comando[0] == 'Z')
    {
      if(MAXIMO_Z <= posZ + distancia)
        distancia = MAXIMO_Z - posZ;
      posZ = posZ + distancia;
      motor = Z;
      direccion = !(HOME_Z);
      velocidad = 10;
    }
  //Serial.print(distancia,7);
  girarMotorPap(motor,direccion,distancia,velocidad);
  mostrarDatos();
}

void comandoG2(String comando){
    double distancia = comando.substring(1).toDouble();
    int motor = 0, velocidad = VELOCIDAD_TEST;
    bool direccion = true;
    if(comando[0] == 'X')
    {
      if(0 >= posX - distancia)
        distancia = posX;
      posX = posX - distancia;
      motor = X;
      direccion = (HOME_X);
    }
    else if(comando[0] == 'Y')
    {
      posY = posY - distancia;
      distancia = distancia/360;
      motor = Y;
    }
    else if(comando[0] == 'M')
    {
      posM = posM - distancia;
      distancia = distancia/360;
      motor = M;
    }
    else if(comando[0] == 'Z')
    {
      if(0 >= posZ - distancia)
        distancia = posZ;
      posZ = posZ - distancia;
      motor = Z;
      velocidad = 15;
      direccion = (HOME_Z);
    }
  girarMotorPap(motor,direccion,distancia,velocidad);
  mostrarDatos();
  
}

void comandoS1(String comando){
  if(comando == "ON")
    encenderMotor();
  else if(comando == "OFF")
    apagarMotor();
}

void comandoG5(String comando)
{
  alturaVarilla = posZ;
  diametroVarilla = comando.toInt();
  Serial.print("La altura seleccionada es: ");
  Serial.print(alturaVarilla);
  Serial.println("mm");
  Serial.println("diametro de la varilla es: " + comando + "mm");
}

void comandoHelp(){
  Serial.print("\n\nLos comandos habilitados son los siguientes:\n");
  Serial.print("- G0 (HOME)\n");
  Serial.print("- G1 (MOV POSITIVO)\n");
  Serial.print("- G2 (MOV NEGATIVO)\n");
  Serial.print("- S1 (CONTROL DE LA SIERRA)\n");
}

void inicioPrograma(){
  int altura = 0;
  bool sentido = !(HOME_X);
  Serial.print("Programa iniciado");
  posY = 0;
  posM = 0;
  /* INICIO DE CORTE*/
      origenMotor(X,FIN_DE_CARRERA_X);
      origenMotor(Z,FIN_DE_CARRERA_Z);
      mostrarDatos();
      encenderMotor();
      for(int i=0;i<TOTAL_FILAS;i++){
        // calcular movimiento Z (altura)
        //altura = promedioDistancias(CANT_MUESTRAS) * 4;
        girarMotorPap(Z,!(HOME_Z),/*altura*/ALTURA_FUNCIONAMIENTO,VELOCIDAD_FUNC_Z);
        posY = ALTURA_FUNCIONAMIENTO;
        mostrarDatos();
        girarMotorPap(X,sentido,/*MAXIMO_X*/200,VELOCIDAD_FUNC_X);
        posX = 200*(int)!sentido;
        sentido = !sentido;
        girarMotorPap(Z,(HOME_Z),/*altura*/ALTURA_FUNCIONAMIENTO,VELOCIDAD_FUNC_Z);
        posZ = 0;
        girarMotorPap(Y,true,((double)1/TOTAL_FILAS),VELOCIDAD_FUNC_Y);
        posY = posY + 1;
        //girarMotorPap(M,true,((double)1/TOTAL_FILAS),VELOCIDAD_FUNC_Y);
        //posM = posM + 1;
      }
      apagarMotor();
      origenMotor(X,FIN_DE_CARRERA_X);
}
#line 1 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Motores.ino"
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
    velocidad = VELOCIDAD_FUNC_Z;
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
