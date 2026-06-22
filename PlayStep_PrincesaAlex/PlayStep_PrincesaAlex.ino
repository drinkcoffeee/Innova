/*
 * PlayStep - La Princesa Alex
 * Alfombra cuento interactiva
 * ESP32 + DFPlayer Mini + 9 anillos WS2812B (3 LEDs c/u, cascabel en pin 4)
 *
 * PINES:
 *   Botón inicio  → GPIO 13
 *   Botón pausa   → GPIO 22
 *   Cuadrado 1    → GPIO 21
 *   Cuadrado 2    → GPIO 32
 *   Cuadrado 3    → GPIO 14
 *   Cuadrado 4    → GPIO 19
 *   Cuadrado 5    → GPIO 33
 *   Cuadrado 6    → GPIO 27
 *   Cuadrado 7    → GPIO 18
 *   Cuadrado 8    → GPIO 25
 *   Cuadrado 9    → GPIO 26
 *   LEDs WS2812B  → GPIO 4
 *   DFPlayer RX   → GPIO 16
 *   DFPlayer TX   → GPIO 17
 *
 * NUMERACIÓN DE AUDIOS (carpeta 01 en SD):
 *   001.mp3  → Intro: "En un reino lejano..."
 *   002.mp3  → "Bien lo pudimos abrir! Tenemos dos formas de escapar..."
 *   003.mp3  → Bosque sabios: "El bosque está lleno de árboles..."
 *   004.mp3  → "¡Muy bien lograste salir del bosque! Ahora encuentras 2 caminos..."
 *   005.mp3  → Campo mariposas: "Las mariposas te ayudan..."
 *   006.mp3  → "¡Lograste salir con éxito! Al final del túnel encuentras el río..."
 *   007.mp3  → Campo flores: "Las flores se ven muy bonitas..."
 *   008.mp3  → "¡Lograste salir del laberinto! Encuentras el río de la felicidad..."
 *   009.mp3  → Atravesar río: "En el río hay un camino de piedras..."
 *   010.mp3  → "¡Lograste atravesar el río! Escuchamos a los lobos..."
 *   011.mp3  → Usar bote: "En la orilla hay un bote..."
 *   012.mp3  → "¡Perfecto! Llegaste a una cueva con dos caminos..."
 *   013.mp3  → Cabaña panadera: "Para entrar debemos abrir la puerta..."
 *   014.mp3  → "¡Lograste abrir la puerta! Escuchaste pasar a Salem..."
 *   015.mp3  → Escalar árbol: "Para escalar el árbol debes estirarte..."
 *   016.mp3  → "¡Lograste escalar el árbol! Escuchaste a Salem... ¿Qué haces?"
 *   017.mp3  → Seguir a Salem (bajar árbol): "Para bajar del árbol pon una mano..."
 *   018.mp3  → "¡Muy bien! Encontraste un mapa con 2 caminos..."
 *   019.mp3  → Buscar refugio: "Quieres salir pero la puerta no se puede abrir..."
 *   020.mp3  → "¡Muy bien! Encontraste las huellas de Salem..."
 *   021.mp3  → Camino rocoso: "Para atravesar esquivando piedras muévete rápido..."
 *   022.mp3  → "¡Qué velocidad! Salem creó dos caminos iguales ¿Cuál eliges?"
 *   023.mp3  → Camino oscuro: "Necesitamos luz, hay una linterna en el árbol..."
 *   024.mp3  → "Perfecto, Salem puso dos obstáculos al final..."
 *   025.mp3  → Arbustos (desde rocoso): "Para atravesar los arbustos agáchate..."
 *   026.mp3  → "¡Bien hecho! Ves pasar a Salem con un gato ¿Qué haces?"
 *   027.mp3  → Camino 1 (sin salida): "Encuentras a uno de los lobos de Salem..."
 *   028.mp3  → Camino 2 (portal): "Ves un portal mágico apagado..."
 *   029.mp3  → "¡Lograste encenderlo! ¿A cuál calabozo vamos?"
 *   030.mp3  → Calabozo 2 (falso): "Salem creó un calabozo falso..."
 *   031.mp3  → Calabozo 1 (real): "Ves a todos los animales en jaulas..."
 *   032.mp3  → "¡Liberaste a los animales! Llegó Salem, decisión final..."
 *   033.mp3  → Final hablar con Salem: "Salem dice que robó las mascotas porque..."
 *   034.mp3  → Final escapar: "Comienzas a correr pero Salem te explica..."
 *   035.mp3  → Final abrazo: "¡Felicitaciones! Moraleja: La amabilidad..."
 */

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <FastLED.h>

