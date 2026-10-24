#line 1 "C:\\Users\\Agustin\\Documents\\Emprendimientos\\Proyectos\\Miplera\\Firmware\\Miplera\\Miplera.ino"
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


#line 1 "C:\\Users\\Agustin\\Documents\\Emprendimientos\\Proyectos\\Miplera\\Firmware\\Miplera\\Comandos.ino"
#include "Comandos.h"
#define MOTOR comando[0]-88

bool sentidoFuncionamiento = 0;
String numeroTemporal = "";
bool cargaCompleta = false;
// Variables para el filtro de seguridad comandos G
int encabezado = -1;
String bufferEncabezado = "";

void mostrarDatos(){
  Serial.print("  X:");
  Serial.print(pos[X]);
  Serial.print("  Y:");
  Serial.print(pos[Y]);
  Serial.print("  Z:");
  Serial.print(pos[Z]);
  Serial.print("\n");
}

void comandoG0(String comando){
      if(comando)
        origenMotor(comando[0]);
      else{
        origenMotor(Z);
        origenMotor(X);
      }
  mostrarDatos();
}

void comandoG1(String comando){
  double distancia = 0;
  if(comando.substring(1,2) == '-')
  {
    distancia = comando.substring(2).toDouble() * (-1);
  }
  else 
    distancia = comando.substring(1).toDouble();
  Serial.println(distancia);
  girarMotorPap(comando[0]-88,distancia);
}
/* old
void comandoG2(String comando){
  double distancia = comando.substring(1).toDouble();
  if(comando[0] == 'Y') distancia = distancia/360;
  girarMotorPap(comando[0],(home[MOTOR]),(-1)*distancia);
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
*/
void comandoS1(String comando){
  if(comando == "ON") encenderMotor();
  else if(comando == "OFF") apagarMotor();
}

void comandoG5(String comando){
  Serial.print(comando);
  alturaVarilla = pos[Z];
  diametroVarilla = comando.substring(0,2).toInt();
  largoVarilla = comando.substring(2).toInt();
  Serial.print("La altura seleccionada es: ");
  Serial.print(alturaVarilla);
  Serial.println("mm");
  Serial.println("largo de la varilla es: " + comando.substring(2) + "cm");
  Serial.println("diametro de la varilla es: " + comando.substring(0,2) + "mm");
}

void comandoHelp(){
  Serial.print("\n\nLos comandos habilitados son los siguientes:\n");
  Serial.print("- G0 (HOME)\n");
  Serial.print("- G1 (MOV POSITIVO)\n");
  Serial.print("- G2 (MOV NEGATIVO)\n");
  Serial.print("- S1 (CONTROL DE LA SIERRA)\n");
}

void inicioPrograma(){
  Serial.print("Programa iniciado");
  pos[Y] = 0;
  /* INICIO DE CORTE*/
  //comandoG0("");
  //mostrarDatos();
  encenderMotor();
  sentidoFuncionamiento = !(home[X]);
}
  
void funcionamiento(){
  girarMotorPap(Z,ALTURA_FUNCIONAMIENTO); //baja eje Z
  pos[Z] = ALTURA_FUNCIONAMIENTO;
  girarMotorPap(X,largoVarilla*10); //corta
  largoVarilla = largoVarilla * (-1);
  girarMotorPap(Z,ALTURA_FUNCIONAMIENTO * (-1));//sube eje Z
  pos[Z] = 0;
  girarMotorPap(Y,((double)1/cantidadDatos)); //gira
  pos[Y] += 360/cantidadDatos;
  datoActual++;
}  

