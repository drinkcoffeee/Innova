#include <FastLED.h>
#include "DFRobotDFPlayerMini.h"
#include <HardwareSerial.h>
#include <OneButton.h>

// ── LEDs ────────────────────────────────────────
#define NUM_ANILLOS 9
#define LEDS_POR_ANILLO 3

CRGB leds[NUM_ANILLOS][LEDS_POR_ANILLO];

CRGB colores[NUM_ANILLOS] = {
  CRGB::Red,    // cuadrado 1
  CRGB::Blue,   // cuadrado 2
  CRGB::Green,  // cuadrado 3
  CRGB::Yellow, // cuadrado 4
  CRGB::Purple, // cuadrado 5
  CRGB::Orange, // cuadrado 6
  CRGB::Cyan,   // cuadrado 7
  CRGB::Gold,   // cuadrado 8
  CRGB::White   // cuadrado 9
};

// ── DFPlayer ────────────────────────────────────
HardwareSerial serial_mp3(2);
DFRobotDFPlayerMini reproductor;

// ── Botones ──────────────────────────────────────
// 1 botón control + 9 cuadrados = 10 total
#define NUM_BOTONES 10

const int pinesBotones[NUM_BOTONES] = {
  13,            // índice 0 → inicio/pausa/reinicio
  14, 32, 21,    // índice 1,2,3 → cuadrados 1,2,3
  27, 33, 26,    // índice 4,5,6 → cuadrados 4,5,6
  25, 34, 35     // índice 7,8,9 → cuadrados 7,8,9
};

OneButton botones[NUM_BOTONES] = {
  OneButton(pinesBotones[0], true),
  OneButton(pinesBotones[1], true),
  OneButton(pinesBotones[2], true),
  OneButton(pinesBotones[3], true),
  OneButton(pinesBotones[4], true),
  OneButton(pinesBotones[5], true),
  OneButton(pinesBotones[6], true),
  OneButton(pinesBotones[7], true),
  OneButton(pinesBotones[8], true, false),  // GPIO34 sin pull-up
  OneButton(pinesBotones[9], true, false)   // GPIO35 sin pull-up
};

// ── Secuencia del cuento ─────────────────────────
struct Paso {
  int audios[4];
  int numAudios;
  int cuadrados[2];
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
  { {16,17,18,0}, 3, {6,0}, 2, 6 },  // paso 5:  cuadrados 7,1 — correcto 7
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
bool pausado         = false;
bool esperandoBoton  = false;
bool cuentoIniciado  = false;

// ── Funciones LEDs ───────────────────────────────
void encenderAnillo(int anillo, CRGB color) {
  for (int i = 0; i < LEDS_POR_ANILLO; i++) {
    leds[anillo][i] = color;
  }
  FastLED.show();
}

void apagarAnillo(int anillo) {
  encenderAnillo(anillo, CRGB::Black);
}

void apagarTodos() {
  for (int a = 0; a < NUM_ANILLOS; a++) {
    apagarAnillo(a);
  }
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
      for (int i = 0; i < NUM_ANILLOS; i++) {
        encenderAnillo(i, colores[i]);
        delay(150);
      }
      delay(3000);
      apagarTodos();
      cuentoIniciado = false;
      Serial.println("Click corto para jugar de nuevo");
      return;
    }

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

// ── Reiniciar juego ───────────────────────────────
void reiniciarJuego() {
  Serial.println("Reiniciando...");
  apagarTodos();
  reproductor.stop();
  pasoActual     = 0;
  audioActual    = 0;
  reproduciendo  = false;
  pausado        = false;
  esperandoBoton = false;
  cuentoIniciado = false;
  Serial.println("Click corto para iniciar");
}

// ── Callbacks botón control ───────────────────────
void clickCorto(void* indice) {
  // Click corto → iniciar si no hay cuento, pausar/reanudar si hay
  if (!cuentoIniciado) {
    Serial.println("Iniciando cuento!");
    cuentoIniciado = true;
    pausado        = false;
    pasoActual     = 0;
    audioActual    = 0;
    reproduciendo  = false;
    esperandoBoton = false;
    reproducirSiguienteAudio();
  } else {
    if (!pausado) {
      reproductor.pause();
      pausado = true;
      Serial.println("Pausado");
    } else {
      reproductor.start();
      pausado       = false;
      reproduciendo = true;
      Serial.println("Reanudado");
    }
  }
}

void clickLargo() {
  // Click largo → reiniciar siempre
  reiniciarJuego();
}

// ── Callback cuadrados ────────────────────────────
void alPresionar(void* indice) {
  int i = (int)(intptr_t)indice;

  if (!cuentoIniciado) return;
  if (pausado)         return;
  if (!esperandoBoton) return;

  // i=1..9 → cuadrado índice 0..8
  int cuadrado = i - 1;

  Paso& p = secuencia[pasoActual];

  if (cuadrado == p.correcto) {
    Serial.print("Correcto! Cuadrado ");
    Serial.println(cuadrado + 1);
    apagarTodos();
    delay(500);
    esperandoBoton = false;
    siguientePaso();
  } else {
    bool esCuadradoDelPaso = false;
    for (int c = 0; c < p.numCuadrados; c++) {
      if (p.cuadrados[c] == cuadrado) {
        esCuadradoDelPaso = true;
        break;
      }
    }
    if (esCuadradoDelPaso) {
      Serial.print("Incorrecto! Era cuadrado ");
      Serial.println(p.correcto + 1);
      encenderAnillo(cuadrado, CRGB::Red);
      delay(500);
      encenderAnillo(cuadrado, colores[cuadrado]);
    }
  }
}

// ── Setup ────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("Iniciando sistema...");

