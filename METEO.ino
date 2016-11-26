/* Program do Stacji METEO

Analogi:
A0- kierunek wiatru
A1- solar_1
A2- solar_2
A3- turbina
A4- uv
A5- aku

Liczniki:
2-int0-deszcz
3-int1-wiatr

OneWire:
8 -DS1-Slonce
9 -DS2-Grunt
10-DS3-10cm
11-DS4-25cm
12-DS5-50cm
13-DS6-100cm

SOFT I2C:
13-SDA
12-SCL
BME280, BH1750 i MOD-1016(AS3935)

ESP:
0,1-komunikacja 115200
4-reset esp

*/
#include <TimerOne.h>

const byte INT_pomiar_deszczu  = 0;
const byte INT_predkosc_wiatru = 1;
const byte PIN_pomiar_deszczu  = 2;
const byte PIN_predkosc_wiatru = 3;
const byte PIN_reset_esp       = 4;

float pomiary[20];

volatile uint8_t deszcz_suma_impulsow = 0;
volatile unsigned long deszcz_czas, deszcz_czas_ostatni;

void IRQ_deszcz() {
  deszcz_czas = millis();
  unsigned long deszcz_interwal = deszcz_czas - deszcz_czas_ostatni;

  if (deszcz_interwal > 10) { // pomijamy czas <= 10ms.
    deszcz_czas_ostatni = deszcz_czas;
    deszcz_suma_impulsow++;
  }
}

volatile unsigned long wiatr_czas, wiatr_czas_ostatni;
volatile uint8_t wiatr_suma_impulsow = 0, wiatr_max_suma_impulsow = 0;

void IRQ_wiatr() {
  wiatr_czas = millis();
  unsigned long wiatr_interwal = wiatr_czas - wiatr_czas_ostatni;

  if (wiatr_interwal > 10) { // pomijamy czas <= 10ms.
    wiatr_czas_ostatni = deszcz_czas;
    wiatr_suma_impulsow++;
    wiatr_max_suma_impulsow++;
  }
}

volatile uint8_t sekunda = 0;

void IRQ_co_1s() {
    sekunda++;
}

void deszcz_przelicz(){
  pomiary[0] = (float) deszcz_suma_impulsow * 0.2794;
  deszcz_suma_impulsow = 0;
}

void wiatr_przelicz(){
  pomiary[1] = (float) wiatr_suma_impulsow * 2.4 / 60;
  wiatr_suma_impulsow = 0;
}

void przygotuj_pomiary(){
  deszcz_przelicz();
  wiatr_przelicz();
}

void wyslij_pomiary(){
  for(uint8_t i=0;i<sizeof(pomiary)/sizeof(float);i++) {
    if (i>0) Serial.write(' ');
    Serial.print(pomiary[i],2);
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  //ESP
  pinMode(PIN_reset_esp,OUTPUT);
  digitalWrite(PIN_reset_esp,LOW);
  //DESZCZ
  pinMode(PIN_pomiar_deszczu,INPUT);
  digitalWrite(PIN_pomiar_deszczu,LOW);
  attachInterrupt(INT_pomiar_deszczu, IRQ_deszcz, RISING);
  //WIATR
  pinMode(PIN_predkosc_wiatru,INPUT);
  digitalWrite(PIN_predkosc_wiatru,LOW);
  attachInterrupt(INT_predkosc_wiatru, IRQ_wiatr, RISING);
  //zegarek
  Timer1.initialize(1000000);
  Timer1.attachInterrupt(IRQ_co_1s);
  
  interrupts();
}

void loop() {
  if (sekunda == 47) {
    // resetujemy
    digitalWrite(PIN_reset_esp,HIGH);
    digitalWrite(PIN_reset_esp,LOW);
  }
  
  if (sekunda >= 60) {
    sekunda = 0;
    przygotuj_pomiary();
    wyslij_pomiary();
  }
}

