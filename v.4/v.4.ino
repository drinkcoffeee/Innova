/*
 * ── RESUMEN DE CONEXIONES (ESP32) ─────────────────────────────
 * NEOPIXELS (Cascada): Pin 4 (Va al DI del Anillo 1)
 * DFPLAYER MINI:       RX al Pin 17 (TX2) | TX al Pin 16 (RX2)
 * BOTONES (A GND):     Inicio(13), Pausa(22)
 * CUADRADOS 1 al 9:    14, 32, 21, 27, 33, 19, 26, 25, 18
 * ──────────────────────────────────────────────────────────────
 */

#include <Adafruit_NeoPixel.h>
#include "DFRobotDFPlayerMini.h"
#include <HardwareSerial.h>
#include <OneButton.h>

// ── CONFIGURACIÓN NEOPIXEL EN CASCADA ────────────
#define PIN_NEOPIXEL     4   // Un solo pin para todos los anillos
#define LEDS_POR_ANILLO  3
#define NUM_ANILLOS      9
#define TOTAL_LEDS       (LEDS_POR_ANILLO * NUM_ANILLOS) // 27 LEDs en total

Adafruit_NeoPixel tiraTotal = Adafruit_NeoPixel(TOTAL_LEDS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

uint32_t colores[NUM_ANILLOS];

// ── CONFIGURACIÓN DFPLAYER Y BOTONES ──────────────
HardwareSerial serial_mp3(2); 
DFRobotDFPlayerMini reproductor;

#define NUM_BOTONES 11

const int pines[NUM_BOTONES] = {
  13, 22,       // botón 1, 2 (Inicio/reinicio , Pausa/Reanudación)
  14, 32, 21,   // botón 3, 4, 5 (Cuadrados 1, 2, 3)
  27, 33, 19,   // botón 6, 7, 8 (Cuadrados 4, 5, 6)
  26, 25, 18    // botón 9, 10, 11 (Cuadrados 7, 8, 9)
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

// ── SECUENCIA DEL CUENTO ─────────────────────────
struct Paso {
  int audios[4];
  int numAudios;
  int cuadrados[2];  // Índices del 0 al 8 (Cuadrados 1 al 9)
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
  { {31,32,0,0},  2, {-1,-1}, 0, -1} // paso 10: fin del cuento
};

// ── ESTADO LÓGICO ────────────────────────────────
int  pasoActual      = 0;
int  audioActual     = 0;
bool reproduciendo   = false;
bool esperandoBoton  = false;
bool cuentoIniciado  = false;

// ── FUNCIONES AUXILIARES NEOPIXEL ─────────────────
void encenderAnillo(int anillo, uint32_t color) {
  if (anillo >= 0 && anillo < NUM_ANILLOS) {
    int pixelInicio = anillo * LEDS_POR_ANILLO;
    for (int i = 0; i < LEDS_POR_ANILLO; i++) {
      tiraTotal.setPixelColor(pixelInicio + i, color);
    }
    tiraTotal.show();
  }
}

void apagarTodos() {
  tiraTotal.clear();
  tiraTotal.show();
}

void destelloColor(uint32_t color) {
  tiraTotal.fill(color);
  tiraTotal.show();
  delay(300);
  apagarTodos();
}

void mostrarLucesPaso() {
  apagarTodos();
  Paso& p = secuencia[pasoActual];
  for (int c = 0; c < p.numCuadrados; c++) {
    int idxAnillo = p.cuadrados[c];
    if(idxAnillo >= 0 && idxAnillo < NUM_ANILLOS) {
      int pixelInicio = idxAnillo * LEDS_POR_ANILLO;
      for (int i = 0; i < LEDS_POR_ANILLO; i++) {
        tiraTotal.setPixelColor(pixelInicio + i, colores[idxAnillo]);
      }
    }
  }
  tiraTotal.show();
}

// ── CONTROL DEL FLUJO DE AUDIO/JUEGO ──────────────
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
      cuentoIniciado = false;
      apagarTodos();
      Serial.println("Presiona boton 1 para jugar de nuevo");
      return;
    }

    Serial.print("Pisa el cuadrado: ");
    for (int c = 0; c < p.numCuadrados; c++) {
      Serial.print(p.cuadrados[c] + 1);
      Serial.print(" ");
    }
    Serial.println();
    Serial.print("Correcto: ");
    Serial.println(p.correcto + 1);

    mostrarLucesPaso(); 
    esperandoBoton = true;
  }
}

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

// ── CALLBACK DE BOTONES ───────────────────────────
void alPresionar(void* indice) {
  int i = (int)(intptr_t)indice;

  if (i == 0) {
    Serial.println("Iniciando cuento!");
    apagarTodos();
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

  if (!cuentoIniciado) return;
  if (!esperandoBoton) return;

  int cuadrado = i - 2;
  Paso& p = secuencia[pasoActual];

  if (cuadrado == p.correcto) {
    Serial.print("Correcto! Pisaste cuadrado ");
    Serial.println(cuadrado + 1);
    esperandoBoton = false;
    
    destelloColor(tiraTotal.Color(0, 255, 0)); // Destello verde general
    
    delay(500);
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
      Serial.print("Incorrecto! Era el cuadrado ");
      Serial.println(p.correcto + 1);
      
      destelloColor(tiraTotal.Color(255, 0, 0)); // Destello rojo general
      delay(200);
      mostrarLucesPaso(); 
    }
  }
}

// ── SETUP ARDUINO ─────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(1000);

  // Inicializar NeoPixels
  tiraTotal.begin();
  tiraTotal.setBrightness(80); // Nivel de brillo ajustado (0-255)
  tiraTotal.clear();
  tiraTotal.show();

  // Configuración de la paleta de colores para cada anillo
  colores[0] = tiraTotal.Color(255, 0,   0);    // rojo
  colores[1] = tiraTotal.Color(0,   0,   255);  // azul
  colores[2] = tiraTotal.Color(0,   255, 0);    // verde
  colores[3] = tiraTotal.Color(255, 255, 0);    // amarillo
  colores[4] = tiraTotal.Color(128, 0,   128);  // morado
  colores[5] = tiraTotal.Color(255, 165, 0);    // naranjo
  colores[6] = tiraTotal.Color(0,   255, 255);  // cyan
  colores[7] = tiraTotal.Color(255, 215, 0);    // dorado
  colores[8] = tiraTotal.Color(255, 255, 255);  // blanco

  // Inicializar Comunicación Serial MP3
  serial_mp3.begin(9600, SERIAL_8N1, 16, 17);
  delay(1000);

  if (!reproductor.begin(serial_mp3)) {
    Serial.println("Error: no se encontro el DFPlayer");
    while (true); // Se detiene aquí si no encuentra el módulo
  }

  reproductor.volume(0);
  delay(1000);
  reproductor.volume(20);
  Serial.println("DFPlayer OK!");

  // Inicializar Botones
  for (int i = 0; i < NUM_BOTONES; i++) {
    botones[i].attachClick(alPresionar, (void*)(intptr_t)i);
  }

  Serial.println("Sistema listo!");
  Serial.println("Presiona boton 1 para iniciar el cuento");
}

// ── LOOP ARDUINO ──────────────────────────────────
void loop() {
  for (int i = 0; i < NUM_BOTONES; i++) {
    botones[i].tick();
  }
  verificarAudio();
}