// ─── LEDs ───────────────────────────────────────────────────────────────────
#define LED_PIN       4
#define NUM_RINGS     9
#define LEDS_PER_RING 3
#define NUM_LEDS      (NUM_RINGS * LEDS_PER_RING)
CRGB leds[NUM_LEDS];

// ─── DFPlayer ───────────────────────────────────────────────────────────────
SoftwareSerial dfSerial(16, 17); // RX, TX
DFRobotDFPlayerMini dfPlayer;

// ─── BOTONES Y CUADRADOS ────────────────────────────────────────────────────
const int PIN_BTN_INICIO = 13;
const int PIN_BTN_PAUSA  = 22;
const int PIN_CUADRADO[10] = {
  0,    // índice 0 sin uso
  21,   // cuadrado 1
  32,   // cuadrado 2
  14,   // cuadrado 3
  19,   // cuadrado 4
  33,   // cuadrado 5
  27,   // cuadrado 6
  18,   // cuadrado 7
  25,   // cuadrado 8
  26    // cuadrado 9
};

// ─── COLORES ─────────────────────────────────────────────────────────────────
#define COLOR_OPCION1  CRGB(255, 120,   0)  // naranja – cuadrado opción izquierda
#define COLOR_OPCION2  CRGB(  0, 120, 255)  // azul    – cuadrado opción derecha
#define COLOR_SECUENCIA CRGB(200,   0, 200) // morado  – cuadrados de secuencia
#define COLOR_CORRECTO CRGB(  0, 255,   0)  // verde   – confirmación OK
#define COLOR_ERROR    CRGB(255,   0,   0)  // rojo    – error
#define COLOR_OFF      CRGB(  0,   0,   0)

// ─── TIPOS DE PASO ───────────────────────────────────────────────────────────
// CHOICE: el niño elige entre cuadrado A o B → bifurcación
// SEQUENCE: debe pisar una secuencia exacta en orden
// HOLD: debe mantener pisados varios cuadrados a la vez
// REPEAT: debe pisar el mismo cuadrado N veces

enum TipoPaso { CHOICE, SEQUENCE, HOLD, REPEAT };

struct PasoEscena {
  TipoPaso tipo;
  int cuadrados[9]; // cuadrados involucrados (0 = fin de lista)
  int repeticiones; // solo para REPEAT
  int siguienteA;   // escena si elige opción A (o secuencia correcta)
  int siguienteB;   // escena si elige opción B (-1 si no aplica)
  int audioNarrar;  // audio a reproducir AL INICIO de la escena
};

// ─── DEFINICIÓN DE ESCENAS ───────────────────────────────────────────────────
// Índices de escena:
//  0  → Intro (pisá 3 veces cuadrado 2)
//  1  → Bifurcación inicio: bosque(1) o cueva(3)
//  2  → Bosque: pisa 7 luego 9
//  3  → Bifurcación bosque: mariposas(4) o flores(6)
//  4  → Campo mariposas: secuencia 1,2,3,6
//  5  → Río desde mariposas: río(8) o bote(2)
//  6  → Campo flores: secuencia 2,6,8,4,1,3,7,9
//  7  → Río desde flores: río(8) o bote(2)
//  8  → Atravesar río: secuencia 1,2,3
//  9  → Bifurcación río: cabaña(4) o árbol(6)
// 10  → Usar bote: sube pisando 8, luego HOLD 1+3+7+9
// 11  → Bifurcación bote: dulces(4) o hadas(6)  [cueva dulces/hadas vacías → ir a 11b]
// 12  → Cabaña panadera: HOLD 4+6
// 13  → Bifurcación cabaña: seguir(9) o buscar(1)
// 14  → Escalar árbol: REPEAT 7 x3
// 15  → Bifurcación árbol: seguir(9) o buscar(1)
// 16  → Seguir Salem (bajar árbol): HOLD 1+6
// 17  → Bifurcación mapa: rocoso(2) o oscuro(8)
// 18  → Buscar refugio (ventana): pisa 6
// 19  → Bifurcación huellas: huellas(7) o camino(9)  [huellas/camino vacíos → fin]
// 20  → Camino rocoso: secuencia 2,4,6
// 21  → Bifurcación doble camino: camino1(1) o camino2(2)
// 22  → Camino oscuro: salta al 7
// 23  → Bifurcación oscuro: arbustos(1) o pantano(4)  [pantano vacío → arbustos]
// 24  → Arbustos: secuencia 3,6,9
// 25  → Bifurcación arbustos: seguir silencioso(1) o esperar(2)  [ambos vacíos → calabozo]
// 26  → Camino 1 (sin salida): pisa 2 → va a camino 2
// 27  → Camino 2 (portal): salta al 9
// 28  → Bifurcación calabozos: calabozo1(4) o calabozo2(6)
// 29  → Calabozo 2 (falso): pisa 4 → va a calabozo 1
// 30  → Calabozo 1: REPEAT 8 x3
// 31  → Decisión final: hablar(3) o escapar(6)
// 32  → Final hablar: pisa 1 (abrazo)
// 33  → Final escapar: pisa 1 (abrazo)
// 34  → FIN

