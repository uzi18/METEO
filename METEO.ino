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
#include <OneWire.h>
#include <DallasTemperature.h>

const byte INT_pomiar_deszczu  = 0;
const byte INT_predkosc_wiatru = 1;
const byte PIN_pomiar_deszczu  = 2;
const byte PIN_predkosc_wiatru = 3;
const byte PIN_reset_esp       = 4;
const byte PIN_dallas_1        = 8;
const byte PIN_dallas_2        = 9;
const byte PIN_dallas_3        = 10;
const byte PIN_dallas_4        = 11;
const byte PIN_dallas_5        = 12;
const byte PIN_dallas_6        = 13;
const byte PIN_kierunek_wiatru = A0;

OneWire DS_1(PIN_dallas_1);
OneWire DS_2(PIN_dallas_2);
OneWire DS_3(PIN_dallas_3);
OneWire DS_4(PIN_dallas_4);
OneWire DS_5(PIN_dallas_5);
OneWire DS_6(PIN_dallas_6);

DallasTemperature czujnik_DS_1(&DS_1);
DallasTemperature czujnik_DS_2(&DS_2);
DallasTemperature czujnik_DS_3(&DS_3);
DallasTemperature czujnik_DS_4(&DS_4);
DallasTemperature czujnik_DS_5(&DS_5);
DallasTemperature czujnik_DS_6(&DS_6);


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

void dallas_pomiar(){
  czujnik_DS_1.requestTemperatures();
  czujnik_DS_2.requestTemperatures();
  czujnik_DS_3.requestTemperatures();
  czujnik_DS_4.requestTemperatures();
  czujnik_DS_5.requestTemperatures();
  czujnik_DS_6.requestTemperatures();
}

void dallas_odczyt(){
  pomiary[8] = czujnik_DS_1.getTempCByIndex(0);
  pomiary[9] = czujnik_DS_2.getTempCByIndex(0);
  pomiary[10]= czujnik_DS_3.getTempCByIndex(0);
  pomiary[11]= czujnik_DS_4.getTempCByIndex(0);
  pomiary[12]= czujnik_DS_5.getTempCByIndex(0);
  pomiary[13]= czujnik_DS_6.getTempCByIndex(0);
}

float kierunek_wiatru(uint16_t x){
  if (x<=   8) return 112.5;
  if (x<=  10) return 67.5;
  if (x<=  13) return 90;
  if (x<=  20) return 157.5;
  if (x<=  32) return 135;
  if (x<=  43) return 202.5;
  if (x<=  65) return 180;
  if (x<=  92) return 22.5;
  if (x<= 136) return 45;
  if (x<= 184) return 247.5;
  if (x<= 228) return 225;
  if (x<= 320) return 337.5;
  if (x<= 422) return 0;
  if (x<= 570) return 292.5;
  if (x<= 852) return 315;
  if (x<=1023) return 270;
}
uint16_t pomiar_adc(int pin){
  uint32_t pomiar = 0;
  uint8_t ilosc_probek = 32;
  for(uint8_t i=0;i<ilosc_probek;i++)
      pomiar+=analogRead(pin);

  return pomiar/ilosc_probek;
}

void pomiary_analogowe(){
  uint32_t pomiar = 0;
  pomiary[2]=kierunek_wiatru(pomiar_adc(PIN_kierunek_wiatru));
}

void przygotuj_pomiary(){
  deszcz_przelicz();
  wiatr_przelicz();
  pomiary_analogowe();
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
  
  //DALLASY
  czujnik_DS_1.begin();
  czujnik_DS_2.begin();
  czujnik_DS_3.begin();
  czujnik_DS_4.begin();
  czujnik_DS_5.begin();
  czujnik_DS_6.begin();

  czujnik_DS_1.setWaitForConversion(false);
  czujnik_DS_2.setWaitForConversion(false);
  czujnik_DS_3.setWaitForConversion(false);
  czujnik_DS_4.setWaitForConversion(false);
  czujnik_DS_5.setWaitForConversion(false);
  czujnik_DS_6.setWaitForConversion(false);
  
  analogReference(INTERNAL);
}

void loop() {
  if (sekunda == 47) {
    // resetujemy
    digitalWrite(PIN_reset_esp,HIGH);
    digitalWrite(PIN_reset_esp,LOW);
  }
  
  if (sekunda == 48) {
    dallas_pomiar();
  }
  
  if (sekunda == 50) {
    dallas_odczyt();
  }
  
  if (sekunda >= 60) {
    sekunda = 0;
    przygotuj_pomiary();
    wyslij_pomiary();
  }
}

