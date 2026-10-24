#ifndef DISPLAY_H 
#define DISPLAY_H

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <U8g2lib.h>

// --- CONFIGURACIÓN DE PINES RAMPS 1.6 ---
#define SD_SS_PIN      53   
#define SD_DETECT_PIN  49   

#define LCD_PINS_RS    16   
#define LCD_PINS_ENABLE 17 
#define LCD_PINS_D4    23   

#define BTN_EN1        31   // Encoder CLK (Fase A)
#define BTN_EN2        33   // Encoder DATA (Fase B)
#define BTN_ENC        35   // Botón del Encoder

// --- FILTROS DE HARDWARE ---
#define FILTRO_VELOCIDAD_MS 60 
#define MAX_FILES 30         
#define MAX_LINEAS_VISIBLES 5
#define MAX_LINEAS_VISIBLES_B 4

// --- ESTADOS DEL SISTEMA ---
enum Estados { 
  ESTADO_PANTALLA_PRINCIPAL, 
  ESTADO_MENU_OPCIONES,      
  ESTADO_EXPLORADOR_SD,
  ESTADO_MENU_MOVIMIENTO,    
  ESTADO_SELECCION_DISTANCIA,
  ESTADO_EJECUTAR_MOVIMIENTO,
  ESTADO_FUNCIONAMIENTO,
  ESTADO_DEFINIR_DIAMETRO_VARILLA,
  ESTADO_ORIGENES,
  ESTADO_MENU_CONFIGURACION,
  ESTADO_DEFINIR_LARGO_VARILLA
};

// --- VARIABLES GLOBALES COMPARTIDAS (extern) ---
extern U8G2_ST7920_128X64_F_SW_SPI u8g2;
extern Estados estadoActual;
extern bool sdPresente;
extern bool ultimaSDPresente;
extern String fileNames[MAX_FILES];
extern int fileCount; 
extern int menuIndex;           
extern int scrollOffset;        
extern int encoderPos;
extern byte ultimoCLK;
extern byte ultimoDATA;
extern unsigned long ultimoTiempoGiro;
extern bool botonPresionado;
extern unsigned long ultimoTiempoBoton;

extern char ejeSeleccionado; 
extern float distancias[];
extern int diametros[];
extern int largosVarilla[];
extern int distanciaIndex;
extern float pasoActual;
extern double posicionSimulada;
extern int ultimoEncoderPosMov;

extern float* valoresArray;
extern int valoresCount;



// --- PROTOTIPOS DE FUNCIONES ---
void buscarArchivos();
void leerYParsearArchivo(String nombre);
void procesarHardware();
void evaluarEncoderCruzado();
bool leerBotonEncoder();
void liberarMemoriaArray();
void initDisplay();
void estadosDisplay();

#endif
