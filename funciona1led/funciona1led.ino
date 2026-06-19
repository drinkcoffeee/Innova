#include <FastLED.h>
#include "DFRobotDFPlayerMini.h"
#include <HardwareSerial.h>
#include <OneButton.h>

// ── LEDs ────────────────────────────────────────
#define NUM_ANILLOS 9
#define LEDS_POR_ANILLO 3  // cambia si tu anillo tiene más LEDs

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
#define NUM_BOTONES 11

const int pinesBotones[NUM_BOTONES] = {
  13, 22,
  14, 32, 21,
  27, 33, 26,
  25, 34, 35
};

OneButton botones[NUM_BOTONES] = {
  OneButton(pinesBotones[0],  true),
  OneButton(pinesBotones[1],  true),
  OneButton(pinesBotones[2],  true),
  OneButton(pinesBotones[3],  true),
  OneButton(pinesBotones[4],  true),
  OneButton(pinesBotones[5],  true),
  OneButton(pinesBotones[6],  true),
  OneButton(pinesBotones[7],  true),
  OneButton(pinesBotones[8],  true),
  OneButton(pinesBotones[9],  true, false),
  // true = activo en LOW, false = NO usar pull-up interno
  OneButton(pinesBotones[10], true,false)
};

// ── Secuencia del cuento ─────────────────────────
// Cada paso tiene: audios[], cuadrados[], correcto
// Los cuadrados y correcto usan índice 0 (cuadrado 1 = índice 0)

struct Paso {
  int audios[4];      // hasta 4 audios por paso
  int numAudios;
  int cuadrados[2];   // hasta 2 cuadrados por paso
  int numCuadrados;
  int correcto;       // índice 0 del cuadrado correcto
};

const int NUM_PASOS = 11;

Paso secuencia[NUM_PASOS] = {
  // Paso 0: audios 1,2,3 — cuadrados 1,3 — correcto 1
  { {1,2,3,0}, 3, {0,2}, 2, 0 },

  // Paso 1: audios 4,5,6 — cuadrados 1,2 — correcto 1
  { {4,5,6,0}, 3, {0,1}, 2, 0 },

  // Paso 2: audios 7,8,9 — cuadrados 6,4 — correcto 6
  { {7,8,9,0}, 3, {5,3}, 2, 5 },

  // Paso 3: audios 10,11,12 — cuadrados 8,2 — correcto 8
  { {10,11,12,0}, 3, {7,1}, 2, 7 },

  // Paso 4: audios 13,14,15 — cuadrados 6,4 — correcto 6
  { {13,14,15,0}, 3, {5,3}, 2, 5 },

  // Paso 5: audios 16,17,18 — cuadrados 9,1 — correcto 9
  { {16,17,18,0}, 3, {8,0}, 2, 8 },

  // Paso 6: audios 19,20,21 — cuadrados 8,2 — correcto 2
  { {19,20,21,0}, 3, {7,1}, 2, 1 },

  // Paso 7: audios 22,23,24 — cuadrados 1,2 — correcto 2
  { {22,23,24,0}, 3, {0,1}, 2, 1 },

  // Paso 8: audios 25,26,27 — cuadrados 4,6 — correcto 4
  { {25,26,27,0}, 3, {3,5}, 2, 3 },

  // Paso 9: audios 28,29,30 — cuadrados 3,6 — correcto 3
  { {28,29,30,0}, 3, {2,5}, 2, 2 },

  // Paso 10 (fin): audios 31,32 — sin cuadrados
  { {31,32,0,0}, 2, {-1,-1}, 0, -1 }
};

// ── Estado ───────────────────────────────────────
int pasoActual = 0;
int audioActual = 0;       // índice dentro de los audios del paso
bool reproduciendo = false; // true mientras suena un audio
bool esperandoBoton = false;

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
    // Todos los audios del paso terminaron
    reproduciendo = false;

    if (p.numCuadrados == 0) {
      // Es el paso final — cuento terminado
      Serial.println("Fin del cuento!");
      for (int i = 0; i < NUM_ANILLOS; i++) {
        encenderAnillo(i, colores[i]);
        delay(150);
      }
      delay(3000);
      apagarTodos();
      return;
    }

    // Encender los cuadrados del paso
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
  audioActual = 0;
  esperandoBoton = false;

  if (pasoActual >= NUM_PASOS) {
    pasoActual = 0; // reinicia el cuento
  }

  delay(800);
  reproducirSiguienteAudio();
}