#define NUM_ESCENAS 35

PasoEscena escenas[NUM_ESCENAS] = {
  // 0: Intro – pisar cuadrado 2 tres veces
  { REPEAT,    {2,0},     3,  1, -1,  1 },
  // 1: Bifurcación inicio
  { CHOICE,    {1,3,0},   0,  2,  1, 2 },
  //   (cueva de diamantes está vacía en el HTML → misma lógica que bosque por ahora, apunta a 2)
  // 2: Bosque – secuencia 7, 9
  { SEQUENCE,  {7,9,0},   0,  3, -1,  3 },
  // 3: Bifurcación bosque: mariposas(1) o flores(6)
  { CHOICE,    {4,6,0},   0,  4,  6,  4 },
  // 4: Campo mariposas – secuencia 1,2,3,6
  { SEQUENCE,  {1,2,3,6,0}, 0, 5, -1, 5 },
  // 5: Río (desde mariposas): río(8) o bote(2)
  { CHOICE,    {8,2,0},   0,  8, 10,  6 },
  // 6: Campo flores – secuencia 2,6,8,4,1,3,7,9
  { SEQUENCE,  {2,6,8,4,1,3,7,9,0}, 0, 7, -1, 7 },
  // 7: Río (desde flores): río(8) o bote(2)
  { CHOICE,    {8,2,0},   0,  8, 10,  8 },
  // 8: Atravesar río – secuencia 1,2,3
  { SEQUENCE,  {1,2,3,0}, 0,  9, -1,  9 },
  // 9: Bifurcación río: cabaña(4) o árbol(6)
  { CHOICE,    {4,6,0},   0, 12, 14, 10 },
  // 10: Bote – primero sube pisando 8, luego HOLD 1+3+7+9
  //     (simplificamos: SEQUENCE 8 → luego HOLD en escena 10b)
  { SEQUENCE,  {8,0},     0, 10, -1, 11 },
  // 10b (índice 10 ocupado): esta escena maneja el HOLD 1+3+7+9
  // NOTA: se implementa como HOLD en loop; siguienteA=11
  { HOLD,      {1,3,7,9,0}, 0, 11, -1, -1 },  // audio ya reproducido en escena 10
  // 11: Bifurcación bote: dulces(4) o hadas(6) → ambas vacías en HTML, redirigir a CHOICE
  //     por simplicidad llevamos a calabozo path (mismo que caminos medio)
  { CHOICE,    {4,6,0},   0, 11, 11, 12 }, // cueva dulces/hadas vacías → mismo nodo (placeholder)
  // 12: Cabaña – HOLD manos en 4 y pies en 6
  { HOLD,      {4,6,0},   0, 13, -1, 13 },
  // 13: Bifurcación cabaña: seguir(9) o buscar(1)
  { CHOICE,    {9,1,0},   0, 16, 18, 14 },
  // 14: Escalar árbol – REPEAT cuadrado 7 x3
  { REPEAT,    {7,0},     3, 15, -1, 15 },
  // 15: Bifurcación árbol: seguir(9) o buscar(1)
  { CHOICE,    {9,1,0},   0, 16, 18, 16 },
  // 16: Seguir Salem (bajar árbol) – HOLD 1+6
  { HOLD,      {1,6,0},   0, 17, -1, 17 },
  // 17: Bifurcación mapa: rocoso(2) o oscuro(8)
  { CHOICE,    {2,8,0},   0, 20, 22, 18 },
  // 18: Buscar refugio – ventana, pisa 6
  { SEQUENCE,  {6,0},     0, 19, -1, 19 },
  // 19: Bifurcación huellas: seguir huellas(7) o camino original(9) → ambas vacías → placeholder
  { CHOICE,    {7,9,0},   0, 34, 34, 20 }, // fin si está vacío
  // 20: Camino rocoso – secuencia 2,4,6
  { SEQUENCE,  {2,4,6,0}, 0, 21, -1, 21 },
  // 21: Bifurcación doble camino: camino1(1) o camino2(2)
  { CHOICE,    {1,2,0},   0, 26, 27, 22 },
  // 22: Camino oscuro – salta al cuadrado 7
  { SEQUENCE,  {7,0},     0, 23, -1, 23 },
  // 23: Bifurcación oscuro: arbustos(1) o pantano(4) → pantano vacío → arbustos en ambos
  { CHOICE,    {1,4,0},   0, 24, 24, 24 },
  // 24: Arbustos – secuencia 3,6,9
  { SEQUENCE,  {3,6,9,0}, 0, 25, -1, 25 },
  // 25: Bifurcación arbustos: seguir silencioso(1) o esperar huellas(2) → ambas vacías → calabozo
  { CHOICE,    {1,2,0},   0, 28, 28, 26 },
  // 26: Camino 1 (sin salida) – lobo dice "ve al 2" → pisa 2
  { SEQUENCE,  {2,0},     0, 27, -1, 27 },
  // 27: Camino 2 (portal) – salta al 9
  { SEQUENCE,  {9,0},     0, 28, -1, 28 },
  // 28: Bifurcación calabozos: calabozo1(4) o calabozo2(6)
  { CHOICE,    {4,6,0},   0, 30, 29, 29 },
  // 29: Calabozo 2 (falso) – pisa 4 → ir al real
  { SEQUENCE,  {4,0},     0, 30, -1, 30 },
  // 30: Calabozo 1 real – REPEAT 8 x3
  { REPEAT,    {8,0},     3, 31, -1, 31 },
  // 31: Decisión final: hablar con Salem(3) o escapar(6)
  { CHOICE,    {3,6,0},   0, 32, 33, 32 },
  // 32: Final hablar – pisa 1 (abrazo)
  { SEQUENCE,  {1,0},     0, 34, -1, 33 },
  // 33: Final escapar – pisa 1 (abrazo)
  { SEQUENCE,  {1,0},     0, 34, -1, 34 },
  // 34: FIN
  { SEQUENCE,  {0},       0, 34, -1, 35 }
};

