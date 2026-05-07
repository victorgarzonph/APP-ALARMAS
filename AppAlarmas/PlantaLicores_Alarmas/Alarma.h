#pragma once
#include <Arduino.h>

enum TipoAlarma{
  TEMP,
  NIVEL,
  PRESION
};

class Alarma{
  protected:
    float valorActual;
    float umbral; 
    int pinSensor;
    bool activa;
    TipoAlarma tipo;

    virtual void imprimirDatos() const = 0;

  public:
    explicit Alarma(float u = 0.0f, int p = 0);
    virtual ~Alarma();
    
    void activar();           // Activar alarma
    void activar(bool estado); // Nueva sobrecarga: activar/desactivar

    virtual void comprobar() = 0;

    TipoAlarma getTipo() const { return tipo; }

    inline bool estaActiva() const { return activa; }

    friend void registrarEstados(const Alarma& a);
};
