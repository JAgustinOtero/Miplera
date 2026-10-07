# 1 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino"
# 2 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino" 2
# 3 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino" 2
# 4 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino" 2
# 5 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino" 2
# 6 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino" 2



int diametroVarilla = 22;
int alturaVarilla = 15;

const int FIN_DE_CARRERA_X = 3;
const int FIN_DE_CARRERA_Z = 2;
const int MOTOR_DC = 8;


double posX = 0, posY = 0, posZ = 0, posM = 0;


float datos[720];
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

  pinMode(FIN_DE_CARRERA_X, 0x2);
  pinMode(FIN_DE_CARRERA_Z, 0x2);

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
    if (indiceActual >= 720) {
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
  Serial.println((reinterpret_cast<const __FlashStringHelper *>(
# 201 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino" 3
                (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 201 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino"
                "                                                                                         "
# 201 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino" 3
                ); &__c[0];}))
# 201 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino"
                )));
  Serial.println((reinterpret_cast<const __FlashStringHelper *>(
# 202 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino" 3
                (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 202 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino"
                "  ███╗   ███╗██╗██████╗ ██╗     ███████╗██████╗  █████╗                                   "
# 202 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino" 3
                ); &__c[0];}))
# 202 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino"
                )));
  Serial.println((reinterpret_cast<const __FlashStringHelper *>(
# 203 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino" 3
                (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 203 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino"
                "  ████╗ ████║██║██╔══██╗██║     ██╔════╝██╔══██╗██╔══██╗                                  "
# 203 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino" 3
                ); &__c[0];}))
# 203 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino"
                )));
  Serial.println((reinterpret_cast<const __FlashStringHelper *>(
# 204 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino" 3
                (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 204 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino"
                "  ██╔████╔██║██║██████╔╝██║     █████╗  ██████╔╝███████║                                  "
# 204 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino" 3
                ); &__c[0];}))
# 204 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino"
                )));
  Serial.println((reinterpret_cast<const __FlashStringHelper *>(
# 205 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino" 3
                (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 205 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino"
                "  ██║╚██╔╝██║██║██╔═══╝ ██║     ██╔══╝  ██╔══██╗██╔══██║                                  "
# 205 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino" 3
                ); &__c[0];}))
# 205 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino"
                )));
  Serial.println((reinterpret_cast<const __FlashStringHelper *>(
# 206 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino" 3
                (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 206 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino"
                "  ██║ ╚═╝ ██║██║██║     ███████╗███████╗██║  ██║██║  ██║                                  "
# 206 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino" 3
                ); &__c[0];}))
# 206 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino"
                )));
  Serial.println((reinterpret_cast<const __FlashStringHelper *>(
# 207 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino" 3
                (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 207 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino"
                "  ╚═╝     ╚═╝╚═╝╚═╝     ╚══════╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═╝                                  "
# 207 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino" 3
                ); &__c[0];}))
# 207 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino"
                )));
  Serial.println((reinterpret_cast<const __FlashStringHelper *>(
# 208 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino" 3
                (__extension__({static const char __c[] __attribute__((__progmem__)) = (
# 208 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino"
                "                                                                                         "
# 208 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino" 3
                ); &__c[0];}))
# 208 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Miplera.ino"
                )));
}
# 1 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Comandos.ino"
# 2 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Comandos.ino" 2


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
        origenMotor(0,FIN_DE_CARRERA_X);
        posX = 0;
      }

      else if(comando[0] == 'Z'){
        origenMotor(2,FIN_DE_CARRERA_Z);
        posZ = 0;
      }

      else{
        origenMotor(0,FIN_DE_CARRERA_X);
        origenMotor(2,FIN_DE_CARRERA_Z);
      }
  mostrarDatos();
}

void comandoG1(String comando){
    double distancia = comando.substring(1).toDouble();
    int motor = 0, velocidad = 60;
    bool direccion = false;
    if(comando[0] == 'X')
    {
      if(450 <= posX + distancia)
        distancia = 450 - posX;
      posX = posX + distancia;
      motor = 0;
      direccion = !(true);
    }
    else if(comando[0] == 'Y')
    {
      posY = posY + distancia;
      distancia = distancia/360;
      motor = 1;
    }
    else if(comando[0] == 'M')
    {
      distancia = distancia/360;
      posM = posM + distancia;
      motor = 3;
    }
    else if(comando[0] == 'Z')
    {
      if(30 <= posZ + distancia)
        distancia = 30 - posZ;
      posZ = posZ + distancia;
      motor = 2;
      direccion = !(false);
      velocidad = 10;
    }
  //Serial.print(distancia,7);
  girarMotorPap(motor,direccion,distancia,velocidad);
  mostrarDatos();
}

