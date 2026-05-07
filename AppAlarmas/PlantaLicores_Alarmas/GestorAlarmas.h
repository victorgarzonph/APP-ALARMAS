#pragma once
#include "Alarma.h"
#include <vector>
#include <cstddef>

class GestorAlarmas{
  private:
    std::vector<Alarma*> alarmas;
  
  public:
    GestorAlarmas();
    ~GestorAlarmas();

    void agregarAlarma(Alarma* a);
    void procesarAlarmas();
    void enviarEstados();
    void loop();

    void resetearTemperatura();
    void resetearNivel();
    void resetearPresion();
    void resetearTodas();

    size_t cantidadAlarmas() const;
};