  // LEDs
  FastLED.addLeds<WS2812B, 4,  GRB>(leds[0], LEDS_POR_ANILLO); // cuadrado 1
  FastLED.addLeds<WS2812B, 5,  GRB>(leds[1], LEDS_POR_ANILLO); // cuadrado 2
  FastLED.addLeds<WS2812B, 15, GRB>(leds[2], LEDS_POR_ANILLO); // cuadrado 3
  FastLED.addLeds<WS2812B, 23, GRB>(leds[3], LEDS_POR_ANILLO); // cuadrado 4
  FastLED.addLeds<WS2812B, 12, GRB>(leds[4], LEDS_POR_ANILLO); // cuadrado 5
  FastLED.addLeds<WS2812B, 19, GRB>(leds[5], LEDS_POR_ANILLO); // cuadrado 6
  FastLED.addLeds<WS2812B, 18, GRB>(leds[6], LEDS_POR_ANILLO); // cuadrado 7
  FastLED.addLeds<WS2812B, 22, GRB>(leds[7], LEDS_POR_ANILLO); // cuadrado 8
  FastLED.addLeds<WS2812B, 3,  GRB>(leds[8], LEDS_POR_ANILLO); // cuadrado 9

  FastLED.setBrightness(80);
  apagarTodos();
  Serial.println("LEDs OK!");

  // DFPlayer
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

  // Botón control (índice 0) — click corto y largo
  botones[0].attachClick(clickCorto, nullptr);
  botones[0].attachLongPressStart(clickLargo);
  botones[0].setPressTicks(2000); // 2 segundos para click largo

  // Botones cuadrados (índices 1–9)
  for (int i = 1; i < NUM_BOTONES; i++) {
    botones[i].attachClick(alPresionar, (void*)(intptr_t)i);
  }

  Serial.println("Sistema listo!");
  Serial.println("Click corto en boton INICIO para comenzar");
  Serial.println("Click largo  en boton INICIO para reiniciar");
}

// ── Loop ─────────────────────────────────────────
void loop() {
  for (int i = 0; i < NUM_BOTONES; i++) {
    botones[i].tick();
  }
  verificarAudio();
}

// ── Verificar fin de audio ────────────────────────
void verificarAudio() {
  if (!reproduciendo || pausado) return;

  if (reproductor.available()) {
    if (reproductor.readType() == DFPlayerPlayFinished) {
      Serial.println("Audio terminado");
      reproduciendo = false;
      delay(300);
      reproducirSiguienteAudio();
    }
  }
}