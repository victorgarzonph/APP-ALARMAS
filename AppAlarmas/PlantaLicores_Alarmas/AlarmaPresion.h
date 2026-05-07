#pragma once
#include "Alarma.h"

class AlarmaPresion:public Alarma{
  private:
  void imprimirDatos() const override;
  
  public:
  explicit AlarmaPresion(float presionMax, int pin); //Constructor
  ~AlarmaPresion() override; //Destructor

  void comprobar() override; //Sobrescritura del método comprobar
};