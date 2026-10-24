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