// ─── ESTADO GLOBAL ───────────────────────────────────────────────────────────
enum EstadoMaquina { ESPERAR_INICIO, REPRODUCIENDO, ESPERANDO_INPUT, FIN_CUENTO };
EstadoMaquina estado = ESPERAR_INICIO;

int escenaActual = 0;
int pasoSecuencia = 0;       // índice dentro de la secuencia actual
int contadorRepeat = 0;      // contador para REPEAT
bool pausado = false;

unsigned long tiempoUltimoBoton = 0;
const unsigned long DEBOUNCE_MS = 300;
unsigned long tiempoInicioHold = 0;
const unsigned long HOLD_MS = 2000; // tiempo mínimo de hold (ms)

// ─── PROTOTIPOS ──────────────────────────────────────────────────────────────
void reproducirAudio(int num);
bool audioTerminado();
void apagarTodos();
void iluminarCuadrado(int cuad, CRGB color);
void iluminarCuadrados(int* cuads, CRGB color);
void flashCorrecto();
void flashError();
bool botonPresionado(int cuad);
bool botonSuelto(int cuad);
void iniciarEscena(int idx);
void manejarInput();
void manejarChoice(int cuadPresionado);
void manejarSequence(int cuadPresionado);
void manejarRepeat(int cuadPresionado);
void manejarHold();
void irAEscena(int idx);

// ─── SETUP ───────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  // Botones
  pinMode(PIN_BTN_INICIO, INPUT_PULLUP);
  pinMode(PIN_BTN_PAUSA,  INPUT_PULLUP);
  for (int i = 1; i <= 9; i++) {
    pinMode(PIN_CUADRADO[i], INPUT_PULLUP);
  }

  // LEDs
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(150);
  apagarTodos();

  // DFPlayer
  dfSerial.begin(9600);
  delay(1000);
  if (!dfPlayer.begin(dfSerial)) {
    Serial.println("DFPlayer no encontrado. Verificar conexiones.");
    while (true); // halt
  }
  dfPlayer.volume(25); // 0-30
  delay(200);

  Serial.println("PlayStep listo. Esperando inicio...");

  // Animación de bienvenida
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(255, 200, 0);
  }
  FastLED.show();
  delay(800);
  apagarTodos();
}