void lecturaPuertoSerie(){
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
      }/*
      else if (bufferEncabezado == "G2"){
        encabezado = 2;
        numeroTemporal = ""; // Limpiamos por seguridad
      }
      */
      else if (bufferEncabezado == "G5"){
        encabezado = 5;
        numeroTemporal = ""; // Limpiamos por seguridad
      }
      else if (bufferEncabezado == "S1"){
        encabezado = 3;
        numeroTemporal = ""; // Limpiamos por seguridad
      }
      else if (bufferEncabezado == "G80"){
        encabezado = 80;
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
          //comandoG2(numeroTemporal);
        break; 
        case 3:
          comandoS1(numeroTemporal);
        break;
        case 5:
          comandoG5(numeroTemporal);
        break;
        case 80:
          estadoActual = ESTADO_FUNCIONAMIENTO;
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
    cantidadDatos = i+1;
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


#line 1 "C:\\Users\\Agustin\\Documents\\Emprendimientos\\Proyectos\\Miplera\\Firmware\\Miplera\\Display.ino"
#include "Display.h"

void initDisplay(){
  pinMode(BTN_EN1, INPUT_PULLUP);
  pinMode(BTN_EN2, INPUT_PULLUP);
  pinMode(BTN_ENC, INPUT_PULLUP);
  pinMode(SD_DETECT_PIN, INPUT_PULLUP); 
  
  ultimoCLK = digitalRead(BTN_EN1);
  ultimoDATA = digitalRead(BTN_EN2);

  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tf); 
}

// --- DEFINICIÓN DE VARIABLES GLOBALES ---
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, LCD_PINS_D4, LCD_PINS_ENABLE, LCD_PINS_RS, U8X8_PIN_NONE);
Estados estadoActual = ESTADO_PANTALLA_PRINCIPAL;

bool sdPresente = false;
bool ultimaSDPresente = false;
String fileNames[MAX_FILES];
int fileCount = 0; 
int menuIndex = 0;           
int scrollOffset = 0;        

int encoderPos = 0;
byte ultimoCLK;
byte ultimoDATA;
unsigned long ultimoTiempoGiro = 0;
bool botonPresionado = false;
unsigned long ultimoTiempoBoton = 0;

char ejeSeleccionado = 'X'; 
extern int largosVarilla[] = {10,20,30,40,50};
float distancias[] = {0.1, 1.0, 10.0, 100.0};
int diametros[] = {20,22,24,26,28,30,32,34,36,38};
int distanciaIndex = 0;
float pasoActual = 1.0;
double posicionSimulada = 0.0;
int ultimoEncoderPosMov = 0;


// --- IMPLEMENTACIÓN DE FUNCIONES ---

void procesarHardware() {
  sdPresente = (digitalRead(SD_DETECT_PIN) == LOW);

  if (sdPresente != ultimaSDPresente) {
    delay(50); 
    if (sdPresente) {
      if (SD.begin(SD_SS_PIN)) buscarArchivos();
    } else {
      fileCount = 0;
      liberarMemoriaArray();
      estadoActual = ESTADO_PANTALLA_PRINCIPAL; 
    }
    ultimaSDPresente = sdPresente;
  }
  evaluarEncoderCruzado();
}

void evaluarEncoderCruzado() {
  byte muestraCLK = digitalRead(BTN_EN1);
  byte muestraDATA = digitalRead(BTN_EN2);
  unsigned long tiempoActual = millis();

  if (muestraCLK != ultimoCLK && muestraCLK == LOW) {
    if (tiempoActual - ultimoTiempoGiro > FILTRO_VELOCIDAD_MS) {
      if (muestraDATA == HIGH) encoderPos++; else encoderPos--;
      ultimoTiempoGiro = tiempoActual;
    }
  }

  if (muestraDATA != ultimoDATA && muestraDATA == LOW) {
    if (tiempoActual - ultimoTiempoGiro > FILTRO_VELOCIDAD_MS) {
      if (muestraCLK == LOW) encoderPos++; else encoderPos--;
      ultimoTiempoGiro = tiempoActual;
    }
  }
  ultimoCLK = muestraCLK;
  ultimoDATA = muestraDATA;
}

