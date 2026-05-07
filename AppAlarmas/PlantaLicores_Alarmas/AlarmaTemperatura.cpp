#include "AlarmaTemperatura.h"
#include "DHT.h"

#define DHTTYPE DHT11

//Constructor
AlarmaTemperatura::AlarmaTemperatura(float tempMax, int pin) : Alarma(tempMax, pin), dht(pin, DHTTYPE) {
  tipo=TEMP;
  dht.begin();
} 

//Destructor
AlarmaTemperatura::~AlarmaTemperatura(){}

//Método comprobar
void AlarmaTemperatura::comprobar(){
  float temp = dht.readTemperature();

  if(isnan(temp)) return;

  valorActual = temp;

  Serial.printf("T: %.2f\n", temp);
  
  if(temp>umbral){
    activar(); //Llama el método de la clase base
    Serial.println("ALERTA:TEMP");
  }
}

void AlarmaTemperatura::imprimirDatos() const{
  Serial.printf("TIPO:TEMP;PIN:%d;VAL:%.2f;UM:%.2f;ACT:%d\n",pinSensor,valorActual, umbral, activa);
}