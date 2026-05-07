#pragma once
#include "Alarma.h"

class AlarmaNivel:public Alarma{
  private:
    float nivelMin;
    float nivelMax;

    void imprimirDatos() const override;

  public:
    explicit AlarmaNivel(float nMin, float nMax, int pin); //Constructor
    ~AlarmaNivel() override; //Destructor
    
    virtual void comprobar() override; //Sobrescritura del método comprobar (polimorfismo)

};