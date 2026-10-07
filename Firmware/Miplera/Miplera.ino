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

