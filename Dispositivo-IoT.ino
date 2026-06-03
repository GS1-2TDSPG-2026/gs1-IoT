#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// =====================
// Wi-Fi Wokwi
// =====================
const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

// =====================
// MQTT
// =====================
const char* MQTT_BROKER = "broker.hivemq.com";
const int MQTT_PORT = 1883;

const char* MQTT_CLIENT_ID = "phycocarbon-esp32-tanque01";
const char* MQTT_TOPIC_TELEMETRIA = "phycocarbon/fiap/tanque01/telemetria";

// =====================
// Identificação do dispositivo
// =====================
const int DISPOSITIVO_ID = 10;

// =====================
// Pinos
// =====================
const int LED_VERMELHO_PIN = 25;
const int LED_VERDE_PIN = 26;

const int PH_PIN = 34;
const int LDR_PIN = 35;
const int TEMP_PIN = 4;
const int TURBIDEZ_PIN = 32;

// =====================
// Faixas ideais
// =====================
const float PH_MIN_IDEAL = 6.5;
const float PH_MAX_IDEAL = 8.5;

const int LUMINOSIDADE_MIN_IDEAL = 250;
const int LUMINOSIDADE_MAX_IDEAL = 900;

const float TEMP_MIN_IDEAL = 20.0;
const float TEMP_MAX_IDEAL = 30.0;

const int TURBIDEZ_MIN_IDEAL = 100;
const int TURBIDEZ_MAX_IDEAL = 700;

// =====================
// Tempo de publicação MQTT
// =====================
const unsigned long INTERVALO_PUBLICACAO_MS = 5000;
unsigned long ultimaPublicacao = 0;

// =====================
// Objetos
// =====================
LiquidCrystal_I2C lcd(0x27, 16, 2);

OneWire oneWire(TEMP_PIN);
DallasTemperature sensorTemperatura(&oneWire);

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// =====================
// Funções auxiliares
// =====================
float mapFloat(float valor, float entradaMin, float entradaMax, float saidaMin, float saidaMax) {
  return (valor - entradaMin) * (saidaMax - saidaMin) / (entradaMax - entradaMin) + saidaMin;
}

float arredondar1Casa(float valor) {
  return round(valor * 10.0) / 10.0;
}

// =====================
// Wi-Fi
// =====================
void conectarWiFi() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Conectando");
  lcd.setCursor(0, 1);
  lcd.print("WiFi...");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int tentativas = 0;

  while (WiFi.status() != WL_CONNECTED && tentativas < 30) {
    delay(500);
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi conectado");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());

    delay(2000);
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Falha WiFi");
    lcd.setCursor(0, 1);
    lcd.print("Verifique rede");

    delay(2000);
  }
}

// =====================
// MQTT
// =====================
void conectarMQTT() {
  if (mqttClient.connected()) {
    return;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Conectando");
  lcd.setCursor(0, 1);
  lcd.print("MQTT...");

  int tentativas = 0;

  while (!mqttClient.connected() && tentativas < 10) {
    bool conectado = mqttClient.connect(MQTT_CLIENT_ID);

    if (conectado) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("MQTT conectado");
      lcd.setCursor(0, 1);
      lcd.print("HiveMQ OK");

      delay(2000);
    } else {
      tentativas++;
      delay(1000);
    }
  }

  if (!mqttClient.connected()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Falha MQTT");
    lcd.setCursor(0, 1);
    lcd.print("Broker offline");

    delay(2000);
  }
}

// =====================
// Sensores
// =====================
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

// =====================
// Atuadores e display
// =====================
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

// =====================
// Publicação MQTT JSON
// =====================
void publicarTelemetria(float ph, int luminosidade, float temperatura, int turbidez) {
  StaticJsonDocument<256> doc;

  doc["dispositivo_id"] = DISPOSITIVO_ID;
  doc["pH"] = arredondar1Casa(ph);
  doc["temp"] = arredondar1Casa(temperatura);
  doc["turbidez"] = arredondar1Casa(turbidez);
  doc["luminosidade"] = luminosidade;

  char payload[256];
  serializeJson(doc, payload);

  Serial.print("JSON enviado via MQTT: ");
  Serial.println(payload);  

  bool publicado = mqttClient.publish(MQTT_TOPIC_TELEMETRIA, payload);

  lcd.clear();

  if (publicado) {
    lcd.setCursor(0, 0);
    lcd.print("MQTT publicado");
    lcd.setCursor(0, 1);
    lcd.print("Telemetria OK");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Falha publish");
    lcd.setCursor(0, 1);
    lcd.print("MQTT erro");
  }

  delay(1000);
}

// =====================
// Setup
// =====================
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
  lcd.print("JSON MQTT");

  delay(1500);

  conectarWiFi();

  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setBufferSize(512);

  conectarMQTT();
}

// =====================
// Loop
// =====================
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  if (!mqttClient.connected()) {
    conectarMQTT();
  }

  mqttClient.loop();

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

  unsigned long agora = millis();

  if (agora - ultimaPublicacao >= INTERVALO_PUBLICACAO_MS) {
    ultimaPublicacao = agora;
    publicarTelemetria(ph, luminosidade, temperatura, turbidez);
  }

  delay(1000);
}