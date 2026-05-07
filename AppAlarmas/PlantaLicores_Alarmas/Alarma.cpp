#include "Alarma.h"

Alarma::Alarma(float u, int p) : umbral(u), pinSensor(p), activa(false) {}

Alarma::~Alarma() {}

void Alarma::activar() {
  activa = true;
}

// Nueva función: permite activar o desactivar
void Alarma::activar(bool estado) {
  activa = estado;
}

void Alarma::imprimirDatos() const {
  Serial.printf("Umbral: %.2f\n", umbral);
}

void registrarEstados(const Alarma& a) {
  a.imprimirDatos();
}