// ── Callback botones ─────────────────────────────
void alPresionar(void* indice) {
  int i = (int)(intptr_t)indice;

  // Botón 10 → volumen +
  if (i == 9) {
    reproductor.volumeUp();
    Serial.println("Volumen +");
    return;
  }

  // Botón 11 → volumen -
  if (i == 10) {
    reproductor.volumeDown();
    Serial.println("Volumen -");
    return;
  }

  // Si no espera botón, ignorar
  if (!esperandoBoton) return;

  Paso& p = secuencia[pasoActual];

  if (i == p.correcto) {
    // ✅ Correcto — apagar todos y avanzar
    Serial.print("Correcto! Pisaste el cuadrado ");
    Serial.println(i + 1);
    apagarTodos();
    delay(500);
    esperandoBoton = false;
    siguientePaso();
  } else {
    // Verificar si pisó alguno de los cuadrados del paso
    bool esCuadradoDelPaso = false;
    for (int c = 0; c < p.numCuadrados; c++) {
      if (p.cuadrados[c] == i) {
        esCuadradoDelPaso = true;
        break;
      }
    }

    if (esCuadradoDelPaso) {
      // ❌ Pisó el incorrecto — parpadea en rojo
      Serial.print("Incorrecto! Era el cuadrado ");
      Serial.println(p.correcto + 1);
      encenderAnillo(i, CRGB::Red);
      delay(500);
      encenderAnillo(i, colores[i]); // vuelve a su color
    }
    // Si pisó un cuadrado que no es del paso, ignorar
  }
}

// ── Detectar fin de audio (loop) ──────────────────
void verificarAudio() {
  if (!reproduciendo) return;

  if (reproductor.available()) {
    if (reproductor.readType() == DFPlayerPlayFinished) {
      Serial.println("Audio terminado");
      reproduciendo = false;
      delay(300);
      reproducirSiguienteAudio(); // reproduce el siguiente del paso
    }
  }
}

// ── Setup ────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(2000); // aumenta el delay para ver el mensaje
  Serial.println("Iniciando...");

  // Iniciar LEDs — comenta GPIO0 y GPIO2 temporalmente
  FastLED.addLeds<WS2812B, 4,  GRB>(leds[0], LEDS_POR_ANILLO);
  FastLED.addLeds<WS2812B, 5,  GRB>(leds[1], LEDS_POR_ANILLO);
  FastLED.addLeds<WS2812B, 15, GRB>(leds[2], LEDS_POR_ANILLO);
  FastLED.addLeds<WS2812B, 23, GRB>(leds[3], LEDS_POR_ANILLO);
  // FastLED.addLeds<WS2812B, 2,  GRB>(leds[4], LEDS_POR_ANILLO); // ← comenta
  // FastLED.addLeds<WS2812B, 0,  GRB>(leds[5], LEDS_POR_ANILLO); // ← comenta
  FastLED.addLeds<WS2812B, 18, GRB>(leds[6], LEDS_POR_ANILLO);
  FastLED.addLeds<WS2812B, 19, GRB>(leds[7], LEDS_POR_ANILLO);
  FastLED.addLeds<WS2812B, 12, GRB>(leds[8], LEDS_POR_ANILLO);
  
  Serial.println("LEDs OK!");
  

  // Iniciar DFPlayer
  serial_mp3.begin(9600, SERIAL_8N1, 16, 17);
  delay(1000);

  if (!reproductor.begin(serial_mp3)) {
    Serial.println("Error: no se encontro el DFPlayer");
    while (true);
  }
  reproductor.volume(20);
  Serial.println("DFPlayer OK!");

  // Asignar callbacks
  for (int i = 0; i < NUM_BOTONES; i++) {
    botones[i].attachClick(alPresionar, (void*)(intptr_t)i);
  }

  Serial.println("Iniciando cuento...");
  delay(1000);
  reproducirSiguienteAudio(); // arranca con el audio 1
}

// ── Loop ─────────────────────────────────────────
void loop() {
  // Escuchar botones
  for (int i = 0; i < NUM_BOTONES; i++) {
    botones[i].tick();
  }

  // Verificar si terminó un audio para reproducir el siguiente
  verificarAudio();
}