// ─── LOOP ────────────────────────────────────────────────────────────────────
void loop() {
  // Botón pausa en cualquier momento
  if (digitalRead(PIN_BTN_PAUSA) == LOW) {
    delay(50);
    if (digitalRead(PIN_BTN_PAUSA) == LOW) {
      pausado = !pausado;
      if (pausado) {
        dfPlayer.pause();
        Serial.println("PAUSADO");
      } else {
        dfPlayer.start();
        Serial.println("REANUDADO");
      }
      delay(500);
    }
  }
  if (pausado) return;

  switch (estado) {
    case ESPERAR_INICIO:
      // Cuadrado 2 parpadea esperando
      iluminarCuadrado(2, CRGB(0, 200, 200));
      if (digitalRead(PIN_BTN_INICIO) == LOW || digitalRead(PIN_CUADRADO[2]) == LOW) {
        delay(50);
        apagarTodos();
        delay(200);
        iniciarEscena(0);
      }
      break;

    case REPRODUCIENDO:
      if (audioTerminado()) {
        estado = ESPERANDO_INPUT;
        // Iluminar cuadrados según tipo
        PasoEscena& e = escenas[escenaActual];
        if (e.tipo == CHOICE) {
          iluminarCuadrado(e.cuadrados[0], COLOR_OPCION1);
          iluminarCuadrado(e.cuadrados[1], COLOR_OPCION2);
        } else if (e.tipo == SEQUENCE || e.tipo == REPEAT) {
          // Iluminar solo el primer cuadrado esperado
          iluminarCuadrado(e.cuadrados[0], COLOR_SECUENCIA);
        } else if (e.tipo == HOLD) {
          for (int i = 0; e.cuadrados[i] != 0; i++) {
            iluminarCuadrado(e.cuadrados[i], COLOR_SECUENCIA);
          }
          tiempoInicioHold = 0;
        }
      }
      break;

    case ESPERANDO_INPUT:
      manejarInput();
      break;

    case FIN_CUENTO:
      // Festejo final
      for (int rep = 0; rep < 5; rep++) {
        for (int i = 0; i < NUM_LEDS; i++) leds[i] = CRGB(random(256), random(256), random(256));
        FastLED.show();
        delay(300);
      }
      apagarTodos();
      delay(3000);
      estado = ESPERAR_INICIO;
      break;
  }
}

// ─── FUNCIONES ───────────────────────────────────────────────────────────────

void iniciarEscena(int idx) {
  escenaActual = idx;
  pasoSecuencia = 0;
  contadorRepeat = 0;
  tiempoInicioHold = 0;
  apagarTodos();

  PasoEscena& e = escenas[idx];

  if (idx == 34) { // FIN
    reproducirAudio(35);
    estado = FIN_CUENTO;
    return;
  }

  if (e.audioNarrar > 0) {
    reproducirAudio(e.audioNarrar);
    estado = REPRODUCIENDO;
  } else {
    estado = ESPERANDO_INPUT;
  }

  Serial.print("Escena: "); Serial.println(idx);
}

void irAEscena(int idx) {
  flashCorrecto();
  delay(600);
  apagarTodos();
  delay(400);
  iniciarEscena(idx);
}

void manejarInput() {
  PasoEscena& e = escenas[escenaActual];

  for (int i = 1; i <= 9; i++) {
    if (botonPresionado(i)) {
      switch (e.tipo) {
        case CHOICE:    manejarChoice(i);    break;
        case SEQUENCE:  manejarSequence(i);  break;
        case REPEAT:    manejarRepeat(i);    break;
        default: break;
      }
      return;
    }
  }

  if (e.tipo == HOLD) {
    manejarHold();
  }
}

void manejarChoice(int cuadPresionado) {
  PasoEscena& e = escenas[escenaActual];
  if (cuadPresionado == e.cuadrados[0]) {
    irAEscena(e.siguienteA);
  } else if (cuadPresionado == e.cuadrados[1]) {
    irAEscena(e.siguienteB);
  } else {
    flashError();
  }
}

void manejarSequence(int cuadPresionado) {
  PasoEscena& e = escenas[escenaActual];
  int esperado = e.cuadrados[pasoSecuencia];

  if (cuadPresionado == esperado) {
    iluminarCuadrado(esperado, COLOR_CORRECTO);
    delay(200);
    pasoSecuencia++;

    // ¿Terminó la secuencia?
    if (e.cuadrados[pasoSecuencia] == 0) {
      irAEscena(e.siguienteA);
    } else {
      // Siguiente cuadrado de la secuencia
      apagarTodos();
      iluminarCuadrado(e.cuadrados[pasoSecuencia], COLOR_SECUENCIA);
    }
  } else {
    flashError();
    // Reiniciar secuencia
    pasoSecuencia = 0;
    apagarTodos();
    iluminarCuadrado(e.cuadrados[0], COLOR_SECUENCIA);
  }
}

