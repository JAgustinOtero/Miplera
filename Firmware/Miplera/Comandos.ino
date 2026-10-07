#include "Comandos.h"
#define ALTURA_FUNCIONAMIENTO (double)(alturaVarilla + diametroVarilla/2 - datos[i])

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
        origenMotor(Z,FIN_DE_CARRERA_Z);
        origenMotor(X,FIN_DE_CARRERA_X);
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
  origenMotor(Z,FIN_DE_CARRERA_Z);
  origenMotor(X,FIN_DE_CARRERA_X);
  mostrarDatos();
  encenderMotor();
  for(int i=0;i<cantidadDatos;i++){
    girarMotorPap(Z,!(HOME_Z),ALTURA_FUNCIONAMIENTO,VELOCIDAD_FUNC_Z); //baja eje Z
    posY = ALTURA_FUNCIONAMIENTO;
    girarMotorPap(X,!(HOME_X),MAXIMO_X,VELOCIDAD_FUNC_X/2); //corta
    sentido = !sentido;
    girarMotorPap(Z,(HOME_Z),ALTURA_FUNCIONAMIENTO,VELOCIDAD_FUNC_Z);//sube eje Z
    posZ = 0;
    girarMotorPap(X,(HOME_X),MAXIMO_X,VELOCIDAD_FUNC_X); //vuelve
    sentido = !sentido;
    girarMotorPap(Y,true,((double)1/cantidadDatos),VELOCIDAD_FUNC_Y); //gira
    posY = posY + 360/cantidadDatos;
  }
  apagarMotor();
  origenMotor(X,FIN_DE_CARRERA_X);
}