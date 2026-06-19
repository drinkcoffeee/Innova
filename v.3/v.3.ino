#include "DFRobotDFPlayerMini.h"
#include <HardwareSerial.h>
#include <OneButton.h>

HardwareSerial serial_mp3(2);
DFRobotDFPlayerMini reproductor;

#define NUM_BOTONES 11

const int pines[NUM_BOTONES] = {
  13, 22,       // botón 1, 2  naranjo y naranjo
  14, 32, 21,   // botón 3, 4, 5  amarillo, blanco, caca
  27, 33, 19,   // botón 6, 7, 8  gris, azul, cafe
  26, 25, 18    // botón 9, 10, 11  morado, verde, negro
};

OneButton botones[NUM_BOTONES] = {
  OneButton(pines[0],  true),
  OneButton(pines[1],  true),
  OneButton(pines[2],  true),
  OneButton(pines[3],  true),
  OneButton(pines[4],  true),
  OneButton(pines[5],  true),
  OneButton(pines[6],  true),
  OneButton(pines[7],  true),
  OneButton(pines[8],  true),
  OneButton(pines[9],  true),
  OneButton(pines[10], true)
};

// ── Secuencia del cuento ─────────────────────────
struct Paso {
  int audios[4];
  int numAudios;
  int cuadrados[2];  // índice 0 = cuadrado 1
  int numCuadrados;
  int correcto;      // índice 0 = cuadrado 1
};

const int NUM_PASOS = 11;

Paso secuencia[NUM_PASOS] = {
  { {1,2,3,0},    3, {0,2}, 2, 0 },  // paso 0:  cuadrados 1,3 — correcto 1
  { {4,5,6,0},    3, {0,1}, 2, 0 },  // paso 1:  cuadrados 1,2 — correcto 1
  { {7,8,9,0},    3, {5,3}, 2, 5 },  // paso 2:  cuadrados 6,4 — correcto 6
  { {10,11,12,0}, 3, {7,1}, 2, 7 },  // paso 3:  cuadrados 8,2 — correcto 8
  { {13,14,15,0}, 3, {5,3}, 2, 5 },  // paso 4:  cuadrados 6,4 — correcto 6
  { {16,17,18,0}, 3, {8,0}, 2, 8 },  // paso 5:  cuadrados 9,1 — correcto 9
  { {19,20,21,0}, 3, {7,1}, 2, 1 },  // paso 6:  cuadrados 8,2 — correcto 2
  { {22,23,24,0}, 3, {0,1}, 2, 1 },  // paso 7:  cuadrados 1,2 — correcto 2
  { {25,26,27,0}, 3, {3,5}, 2, 3 },  // paso 8:  cuadrados 4,6 — correcto 4
  { {28,29,30,0}, 3, {2,5}, 2, 2 },  // paso 9:  cuadrados 3,6 — correcto 3
  { {31,32,0,0},  2, {-1,-1}, 0, -1} // paso 10: fin del cuento
};

// ── Estado ───────────────────────────────────────
int  pasoActual      = 0;
int  audioActual     = 0;
bool reproduciendo   = false;
bool esperandoBoton  = false;
bool cuentoIniciado  = false;

// ── Reproducir audios en secuencia ───────────────
void reproducirSiguienteAudio() {
  Paso& p = secuencia[pasoActual];

  if (audioActual < p.numAudios) {
    int audio = p.audios[audioActual];
    Serial.print("Reproduciendo audio ");
    Serial.println(audio);
    reproductor.play(audio);
    reproduciendo = true;
    audioActual++;
  } else {
    reproduciendo = false;

    if (p.numCuadrados == 0) {
      // Fin del cuento
      Serial.println("Fin del cuento!");
      cuentoIniciado = false;
      Serial.println("Presiona boton 1 para jugar de nuevo");
      return;
    }

    // Mostrar qué cuadrados debe pisar
    Serial.print("Pisa el cuadrado: ");
    for (int c = 0; c < p.numCuadrados; c++) {
      Serial.print(p.cuadrados[c] + 1);
      Serial.print(" ");
    }
    Serial.println();
    Serial.print("Correcto: ");
    Serial.println(p.correcto + 1);

    esperandoBoton = true;
  }
}

