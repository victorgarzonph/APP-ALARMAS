#include "AlarmaNivel.h"

//Constructor
AlarmaNivel::AlarmaNivel(float nMin, float nMax, int pin)
  : Alarma((nMin+nMax)/2.0f, pin), //Se llama al constructor base
    nivelMin(nMin),
    nivelMax(nMax)
{
  tipo=NIVEL;
  //Validación de rangos
  if (nivelMin>nivelMax){
    nivelMin=0.0f;
    nivelMax=100.0f;
  }
}

//Destructor
AlarmaNivel::~AlarmaNivel(){}  

//Método comprobar
void AlarmaNivel::comprobar(){
  int rNivel = analogRead(pinSensor);
  float Nivel=(rNivel/4095.0)*100; //convertir valor del sensor a porcentaje
  
  valorActual=Nivel;

  Serial.printf("N: %.2f\n", Nivel);
  
  if(Nivel<nivelMin || Nivel>nivelMax){
    activar(); //Método heredado de Alarma
    Serial.println("ALERTA:NIVEL");
  }
}

void AlarmaNivel::imprimirDatos() const{
  Serial.printf("TIPO:NIVEL;PIN:%d;VAL:%.2f;MIN:%.2f;MAX:%.2f;ACT:%d\n",pinSensor,valorActual, nivelMin, nivelMax, activa);
}
