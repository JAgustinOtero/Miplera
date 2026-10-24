#line 1 "C:\\Users\\Agustin\\Documents\\Emprendimientos\\Proyectos\\Miplera\\Firmware\\Miplera\\Comandos.h"
#ifndef COMANDOS_H 
#define COMANDOS_H

#define ALTURA_FUNCIONAMIENTO (double)(alturaVarilla + diametroVarilla/2 - datos[datoActual])

void lecturaPuertoSerie();
void mostrarDatosLeidos();
void printBanner();
void procesarNumeroTemporal();
void mostrarDatos();
void comandoG0(String);
void comandoG1(String);
void comandoG2(String);
void comandoG4(String);
void comandoS1(String);
void comandoG5(String comando);
void comandoHelp();
void inicioPrograma();
void funcionamiento();

#endif