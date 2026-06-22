#include "DFRobotDFPlayerMini.h"
#include <HardwareSerial.h>
#include <OneButton.h>
#include <Adafruit_NeoPixel.h>

// ── LEDs en cascada ──────────────────────────────
#define PIN_LED        4
#define NUM_ANILLOS    9
#define LEDS_POR_ANILLO 3
#define NUM_LEDS_TOTAL 27  // 9 x 3

Adafruit_NeoPixel tira(NUM_LEDS_TOTAL, PIN_LED, NEO_GRB + NEO_KHZ800);

// Colores de cada cuadrado
uint32_t colores[NUM_ANILLOS];

// ── DFPlayer ────────────────────────────────────
HardwareSerial serial_mp3(2);
DFRobotDFPlayerMini reproductor;

// ── Botones ──────────────────────────────────────
#define NUM_BOTONES 11

const int pines[NUM_BOTONES] = {
  13, 22,       // botón 1 (iniciar), botón 2 (pausa)
  14, 32, 21,   // cuadrados 1, 2, 3
  27, 33, 19,   // cuadrados 4, 5, 6
  26, 25, 18    // cuadrados 7, 8, 9
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
  int correcto;
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
  { {31,32,0,0},  2, {-1,-1}, 0, -1} // paso 10: fin
};

// ── Estado ───────────────────────────────────────
int  pasoActual      = 0;
int  audioActual     = 0;
bool reproduciendo   = false;
bool esperandoBoton  = false;
bool cuentoIniciado  = false;

// ── Funciones LEDs ───────────────────────────────
void encenderAnillo(int anillo, uint32_t color) {
  int inicio = anillo * LEDS_POR_ANILLO;
  for (int i = inicio; i < inicio + LEDS_POR_ANILLO; i++) {
    tira.setPixelColor(i, color);
  }
  tira.show();
}

void apagarAnillo(int anillo) {
  encenderAnillo(anillo, tira.Color(0, 0, 0));
}

void apagarTodos() {
  tira.clear();
  tira.show();
}

void celebracionFinal() {
  // Enciende todos los anillos en secuencia
  for (int i = 0; i < NUM_ANILLOS; i++) {
    encenderAnillo(i, colores[i]);
    delay(150);
  }
  delay(2000);
  // Parpadea 3 veces
  for (int v = 0; v < 3; v++) {
    tira.clear();
    tira.show();
    delay(300);
    for (int i = 0; i < NUM_ANILLOS; i++) {
      encenderAnillo(i, colores[i]);
    }
    delay(300);
  }
  delay(1000);
  apagarTodos();
}

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
      Serial.println("Fin del cuento!");
      celebracionFinal();
      cuentoIniciado = false;
      Serial.println("Presiona boton 1 para jugar de nuevo");
      return;
    }

    // Encender cuadrados del paso
    Serial.print("Encendiendo cuadrados: ");
    for (int c = 0; c < p.numCuadrados; c++) {
      int idx = p.cuadrados[c];
      encenderAnillo(idx, colores[idx]);
      Serial.print(idx + 1);
      Serial.print(" ");
    }
    Serial.println();
    Serial.print("Correcto: cuadrado ");
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

  // Botón 1 (i=0) → iniciar o reiniciar
  if (i == 0) {
    Serial.println("Iniciando cuento!");
    apagarTodos();
    reproductor.stop();
    delay(300);
    cuentoIniciado = true;
    pasoActual     = 0;
    audioActual    = 0;
    reproduciendo  = false;
    esperandoBoton = false;
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

  int cuadrado = i - 2; // i=2 → cuadrado 0, i=3 → cuadrado 1...

  Paso& p = secuencia[pasoActual];

  if (cuadrado == p.correcto) {
    // ✅ Correcto
    Serial.print("Correcto! Cuadrado ");
    Serial.println(cuadrado + 1);
    apagarTodos();
    esperandoBoton = false;
    delay(500);
    siguientePaso();
  } else {
    // Verificar si es cuadrado del paso
    bool esCuadradoDelPaso = false;
    for (int c = 0; c < p.numCuadrados; c++) {
      if (p.cuadrados[c] == cuadrado) {
        esCuadradoDelPaso = true;
        break;
      }
    }
    if (esCuadradoDelPaso) {
      // ❌ Incorrecto — parpadea rojo y vuelve a su color
      Serial.print("Incorrecto! Era cuadrado ");
      Serial.println(p.correcto + 1);
      encenderAnillo(cuadrado, tira.Color(255, 0, 0));
      delay(500);
      encenderAnillo(cuadrado, colores[cuadrado]);
    }
  }
}

// ── Setup ────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(1000);

  // Iniciar LEDs
  tira.begin();
  tira.setBrightness(50); // bajo para pruebas con ESP32
  tira.clear();
  tira.show();

  // Definir colores
  colores[0] = tira.Color(255, 0,   0);    // cuadrado 1 — rojo
  colores[1] = tira.Color(0,   0,   255);  // cuadrado 2 — azul
  colores[2] = tira.Color(0,   255, 0);    // cuadrado 3 — verde
  colores[3] = tira.Color(255, 255, 0);    // cuadrado 4 — amarillo
  colores[4] = tira.Color(128, 0,   128);  // cuadrado 5 — morado
  colores[5] = tira.Color(255, 165, 0);    // cuadrado 6 — naranjo
  colores[6] = tira.Color(0,   255, 255);  // cuadrado 7 — cyan
  colores[7] = tira.Color(255, 215, 0);    // cuadrado 8 — dorado
  colores[8] = tira.Color(255, 255, 255);  // cuadrado 9 — blanco

  Serial.println("LEDs OK!");

  // Iniciar DFPlayer
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

  // Callbacks botones
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