void comandoG2(String comando){
    double distancia = comando.substring(1).toDouble();
    int motor = 0, velocidad = 60;
    bool direccion = true;
    if(comando[0] == 'X')
    {
      if(0 >= posX - distancia)
        distancia = posX;
      posX = posX - distancia;
      motor = 0;
      direccion = (true);
    }
    else if(comando[0] == 'Y')
    {
      posY = posY - distancia;
      distancia = distancia/360;
      motor = 1;
    }
    else if(comando[0] == 'M')
    {
      posM = posM - distancia;
      distancia = distancia/360;
      motor = 3;
    }
    else if(comando[0] == 'Z')
    {
      if(0 >= posZ - distancia)
        distancia = posZ;
      posZ = posZ - distancia;
      motor = 2;
      velocidad = 15;
      direccion = (false);
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
  bool sentido = !(true);
  Serial.print("Programa iniciado");
  posY = 0;
  posM = 0;
  /* INICIO DE CORTE*/
      origenMotor(0,FIN_DE_CARRERA_X);
      origenMotor(2,FIN_DE_CARRERA_Z);
      mostrarDatos();
      encenderMotor();
      for(int i=0;i<720;i++){
        // calcular movimiento Z (altura)
        //altura = promedioDistancias(CANT_MUESTRAS) * 4;
        girarMotorPap(2,!(false),/*altura*/(alturaVarilla + diametroVarilla/2 - datos[i]),15);
        posY = (alturaVarilla + diametroVarilla/2 - datos[i]);
        mostrarDatos();
        girarMotorPap(0,sentido,/*MAXIMO_X*/200,60);
        posX = 200*(int)!sentido;
        sentido = !sentido;
        girarMotorPap(2,(false),/*altura*/(alturaVarilla + diametroVarilla/2 - datos[i]),15);
        posZ = 0;
        girarMotorPap(1,true,((double)1/720),5);
        posY = posY + 1;
        //girarMotorPap(M,true,((double)1/TOTAL_FILAS),VELOCIDAD_FUNC_Y);
        //posM = posM + 1;
      }
      apagarMotor();
      origenMotor(0,FIN_DE_CARRERA_X);
}
# 1 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Motores.ino"
# 2 "C:\\Users\\a.otero_admin\\OneDrive - Colegio Bayard\\Escritorio\\Miplera\\Firmware\\Miplera\\Motores.ino" 2

void girarMotorPap(int motor,bool dir, double distancia, int velocidad){
  long stepPin = 0, dirPin = 0, microsteps = 0;
  if(motor == 0)
  {
    stepPin = 26;
    dirPin = 28;
    microsteps = 400;

  }
  else if(motor == 1)
  {
    stepPin = 60;
    dirPin = 61;
    microsteps = 3200*4;
  }
  else if(motor == 3)
  {
    stepPin = 46;
    dirPin = 48;
    microsteps = 3200;
  }
  else if(motor == 2)
  {
    stepPin = 54;
    dirPin = 55;
    microsteps = 100;
  }
  digitalWrite(dirPin, dir);
  for (double i = 0; i < (microsteps) * distancia; i++) {
    digitalWrite(stepPin, 0x1);
    delayMicroseconds(1000000/80/velocidad);
    digitalWrite(stepPin, 0x0);
    delayMicroseconds(1000000/80/velocidad);
  }
}

void origenMotor(int motor, int stopPin){
  int distanciaInicio = 0;
  int dir = 0, velocidad = 0;

  if (motor == 0){
    distanciaInicio = posX;
    dir = true;
    velocidad = 60 + 20;
    posX = 0;
  }
  else if (motor == 2){
    distanciaInicio = posZ;
    dir = false;
    velocidad = 15;
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
  digitalWrite(MOTOR_DC, 0x1);
}

void apagarMotor(){
  digitalWrite(MOTOR_DC, 0x0);
}

void initMotores(){
  pinMode(MOTOR_DC, 0x2);
  /*MOTOR X*/

  pinMode(26, 0x1);
  pinMode(28, 0x1);
  pinMode(24, 0x1);
  digitalWrite(24, 0x0); // Habilita el A4988
  digitalWrite(28, 0x1);

  /*MOTOR Y*/
  pinMode(60, 0x1);
  pinMode(61, 0x1);
  pinMode(56, 0x1);
  digitalWrite(56, 0x0); // Habilita el A4988
  digitalWrite(61, 0x1);

    /*MOTOR Y2*/
  pinMode(46, 0x1);
  pinMode(48, 0x1);
  pinMode(62, 0x1);
  digitalWrite(62, 0x0); // Habilita el A4988
  digitalWrite(48, 0x1);

  /*MOTOR Z*/
  pinMode(54, 0x1);
  pinMode(55, 0x1);
  pinMode(38, 0x1);
  digitalWrite(38, 0x0); // Habilita el A4988
  digitalWrite(55, 0x1);

  /*MOTOR DC*/
  digitalWrite(MOTOR_DC, 0x0);

}