bool leerBotonEncoder() {
  bool resultado = false;
  int lectura = digitalRead(BTN_ENC);
  unsigned long tiempoActual = millis();

  if (lectura == LOW && !botonPresionado) {
    if (tiempoActual - ultimoTiempoBoton > 250) { 
      botonPresionado = true;
      resultado = true;
      ultimoTiempoBoton = tiempoActual;
    }
  }
  if (lectura == HIGH) {
    botonPresionado = false;
  }
  return resultado;
}

void buscarArchivos() {
  File root = SD.open("/");
  fileNames[0] = "... \x1b"; // Corregido con el índice [0]
  fileCount = 1; 
  if(!root) return;
  
  while (true) {
    File entry = root.openNextFile();
    if (!entry || fileCount >= MAX_FILES) break;
    
    if (!entry.isDirectory()) {
      String name = entry.name();
      if (name.endsWith(".TXT") || name.endsWith(".txt")) {
        fileNames[fileCount] = name;
        fileCount++;
      }
    }
    entry.close();
  }
  root.close();
}

void liberarMemoriaArray() {
  indiceActual = 0;
}

void leerYParsearArchivo(String nombre) {
  File file = SD.open(nombre);
  if (!file) return;

  liberarMemoriaArray();
  String bufferString = "";
  
  while (file.available()) {
    char c = file.read();
    
    if (c == ',' || !file.available()) {
      if (!file.available() && c != ',') bufferString += c;
      if (bufferString.length() > 0) {
        indiceActual++;
        //datos = (float*) realloc(valoresArray, valoresCount * sizeof(float));
        datos[indiceActual - 1] = bufferString.toFloat();
        bufferString = "";
      }
    } else if (c != '\r' && c != '\n' && c != ' ') {
      bufferString += c;
    }
  }
  file.close();
}

