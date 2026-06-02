#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int LED_VERMELHO_PIN = 25;
const int LED_VERDE_PIN = 26;
const int PH_PIN = 34;

const float PH_MIN_IDEAL = 6.5;
const float PH_MAX_IDEAL = 8.5;

LiquidCrystal_I2C lcd(0x27, 16, 2);

float mapFloat(float valor, float entradaMin, float entradaMax, float saidaMin, float saidaMax) {
  return (valor - entradaMin) * (saidaMax - saidaMin) / (entradaMax - entradaMin) + saidaMin;
}

float lerPH() {
  int leituraADC = analogRead(PH_PIN);

  float ph = mapFloat(leituraADC, 0, 4095, 0.0, 14.0);

  return ph;
}

void atualizarDisplay(float ph, bool phNormal) {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("pH: ");
  lcd.print(ph, 2);

  lcd.setCursor(0, 1);

  if (phNormal) {
    lcd.print("OK 6.5 - 8.5");
  } else {
    lcd.print("ALERTA PH");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_VERMELHO_PIN, OUTPUT);
  pinMode(LED_VERDE_PIN, OUTPUT);
  pinMode(PH_PIN, INPUT);

  digitalWrite(LED_VERMELHO_PIN, LOW);
  digitalWrite(LED_VERDE_PIN, LOW);

  analogReadResolution(12);

  Wire.begin(21, 22);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Phycocarbon");
  lcd.setCursor(0, 1);
  lcd.print("IoT iniciado");

  delay(2000);
}

void loop() {
  float ph = lerPH();

  bool phNormal = ph >= PH_MIN_IDEAL && ph <= PH_MAX_IDEAL;

  if (phNormal) {
    digitalWrite(LED_VERMELHO_PIN, LOW);
    digitalWrite(LED_VERDE_PIN, HIGH);
  } else {
    digitalWrite(LED_VERMELHO_PIN, HIGH);
    digitalWrite(LED_VERDE_PIN, LOW);
  }

  atualizarDisplay(ph, phNormal);

  delay(1000);
}