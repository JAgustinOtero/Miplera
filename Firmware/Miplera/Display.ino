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
