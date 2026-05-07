#include "GestorAlarmas.h"
#include "AlarmaTemperatura.h"
#include "AlarmaNivel.h"
#include "AlarmaPresion.h"
#include <algorithm>

GestorAlarmas::GestorAlarmas() {}

GestorAlarmas::~GestorAlarmas() {
  for (auto* a : alarmas) delete a;
  alarmas.clear();
}

void GestorAlarmas::agregarAlarma(Alarma* a) {
  if (a != nullptr) {
    alarmas.push_back(a);
  }
}

void GestorAlarmas::procesarAlarmas() {
  for (auto* a : alarmas) {
    if (a != nullptr) {
      a->comprobar();
    }
  }
}

void GestorAlarmas::enviarEstados() {
  for (auto* a : alarmas) {
    if (a != nullptr) {
      registrarEstados(*a);
    }
  }
}

void GestorAlarmas::loop() {
  procesarAlarmas();
}

// ==================== NUEVOS MÉTODOS: RESETEAR (solo desactivar) ====================

void GestorAlarmas::resetearTemperatura() {
  for (auto* a : alarmas) {
    if (a && a->getTipo() == TEMP) {
      a->activar(false);        // Nueva función que agregaremos en Alarma
    }
  }
}

void GestorAlarmas::resetearNivel() {
  for (auto* a : alarmas) {
    if (a && a->getTipo() == NIVEL) {
      a->activar(false);
    }
  }
}

void GestorAlarmas::resetearPresion() {
  for (auto* a : alarmas) {
    if (a && a->getTipo() == PRESION) {
      a->activar(false);
    }
  }
}

void GestorAlarmas::resetearTodas() {
  for (auto* a : alarmas) {
    if (a) a->activar(false);
  }
}

size_t GestorAlarmas::cantidadAlarmas() const {
  return alarmas.size();
}