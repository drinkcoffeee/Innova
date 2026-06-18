#include "DFRobotDFPlayerMini.h"
#include <SoftwareSerial.h>

SoftwareSerial serial_mp3(16, 17); // RX, TX
DFRobotDFPlayerMini reproductor;

const int botones[11] = {
  13, 22,   //1 y 2
  14, 32, 21, // 3, 4, 5
  27, 33, 19, //6, 7 , 8
  26, 25, 18 // 9, 10, 11
}; 

int estado[11];

void iniciarBotones() {
  for (int i = 0; i < 11; i++) {
    pinMode(botones[i], INPUT_PULLUP);
    estado[i] = true;
  }
}

void actualizarBotones() {
  for (int i = 0; i < 11; i++) {
    int lectura = digitalRead(botones[i]);
    estado[i] = lectura;
  }
}

void setup(){
  Serial.begin(115200);
  serial_mp3.begin(9600);
  iniciarBotones();
  if (!reproductor.begin(serial_mp3)) {
    Serial.println("Error: no se encontró el DFPlayer");
    while (true); // se queda aquí si falla
  }

  reproductor.volume(20);       // volumen 0-30
  reproductor.play(1);          // reproduce 0001.mp3

}



void loop(){
  actualizarBotones();
  for (int i = 0; i < 11; i++) {
  Serial.print(estado[0]); Serial.print("-"); Serial.println(estado[1]); 
  Serial.print(estado[2]); Serial.print(estado[3]); Serial.println(estado[4]); 
  Serial.print(estado[5]); Serial.print(estado[6]); Serial.println(estado[7]); 
  Serial.print(estado[8]); Serial.print(estado[9]); Serial.println(estado[10]); 
  delay(500);
  }

  for (int i = 0 ; i < 11; i++){
    if( estado[i] == 0 ){
        int leer1 = estado[i];
        delay(50);
        int leer2= estado[i];
        if (leer1 == leer2){
          reproductor.play(2)
          delay(500);
          Serial.print("Boton");
          Serial.print("\n");
          Serial.print(i+1 );
          Serial.print("\n");
          Serial.print("Presionado");
          Serial.print("\n");
        
        } else{ 
          Serial.print("Soltaste boton antes");
        
        }
    }
  }

}