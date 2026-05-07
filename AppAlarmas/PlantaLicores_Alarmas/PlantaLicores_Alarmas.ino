#include "GestorAlarmas.h"
#include "AlarmaTemperatura.h"
#include "AlarmaNivel.h"
#include "AlarmaPresion.h"

GestorAlarmas* gestor = nullptr;
String comando = "";

void leerComando() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n') {
      comando.trim();
      break;
    }
    if (c != '\r') comando += c;
  }
}

void procesarComando() {
  if (gestor == nullptr || comando.length() == 0) return;

  if (comando == "CLEAR:TEMP") {
    gestor->resetearTemperatura();
    Serial.println("OK:CLEAR_TEMP");
  }
  else if (comando == "CLEAR:NIVEL") {
    gestor->resetearNivel();
    Serial.println("OK:CLEAR_NIVEL");
  }
  else if (comando == "CLEAR:PRESION") {
    gestor->resetearPresion();
    Serial.println("OK:CLEAR_PRESION");
  }
  else if (comando == "CLEAR:TODO") {
    gestor->resetearTodas();
    Serial.println("OK:CLEAR_ALL");
  }
  else if (comando == "GET:ESTADO") {
    gestor->enviarEstados();
  }

  comando = "";
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("------ GESTOR DE ALARMAS ------");
  Serial.println("Sistema iniciado.");

  gestor = new GestorAlarmas();

  gestor->agregarAlarma(new AlarmaTemperatura(30.0f, 4));
  gestor->agregarAlarma(new AlarmaNivel(0.0f, 85.0f, 34));
  gestor->agregarAlarma(new AlarmaPresion(45.0f, 22));

  Serial.println("Alarmas inicializadas correctamente.");
  Serial.println("Listo para recibir comandos desde la aplicación.\n");
}

static unsigned long lastSample = 0;
const unsigned long SAMPLE_MS = 500;

void loop() {
  leerComando();
  procesarComando();

  unsigned long now = millis();
  if (now - lastSample >= SAMPLE_MS) {
    lastSample = now;
    if (gestor) gestor->procesarAlarmas();
  }
}