void estadosDisplay(){
  switch (estadoActual) {
    case ESTADO_PANTALLA_PRINCIPAL:
      u8g2.setFont(u8g2_font_t0_40b_me); 
      u8g2.drawStr(0, 42, "MIPLERA");

      if (leerBotonEncoder()) {
        menuIndex = 0;
        encoderPos = 0;
        estadoActual = ESTADO_MENU_OPCIONES;
      }
      u8g2.setFont(u8g2_font_6x10_tf);
      break;

    case ESTADO_MENU_OPCIONES:
      if (encoderPos > 3) encoderPos = 3;
      if (encoderPos < 0) encoderPos = 0;
      menuIndex = encoderPos;

      u8g2.drawStr(6, 11, "Atras \x1b");
      if (sdPresente) u8g2.drawStr(6, 23, "Explorar SD");
      else u8g2.drawStr(6, 23, "INSERTE UNA SD");
      u8g2.drawStr(6, 35, "Movimiento");
      u8g2.drawStr(6, 47, "Configuracion");

      u8g2.setDrawColor(2);         
      u8g2.drawBox(0, 1 + (menuIndex * 12), 128, 12); 
      u8g2.setDrawColor(1);         

      if (leerBotonEncoder()) {
        if (menuIndex == 0) {
          menuIndex = 0; encoderPos = 0;
          estadoActual = ESTADO_PANTALLA_PRINCIPAL;
        } else if (menuIndex == 1) {
          if (sdPresente && fileCount > 1) { 
            menuIndex = 0; scrollOffset = 0; encoderPos = 0;
            estadoActual = ESTADO_EXPLORADOR_SD;
          } else if (!sdPresente) {
            u8g2.clearBuffer(); u8g2.drawStr(20, 35, "No hay tarjeta!"); u8g2.sendBuffer(); delay(1000);
          }
        } else if (menuIndex == 2) {
          menuIndex = 0; encoderPos = 0;
          estadoActual = ESTADO_MENU_MOVIMIENTO;
        } else if (menuIndex == 3) {
          estadoActual = ESTADO_MENU_CONFIGURACION;
          encoderPos = 0;
        }
      }
      break;
    case ESTADO_EXPLORADOR_SD:
      if (encoderPos >= fileCount) encoderPos = fileCount - 1;
      if (encoderPos < 0) encoderPos = 0;
      menuIndex = encoderPos;

      if (menuIndex < scrollOffset) scrollOffset = menuIndex; 
      if (menuIndex >= scrollOffset + MAX_LINEAS_VISIBLES) scrollOffset = menuIndex - MAX_LINEAS_VISIBLES + 1; 

      for (int i = 0; i < MAX_LINEAS_VISIBLES; i++) {
        int fileIdx = scrollOffset + i;
        if (fileIdx >= fileCount) break; 
        u8g2.drawStr(4, 11 + (i * 12), fileNames[fileIdx].c_str());
      }

      for (int i = 0; i < MAX_LINEAS_VISIBLES; i++) {
        if (scrollOffset + i == menuIndex) {
          u8g2.setDrawColor(2);         
          u8g2.drawBox(0, 1 + (i * 12), 128, 12); 
          u8g2.setDrawColor(1);         
          break; 
        }
      }

      if (leerBotonEncoder()) {
        if (menuIndex == 0) {
          menuIndex = 1; encoderPos = 1; 
          estadoActual = ESTADO_MENU_OPCIONES;
        } else {
          u8g2.clearBuffer(); u8g2.drawStr(10, 35, "Cargando datos..."); u8g2.sendBuffer();
          leerYParsearArchivo(fileNames[menuIndex]);
          u8g2.clearBuffer(); u8g2.drawStr(10, 25, "Cargado con Exito!");
          u8g2.setCursor(10, 45); u8g2.print("Valores: "); u8g2.print(indiceActual); u8g2.sendBuffer(); delay(2000);
          mostrarDatosLeidos();
          estadoActual = ESTADO_FUNCIONAMIENTO; 
        }
      }
      break;
    case ESTADO_MENU_CONFIGURACION:
      if (encoderPos > 3) encoderPos = 3;
      if (encoderPos < 0) encoderPos = 0;
      menuIndex = encoderPos;

      u8g2.drawStr(6, 11, "Atras \x1b");
      u8g2.drawStr(6, 23, "Definir Posicion Z");
      u8g2.drawStr(6, 35, "Definir Diametro Varilla");
      u8g2.drawStr(6, 47, "Definir Largo Varilla");

      u8g2.setDrawColor(2);         
      u8g2.drawBox(0, 1 + (menuIndex * 12), 128, 12); 
      u8g2.setDrawColor(1); 

      if (leerBotonEncoder()) {
        if (menuIndex == 0) {
          menuIndex = 0; encoderPos = 0;
          estadoActual = ESTADO_MENU_OPCIONES;
        } else if (menuIndex == 1) {
          u8g2.clearBuffer();
          u8g2.drawStr(15, 25, "Posicion Z");
          u8g2.drawStr(15, 40, "Definida");
          alturaVarilla = pos[Z];
          u8g2.sendBuffer();
          delay(1500);
        } else if (menuIndex == 2) {
          encoderPos = 0;
          estadoActual = ESTADO_DEFINIR_DIAMETRO_VARILLA;
        } else if (menuIndex == 3) {
          encoderPos = 0;
          estadoActual = ESTADO_DEFINIR_LARGO_VARILLA;
        }
      }
      
      break;  
    case ESTADO_DEFINIR_DIAMETRO_VARILLA:
      if (encoderPos > 9) encoderPos = 9;
      if (encoderPos < 0) encoderPos = 0;
      menuIndex = encoderPos;

      if (menuIndex < scrollOffset) scrollOffset = menuIndex; 
      if (menuIndex >= scrollOffset + MAX_LINEAS_VISIBLES_B)
        scrollOffset = menuIndex - MAX_LINEAS_VISIBLES_B+1;

      u8g2.drawStr(6, 11, "DIAMETRO DE LA VARILLA:");

      for (int i = 0; i < MAX_LINEAS_VISIBLES_B; i++) {
        int fileIdx = scrollOffset + i;
        if (fileIdx >= 10) break; 
        u8g2.setCursor(6, 23 + (i * 12));
        u8g2.print(diametros[fileIdx],1); u8g2.print(" mm");
      }

      for (int i = 0; i < MAX_LINEAS_VISIBLES_B; i++) {
        if (scrollOffset + i == menuIndex) {
          u8g2.setDrawColor(2);         
          u8g2.drawBox(0, 13 + (i * 12), 128, 12); 
          u8g2.setDrawColor(1);         
          break; 
        }
      }
      if (leerBotonEncoder()) {
        diametroVarilla = diametros[menuIndex];
        Serial.println(diametroVarilla);
        encoderPos = 0;
        ultimoEncoderPosMov = 0;
        posicionSimulada = 0.0;
        estadoActual = ESTADO_MENU_CONFIGURACION;
      }

      break;
    case ESTADO_DEFINIR_LARGO_VARILLA:
      if (encoderPos > 4) encoderPos = 4;
      if (encoderPos < 0) encoderPos = 0;
      menuIndex = encoderPos;

      if (menuIndex < scrollOffset) scrollOffset = menuIndex; 
      if (menuIndex >= scrollOffset + MAX_LINEAS_VISIBLES_B)
        scrollOffset = menuIndex - MAX_LINEAS_VISIBLES_B+1;

      u8g2.drawStr(6, 11, "LARGO DE LA VARILLA:");

      for (int i = 0; i < MAX_LINEAS_VISIBLES_B; i++) {
        int fileIdx = scrollOffset + i;
        if (fileIdx >= 10) break; 
        u8g2.setCursor(6, 23 + (i * 12));
        u8g2.print(largosVarilla[fileIdx],1); u8g2.print(" cm");
      }

      for (int i = 0; i < MAX_LINEAS_VISIBLES_B; i++) {
        if (scrollOffset + i == menuIndex) {
          u8g2.setDrawColor(2);         
          u8g2.drawBox(0, 13 + (i * 12), 128, 12); 
          u8g2.setDrawColor(1);         
          break; 
        }
      }
      if (leerBotonEncoder()) {
        largoVarilla = largosVarilla[menuIndex];
        Serial.println(largoVarilla);
        encoderPos = 0;
        ultimoEncoderPosMov = 0;
        posicionSimulada = 0.0;
        estadoActual = ESTADO_MENU_CONFIGURACION;
      }

      break;
    

    case ESTADO_MENU_MOVIMIENTO:
      if (encoderPos > 3) encoderPos = 3;
      if (encoderPos < 0) encoderPos = 0;
      menuIndex = encoderPos;

      u8g2.drawStr(6, 11, "Atras \x1b");
      u8g2.drawStr(6, 23, "Mover X");
      u8g2.drawStr(6, 35, "Mover Y");
      u8g2.drawStr(6, 47, "Mover Z");

      u8g2.setDrawColor(2);         
      u8g2.drawBox(0, 1 + (menuIndex * 12), 128, 12); 
      u8g2.setDrawColor(1);         

      if (leerBotonEncoder()) {
        if (menuIndex == 0) {
          menuIndex = 2; encoderPos = 2; 
          estadoActual = ESTADO_MENU_OPCIONES;
        } else {
          if (menuIndex == 1) ejeSeleccionado = X;
          if (menuIndex == 2) ejeSeleccionado = Y;
          if (menuIndex == 3) ejeSeleccionado = Z;
          menuIndex = 0; encoderPos = 0;
          estadoActual = ESTADO_SELECCION_DISTANCIA;
        }
      }
      break;
    
    case ESTADO_ORIGENES:
      if (encoderPos > 2) encoderPos = 2;
      if (encoderPos < 0) encoderPos = 0;
      menuIndex = encoderPos;

      u8g2.drawStr(6, 11, "Atras \x1b");
      u8g2.drawStr(6, 23, "HOME X");
      u8g2.drawStr(6, 47, "HOME Z");

      u8g2.setDrawColor(2);         
      u8g2.drawBox(0, 1 + (menuIndex * 12), 128, 12); 
      u8g2.setDrawColor(1);

      if (leerBotonEncoder()){
        if (menuIndex == 0) {
          menuIndex = 2; encoderPos = 2; 
          estadoActual = ESTADO_MENU_OPCIONES;
        }
        else{
          if (menuIndex == 1) origenMotor(X);
          if (menuIndex == 2) origenMotor(Z);
        }
      }

      break;

    case ESTADO_SELECCION_DISTANCIA:
      if (encoderPos > 3) encoderPos = 3;
      if (encoderPos < 0) encoderPos = 0;
      menuIndex = encoderPos;

      for (int i = 0; i < 4; i++) {
        u8g2.setCursor(6, 11 + (i * 12));
        u8g2.print(distancias[i], 1);
        if (ejeSeleccionado == 'Y') u8g2.print(" GRAD"); else u8g2.print(" mm");
      }

      u8g2.setDrawColor(2);         
      u8g2.drawBox(0, 1 + (menuIndex * 12), 128, 12); 
      u8g2.setDrawColor(1);         

      if (leerBotonEncoder()) {
        pasoActual = distancias[menuIndex];
        encoderPos = 0;
        ultimoEncoderPosMov = 0;
        posicionSimulada = 0.0;
        estadoActual = ESTADO_EJECUTAR_MOVIMIENTO;
      }
      break;

    case ESTADO_EJECUTAR_MOVIMIENTO:
      u8g2.setCursor(15, 15); u8g2.print("MOVIENDO EJE: "); u8g2.print(ejeSeleccionado);
      u8g2.setCursor(15, 35); u8g2.print("Paso: "); u8g2.print(pasoActual, 1);
      u8g2.setCursor(15, 55); u8g2.print("Cota: "); u8g2.print(posicionSimulada, 1);

      if (encoderPos != ultimoEncoderPosMov) {
        float direccion = (encoderPos > ultimoEncoderPosMov) ? 1.0 : -1.0;
        float mmAMover = pasoActual * direccion;
        posicionSimulada += mmAMover;
        ultimoEncoderPosMov = encoderPos;
      }

      if (leerBotonEncoder()) {
        bool direccion = true;
        menuIndex = 0; encoderPos = 0;
        girarMotorPap(ejeSeleccionado,posicionSimulada);
        estadoActual = ESTADO_MENU_MOVIMIENTO;
      }
      break;

    case ESTADO_FUNCIONAMIENTO:
      int anchoMaximoBarra = 96; 
      int anchoActualBarra = map(datoActual, 0, cantidadDatos, 0, anchoMaximoBarra);
      int porcentaje = map(datoActual, 0, cantidadDatos, 0, 100);

      // 1. Configurar tipografía básica
      u8g2.setFont(u8g2_font_6x10_tr);
      u8g2.drawStr(14, 20, "Trabajo en progreso...");
      u8g2.drawBox(16, 30, anchoActualBarra, 8);
      u8g2.setCursor(52, 54);
      u8g2.print(porcentaje);
      u8g2.print("%");

      if(datoActual = 0)
      {
      inicioPrograma();
      funcionamiento();  
      } 
      else if(datoActual == cantidadDatos)
      {
        apagarMotor();
        origenMotor(X);
        datoActual = 0;
        estadoActual = ESTADO_PANTALLA_PRINCIPAL;
      }
      else
        funcionamiento();
      break;
  }
  u8g2.sendBuffer();
}

#line 1 "C:\\Users\\Agustin\\Documents\\Emprendimientos\\Proyectos\\Miplera\\Firmware\\Miplera\\Motores.ino"
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