void manejarRepeat(int cuadPresionado) {
  PasoEscena& e = escenas[escenaActual];
  if (cuadPresionado == e.cuadrados[0]) {
    contadorRepeat++;
    iluminarCuadrado(cuadPresionado, COLOR_CORRECTO);
    delay(300);
    apagarTodos();
    delay(200);

    if (contadorRepeat >= e.repeticiones) {
      irAEscena(e.siguienteA);
    } else {
      iluminarCuadrado(e.cuadrados[0], COLOR_SECUENCIA);
    }
  } else {
    flashError();
    contadorRepeat = 0;
    iluminarCuadrado(e.cuadrados[0], COLOR_SECUENCIA);
  }
}

void manejarHold() {
  PasoEscena& e = escenas[escenaActual];

  // Verificar que todos los cuadrados requeridos estén presionados
  bool todosPresionados = true;
  for (int i = 0; e.cuadrados[i] != 0; i++) {
    if (digitalRead(PIN_CUADRADO[e.cuadrados[i]]) != LOW) {
      todosPresionados = false;
      break;
    }
  }

  if (todosPresionados) {
    if (tiempoInicioHold == 0) {
      tiempoInicioHold = millis();
      // Cambiar color a verde mientras mantiene
      for (int i = 0; e.cuadrados[i] != 0; i++) {
        iluminarCuadrado(e.cuadrados[i], COLOR_CORRECTO);
      }
    } else if (millis() - tiempoInicioHold >= HOLD_MS) {
      irAEscena(e.siguienteA);
    }
  } else {
    if (tiempoInicioHold != 0) {
      tiempoInicioHold = 0;
      // Volver a color indicador
      for (int i = 0; e.cuadrados[i] != 0; i++) {
        iluminarCuadrado(e.cuadrados[i], COLOR_SECUENCIA);
      }
    }
  }
}

// ─── AUDIO ───────────────────────────────────────────────────────────────────
void reproducirAudio(int num) {
  dfPlayer.play(num);
  delay(200);
  Serial.print("Reproduciendo audio: "); Serial.println(num);
}

bool audioTerminado() {
  if (dfPlayer.available()) {
    uint8_t tipo = dfPlayer.readType();
    if (tipo == DFPlayerPlayFinished) return true;
  }
  return false;
}

// ─── LEDs ────────────────────────────────────────────────────────────────────
// Mapeo cuadrado 1-9 → índice del primer LED del anillo
// Orden físico: cuadrados 1-9 en cascada
int cuadradoAIndexLED(int cuad) {
  return (cuad - 1) * LEDS_PER_RING;
}

void iluminarCuadrado(int cuad, CRGB color) {
  if (cuad < 1 || cuad > 9) return;
  int base = cuadradoAIndexLED(cuad);
  for (int i = 0; i < LEDS_PER_RING; i++) {
    leds[base + i] = color;
  }
  FastLED.show();
}

void apagarTodos() {
  fill_solid(leds, NUM_LEDS, COLOR_OFF);
  FastLED.show();
}

void flashCorrecto() {
  fill_solid(leds, NUM_LEDS, COLOR_CORRECTO);
  FastLED.show();
  delay(400);
  apagarTodos();
  delay(200);
  fill_solid(leds, NUM_LEDS, COLOR_CORRECTO);
  FastLED.show();
  delay(400);
  apagarTodos();
}

void flashError() {
  fill_solid(leds, NUM_LEDS, COLOR_ERROR);
  FastLED.show();
  delay(500);
  apagarTodos();
  delay(300);
}

// ─── DEBOUNCE BOTONES ────────────────────────────────────────────────────────
bool botonPresionado(int cuad) {
  if (digitalRead(PIN_CUADRADO[cuad]) == LOW) {
    unsigned long ahora = millis();
    if (ahora - tiempoUltimoBoton > DEBOUNCE_MS) {
      tiempoUltimoBoton = ahora;
      // Esperar a que se suelte para no disparar doble
      while (digitalRead(PIN_CUADRADO[cuad]) == LOW) delay(10);
      return true;
    }
  }
  return false;
}
