#include "AlarmaPresion.h"

//Constructor
AlarmaPresion::AlarmaPresion(float presionMax, int pin) : Alarma(presionMax, pin) {
  tipo=PRESION;
} //Se llama al constructor de la clase base

//Destructor
AlarmaPresion::~AlarmaPresion(){}

//Método comprobar
void AlarmaPresion::comprobar(){
  int rPresion = analogRead(pinSensor);
  float Presion = (rPresion/4095.0)*100; //Convertir el valor del sensor a porcentaje
  
  valorActual = Presion;

  Serial.printf("P: %.2f\n", Presion);
  
  if(Presion>umbral){
    activar(); //Método heredado de Alarma
    Serial.println("ALERTA:PRESION");
  }
}

void AlarmaPresion::imprimirDatos() const{
  Serial.printf("TIPO:PRES;PIN:%d;VAL:%.2f;UM:%.2f;ACT:%d\n",pinSensor, valorActual, umbral, activa);
}