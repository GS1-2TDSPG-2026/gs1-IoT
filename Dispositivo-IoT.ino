#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

const int LED_VERMELHO_PIN = 25;
const int LED_VERDE_PIN = 26;

const int PH_PIN = 34;
const int LDR_PIN = 35;
const int TEMP_PIN = 4;
const int TURBIDEZ_PIN = 32;

const float PH_MIN_IDEAL = 6.5;
const float PH_MAX_IDEAL = 8.5;

const int LUMINOSIDADE_MIN_IDEAL = 250;
const int LUMINOSIDADE_MAX_IDEAL = 900;

const float TEMP_MIN_IDEAL = 20.0;
const float TEMP_MAX_IDEAL = 30.0;

const int TURBIDEZ_MIN_IDEAL = 100;
const int TURBIDEZ_MAX_IDEAL = 700;

LiquidCrystal_I2C lcd(0x27, 16, 2);

OneWire oneWire(TEMP_PIN);
DallasTemperature sensorTemperatura(&oneWire);

float mapFloat(float valor, float entradaMin, float entradaMax, float saidaMin, float saidaMax) {
  return (valor - entradaMin) * (saidaMax - saidaMin) / (entradaMax - entradaMin) + saidaMin;
}

void conectarWiFi() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Conectando");
  lcd.setCursor(0, 1);
  lcd.print("WiFi...");

  Serial.print("Conectando ao WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int tentativas = 0;

  while (WiFi.status() != WL_CONNECTED && tentativas < 30) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi conectado");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());

    delay(2500);
  } else {
    Serial.println();
    Serial.println("Falha ao conectar WiFi.");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Falha WiFi");
    lcd.setCursor(0, 1);
    lcd.print("Verifique rede");

    delay(2500);
  }
}

float lerPH() {
  int leituraADC = analogRead(PH_PIN);
  float ph = mapFloat(leituraADC, 0, 4095, 0.0, 14.0);
  return ph;
}

int lerLuminosidade() {
  int leituraADC = analogRead(LDR_PIN);
  int luminosidade = map(leituraADC, 0, 4095, 0, 1000);
  return constrain(luminosidade, 0, 1000);
}

float lerTemperatura() {
  sensorTemperatura.requestTemperatures();

  float temperatura = sensorTemperatura.getTempCByIndex(0);

  if (temperatura == DEVICE_DISCONNECTED_C) {
    return -99.0;
  }

  return temperatura;
}

int lerTurbidez() {
  int leituraADC = analogRead(TURBIDEZ_PIN);
  int turbidez = map(leituraADC, 0, 4095, 0, 1000);
  return constrain(turbidez, 0, 1000);
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

void atualizarDisplay(
  float ph,
  int luminosidade,
  float temperatura,
  int turbidez,
  bool phNormal,
  bool luzNormal,
  bool tempNormal,
  bool turbidezNormal
) {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("pH:");
  lcd.print(ph, 1);
  lcd.print(" T:");
  lcd.print(temperatura, 1);

  lcd.setCursor(0, 1);

  if (phNormal && luzNormal && tempNormal && turbidezNormal) {
    lcd.print("L:");
    lcd.print(luminosidade);
    lcd.print(" Tu:");
    lcd.print(turbidez);
  } else if (!phNormal) {
    lcd.print("ALERTA PH");
  } else if (!luzNormal) {
    lcd.print("ALERTA LUZ");
  } else if (!tempNormal) {
    lcd.print("ALERTA TEMP");
  } else if (!turbidezNormal) {
    lcd.print("ALERTA TURB");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_VERMELHO_PIN, OUTPUT);
  pinMode(LED_VERDE_PIN, OUTPUT);

  pinMode(PH_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(TURBIDEZ_PIN, INPUT);

  digitalWrite(LED_VERMELHO_PIN, LOW);
  digitalWrite(LED_VERDE_PIN, LOW);

  analogReadResolution(12);

  Wire.begin(21, 22);

  lcd.init();
  lcd.backlight();

  sensorTemperatura.begin();

  lcd.setCursor(0, 0);
  lcd.print("Phycocarbon");
  lcd.setCursor(0, 1);
  lcd.print("Iniciando WiFi");

  delay(1500);

  conectarWiFi();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  float ph = lerPH();
  int luminosidade = lerLuminosidade();
  float temperatura = lerTemperatura();
  int turbidez = lerTurbidez();

  bool phNormal = ph >= PH_MIN_IDEAL && ph <= PH_MAX_IDEAL;
  bool luzNormal = luminosidade >= LUMINOSIDADE_MIN_IDEAL && luminosidade <= LUMINOSIDADE_MAX_IDEAL;
  bool tempNormal = temperatura >= TEMP_MIN_IDEAL && temperatura <= TEMP_MAX_IDEAL;
  bool turbidezNormal = turbidez >= TURBIDEZ_MIN_IDEAL && turbidez <= TURBIDEZ_MAX_IDEAL;

  bool sistemaNormal = phNormal && luzNormal && tempNormal && turbidezNormal;

  atualizarAtuadores(sistemaNormal);
  atualizarDisplay(ph, luminosidade, temperatura, turbidez, phNormal, luzNormal, tempNormal, turbidezNormal);

  delay(1000);
}