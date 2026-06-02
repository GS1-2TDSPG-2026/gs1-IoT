#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const int LED_VERMELHO_PIN = 25;
const int LED_VERDE_PIN = 26;

const int PH_PIN = 34;
const int LDR_PIN = 35;

const float PH_MIN_IDEAL = 6.5;
const float PH_MAX_IDEAL = 8.5;

const int LUMINOSIDADE_MIN_IDEAL = 250;
const int LUMINOSIDADE_MAX_IDEAL = 900;

LiquidCrystal_I2C lcd(0x27, 16, 2);

float mapFloat(float valor, float entradaMin, float entradaMax, float saidaMin, float saidaMax) {
  return (valor - entradaMin) * (saidaMax - saidaMin) / (entradaMax - entradaMin) + saidaMin;
}

float lerPH() {
  int leituraADC = analogRead(PH_PIN);
  float ph = mapFloat(leituraADC, 0, 4095, 0.0, 14.0);
  return ph;
}

int lerLuminosidade() {
  int leituraADC = analogRead(LDR_PIN);

  // Escala didática: 0 a 1000
  int luminosidade = map(leituraADC, 0, 4095, 0, 1000);

  luminosidade = constrain(luminosidade, 0, 1000);

  return luminosidade;
}

void atualizarAtuadores(bool sistemaNormal) {
  if (sistemaNormal) {
    digitalWrite(LED_VERDE_PIN, HIGH);
    digitalWrite(LED_VERMELHO_PIN, LOW);
  } else {
    digitalWrite(LED_VERDE_PIN, LOW);
    digitalWrite(LED_VERMELHO_PIN, HIGH);
  }
}

void atualizarDisplay(float ph, int luminosidade, bool phNormal, bool luzNormal) {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("pH:");
  lcd.print(ph, 1);
  lcd.print(" L:");
  lcd.print(luminosidade);

  lcd.setCursor(0, 1);

  if (phNormal && luzNormal) {
    lcd.print("STATUS: NORMAL");
  } else if (!phNormal && !luzNormal) {
    lcd.print("ALERTA PH/LUZ");
  } else if (!phNormal) {
    lcd.print("ALERTA PH");
  } else {
    lcd.print("ALERTA LUZ");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_VERMELHO_PIN, OUTPUT);
  pinMode(LED_VERDE_PIN, OUTPUT);

  pinMode(PH_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);

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
  int luminosidade = lerLuminosidade();

  bool phNormal = ph >= PH_MIN_IDEAL && ph <= PH_MAX_IDEAL;
  bool luzNormal = luminosidade >= LUMINOSIDADE_MIN_IDEAL && luminosidade <= LUMINOSIDADE_MAX_IDEAL;

  bool sistemaNormal = phNormal && luzNormal;

  atualizarAtuadores(sistemaNormal);
  atualizarDisplay(ph, luminosidade, phNormal, luzNormal);

  delay(1000);
}