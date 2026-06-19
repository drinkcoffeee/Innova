#include "DFRobotDFPlayerMini.h"
#include <HardwareSerial.h>
#include <OneButton.h>

HardwareSerial serial_mp3(2);
DFRobotDFPlayerMini reproductor;

#define NUM_BOTONES 11

const int pines[NUM_BOTONES] = {
  13, 22,       // botón 1, 2  naranjo y naranjo 
  14, 32, 21,   // botón 3, 4, 5     amarrollo,blanco, caca
  27, 33, 19,   // botón 6, 7, 8    gris ,azul,  cafe
  26, 25, 18    // botón 9, 10, 11   morado,verde, negro 

  derecha gnd 
  medio vcc
  abajo pin
};

// Crear un objeto OneButton por cada botón
// true = activo en LOW (INPUT_PULLUP)
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

// ── Callback genérico — se llama con el índice del botón ──
void alPresionar(void* indice) {
  int i = (int)(intptr_t)indice;
  
  Serial.print("Boton ");
  Serial.print(i + 1);
  Serial.println(" presionado");

  // Reproduce el MP3 correspondiente al botón
  // botón 1 → 0001.mp3, botón 2 → 0002.mp3, etc.
  reproductor.play(i + 1);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Iniciar DFPlayer
  serial_mp3.begin(9600, SERIAL_8N1, 16, 17);
  delay(1000);

  if (!reproductor.begin(serial_mp3)) {
    Serial.println("Error: no se encontro el DFPlayer");
    while (true);
  }

  reproductor.volume(20);
  Serial.println("DFPlayer OK!");

  // Asignar callback a cada botón pasando su índice
  for (int i = 0; i < NUM_BOTONES; i++) {
    botones[i].attachClick(alPresionar, (void*)(intptr_t)i);
    
    // Opcional: también puedes detectar si lo mantienen presionado
    // botones[i].attachLongPressStart(alMantener, (void*)(intptr_t)i);
  }
}

void loop() {
  // OneButton necesita tick() en cada loop para funcionar
  for (int i = 0; i < NUM_BOTONES; i++) {
    botones[i].tick();
  }
}