// ── Avanzar al siguiente paso ─────────────────────
void siguientePaso() {
  pasoActual++;
  audioActual    = 0;
  esperandoBoton = false;
  reproduciendo  = false;

  if (pasoActual >= NUM_PASOS) {
    pasoActual = 0;
  }

  delay(800);
  reproducirSiguienteAudio();
}

// ── Verificar fin de audio ────────────────────────
void verificarAudio() {
  if (!reproduciendo) return;

  if (reproductor.available()) {
    if (reproductor.readType() == DFPlayerPlayFinished) {
      Serial.println("Audio terminado");
      reproduciendo = false;
      delay(300);
      reproducirSiguienteAudio();
    }
  }
}

// ── Callback botones ─────────────────────────────
void alPresionar(void* indice) {
  int i = (int)(intptr_t)indice;

  // Botón 1 (i=0) → iniciar o reiniciar cuento
  if (i == 0) {
    Serial.println("Iniciando cuento!");
    cuentoIniciado = true;
    pasoActual     = 0;
    audioActual    = 0;
    reproduciendo  = false;
    esperandoBoton = false;
    reproductor.stop();
    delay(300);
    reproducirSiguienteAudio();
    return;
  }

  // Botón 2 (i=1) → pausar / reanudar
  if (i == 1) {
    if (!cuentoIniciado) return;
    if (reproduciendo) {
      reproductor.pause();
      reproduciendo = false;
      Serial.println("Pausado");
    } else {
      reproductor.start();
      reproduciendo = true;
      Serial.println("Reanudado");
    }
    return;
  }

  // Botones 3–11 (i=2..10) → cuadrados 1–9
  if (!cuentoIniciado) return;
  if (!esperandoBoton) return;

  // i=2 → cuadrado índice 0 (cuadrado 1)
  // i=3 → cuadrado índice 1 (cuadrado 2)
  // ...
  // i=10 → cuadrado índice 8 (cuadrado 9)
  int cuadrado = i - 2;

  Paso& p = secuencia[pasoActual];

  if (cuadrado == p.correcto) {
    // ✅ Correcto
    Serial.print("Correcto! Pisaste cuadrado ");
    Serial.println(cuadrado + 1);
    esperandoBoton = false;
    delay(500);
    siguientePaso();
  } else {
    // Verificar si es uno de los cuadrados del paso
    bool esCuadradoDelPaso = false;
    for (int c = 0; c < p.numCuadrados; c++) {
      if (p.cuadrados[c] == cuadrado) {
        esCuadradoDelPaso = true;
        break;
      }
    }

    if (esCuadradoDelPaso) {
      // ❌ Incorrecto
      Serial.print("Incorrecto! Era el cuadrado ");
      Serial.println(p.correcto + 1);
    }
    // Si no es cuadrado del paso → ignorar
  }
}

// ── Setup ────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(1000);

  serial_mp3.begin(9600, SERIAL_8N1, 16, 17);
  delay(1000);

  if (!reproductor.begin(serial_mp3)) {
    Serial.println("Error: no se encontro el DFPlayer");
    while (true);
  }

  reproductor.volume(0);
  delay(1000);
  reproductor.volume(20);
  Serial.println("DFPlayer OK!");

  for (int i = 0; i < NUM_BOTONES; i++) {
    botones[i].attachClick(alPresionar, (void*)(intptr_t)i);
  }

  Serial.println("Sistema listo!");
  Serial.println("Presiona boton 1 para iniciar el cuento");
}

// ── Loop ─────────────────────────────────────────
void loop() {
  for (int i = 0; i < NUM_BOTONES; i++) {
    botones[i].tick();
  }
  verificarAudio();
}