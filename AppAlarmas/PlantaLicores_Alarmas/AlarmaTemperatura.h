#pragma once
#include "Alarma.h"
#include "DHT.h"

class AlarmaTemperatura:public Alarma{
  private:
  void imprimirDatos() const override;
  DHT dht;
  public:
  explicit AlarmaTemperatura(float tempMax, int pin); //Constructor
  ~AlarmaTemperatura() override; //Destructor
  
  void comprobar() override; //Sobrescritura del método comprobar (polimorfismo)
};
