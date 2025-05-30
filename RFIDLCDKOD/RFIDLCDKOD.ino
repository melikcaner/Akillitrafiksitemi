#include <SPI.h>
#include <RFID.h>
#include <LiquidCrystal.h>

#define SDA_DIO 10
#define RESET_DIO 9

RFID RC522(SDA_DIO, RESET_DIO);

// LED pinleri
const int yellow_led = 15;
const int red_led = 14;
const int green_led = 16;
const int buzzer_pin = 8;

LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

unsigned long timer = 0;
int current_state = 0;

bool rfidTriggered = false;
bool waitingForGreen = false;
bool greenActive = false;
unsigned long greenStartTime = 0;

void playAmbulanceSiren(int pin, int duration_ms) {
  unsigned long start = millis();
  while (millis() - start < duration_ms) {
    tone(pin, 4000); delay(120);
    noTone(pin);     delay(60);
    tone(pin, 1500); delay(120);
    noTone(pin);     delay(60);
  }
}

void setup() {
  Serial.begin(9600);
  SPI.begin();
  RC522.init();

  pinMode(yellow_led, OUTPUT);
  pinMode(red_led, OUTPUT);
  pinMode(green_led, OUTPUT);
  pinMode(buzzer_pin, OUTPUT);

  digitalWrite(buzzer_pin, LOW);
  digitalWrite(red_led, HIGH);
  digitalWrite(yellow_led, LOW);
  digitalWrite(green_led, LOW);

  lcd.begin(16, 4);
  lcd.clear();

  timer = millis();
}

void loop() {
  // Kart okutulursa ve sistem hazırsa
  if (RC522.isCard() && !rfidTriggered && current_state == 0) {
    RC522.readCardSerial();
    rfidTriggered = true;
    waitingForGreen = true;

    digitalWrite(red_led, LOW);
    digitalWrite(yellow_led, HIGH);
    timer = millis(); // 1 saniyelik sarı geçiş süresi başlasın
  }

  // Sarı geçiş süresi bittiğinde yeşile geç
  if (waitingForGreen && millis() - timer >= 1000) {
    waitingForGreen = false;
    greenActive = true;
    greenStartTime = millis();

    digitalWrite(yellow_led, LOW);
    digitalWrite(green_led, HIGH);
    digitalWrite(buzzer_pin, HIGH);

    lcd.clear();
    lcd.setCursor(1, 1);
    lcd.print("AMBULANSA YOL VER");

    // Sireni başlat ama LCD 18 saniyede kapanacak
    playAmbulanceSiren(buzzer_pin, 20000);
  }

  // LCD 18 saniye sonra kapanmalı
  if (greenActive && millis() - greenStartTime >= 18000) {
    lcd.clear();
  }

  // Yeşil 20 saniye dolduysa → sarı → kırmızı
  if (greenActive && millis() - greenStartTime >= 20000) {
    digitalWrite(green_led, LOW);
    digitalWrite(buzzer_pin, LOW);

    digitalWrite(yellow_led, HIGH);
    delay(1000);
    digitalWrite(yellow_led, LOW);

    digitalWrite(red_led, HIGH);

    greenActive = false;
    rfidTriggered = false;
    timer = millis();
  }

  // Normal trafik ışığı döngüsü (kart okutulmamışsa çalışır)
  unsigned long current_time = millis();
  if (!rfidTriggered) {
    switch (current_state) {
      case 0:
        if (current_time - timer >= 15000) {
          digitalWrite(red_led, LOW);
          digitalWrite(yellow_led, HIGH);
          current_state = 1;
          timer = current_time;
        }
        break;
      case 1:
        if (current_time - timer >= 1000) {
          digitalWrite(yellow_led, LOW);
          digitalWrite(green_led, HIGH);
          current_state = 2;
          timer = current_time;
        }
        break;
      case 2:
        if (current_time - timer >= 6000) {
          digitalWrite(green_led, LOW);
          digitalWrite(red_led, HIGH);
          current_state = 0;
          timer = current_time;
        }
        break;
    }
  }
}
