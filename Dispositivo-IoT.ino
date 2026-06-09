#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP32Servo.h>
#include <math.h>

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

const char* MQTT_CLIENT_ID_BASE = "phycocarbon-esp32-tanque01";

// 3 topicos MQTT exigidos/documentados
const char* MQTT_TOPIC_TELEMETRIA = "phycocarbon/fiap/tanque01/telemetria";
const char* MQTT_TOPIC_COMANDOS   = "phycocarbon/fiap/tanque01/comandos";
const char* MQTT_TOPIC_ALERTAS    = "phycocarbon/fiap/tanque01/alertas";

// =====================
// Identificacao alinhada com o banco
// =====================
const int FAZENDA_ID = 5;
const int TANQUE_ID = 10;
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
const int SERVO_PIN = 27;

// =====================
// Servo
// =====================
// No Wokwi:
// 0 graus  = aberto / Colheita ON
// 90 graus = fechado / Colheita OFF
const int SERVO_ABERTO = 0;
const int SERVO_FECHADO = 90;

bool colheitaAutomaticaJaAcionada = false;
bool servoManualAberto = false;
bool servoAbertoAtual = false;

// Evita publicar o mesmo alerta repetidamente
String ultimoAlertaPublicado = "";

// =====================
// Faixas ideais do tanque 10
// =====================
const float PH_MIN_IDEAL = 6.5;
const float PH_MAX_IDEAL = 8.5;

const float TEMP_MIN_IDEAL = 18.0;
const float TEMP_MAX_IDEAL = 28.0;

const int LUMINOSIDADE_MIN_IDEAL = 3000;
const int LUMINOSIDADE_MAX_IDEAL = 15000;

const int TURBIDEZ_MIN_IDEAL = 100;
const int TURBIDEZ_COLHEITA = 650;

// =====================
// Tempo
// =====================
const unsigned long INTERVALO_PUBLICACAO_MS = 5000;
unsigned long ultimaPublicacao = 0;

// =====================
// Controle anti-duplicidade de telemetria
// =====================
bool primeiraTelemetriaPublicada = false;

float ultimoPhTelemetria = -999.0;
float ultimaTemperaturaTelemetria = -999.0;
int ultimaTurbidezTelemetria = -999;
int ultimaLuminosidadeTelemetria = -999;
String ultimoStatusTelemetria = "";
bool ultimoProntoColheitaTelemetria = false;
bool ultimoServoAbertoTelemetria = false;

const float LIMIAR_MUDANCA_PH = 0.1;
const float LIMIAR_MUDANCA_TEMPERATURA = 0.5;
const int LIMIAR_MUDANCA_TURBIDEZ = 10;
const int LIMIAR_MUDANCA_LUMINOSIDADE = 50;

// =====================
// Objetos
// =====================
LiquidCrystal_I2C lcd(0x27, 16, 2);

OneWire oneWire(TEMP_PIN);
DallasTemperature sensorTemperatura(&oneWire);

WiFiClient espClient;
PubSubClient mqttClient(espClient);

Servo servoColheita;

// =====================
// Funcoes auxiliares
// =====================
float mapFloat(
  float valor,
  float entradaMin,
  float entradaMax,
  float saidaMin,
  float saidaMax
) {
  return (valor - entradaMin) * (saidaMax - saidaMin) /
         (entradaMax - entradaMin) + saidaMin;
}

float arredondar1Casa(float valor) {
  return round(valor * 10.0) / 10.0;
}

bool ehAlertaCritico(String statusTanque) {
  return statusTanque == "PH_BAIXO" ||
         statusTanque == "PH_ALTO" ||
         statusTanque == "PH_CRITICO" ||
         statusTanque == "TEMPERATURA_ALTA" ||
         statusTanque == "TEMPERATURA_BAIXA" ||
         statusTanque == "TURBIDEZ_FORA_PADRAO" ||
         statusTanque == "LUMINOSIDADE_BAIXA";
}

bool devePublicarTelemetria(
  float ph,
  float temperatura,
  int turbidez,
  int luminosidade,
  String statusTanque,
  bool prontoColheita
) {
  if (!primeiraTelemetriaPublicada) {
    return true;
  }

  if (fabs(ph - ultimoPhTelemetria) >= LIMIAR_MUDANCA_PH) {
    return true;
  }

  if (fabs(temperatura - ultimaTemperaturaTelemetria) >= LIMIAR_MUDANCA_TEMPERATURA) {
    return true;
  }

  if (abs(turbidez - ultimaTurbidezTelemetria) >= LIMIAR_MUDANCA_TURBIDEZ) {
    return true;
  }

  if (abs(luminosidade - ultimaLuminosidadeTelemetria) >= LIMIAR_MUDANCA_LUMINOSIDADE) {
    return true;
  }

  if (statusTanque != ultimoStatusTelemetria) {
    return true;
  }

  if (prontoColheita != ultimoProntoColheitaTelemetria) {
    return true;
  }

  if (servoAbertoAtual != ultimoServoAbertoTelemetria) {
    return true;
  }

  return false;
}

void salvarUltimaTelemetria(
  float ph,
  float temperatura,
  int turbidez,
  int luminosidade,
  String statusTanque,
  bool prontoColheita
) {
  primeiraTelemetriaPublicada = true;

  ultimoPhTelemetria = ph;
  ultimaTemperaturaTelemetria = temperatura;
  ultimaTurbidezTelemetria = turbidez;
  ultimaLuminosidadeTelemetria = luminosidade;
  ultimoStatusTelemetria = statusTanque;
  ultimoProntoColheitaTelemetria = prontoColheita;
  ultimoServoAbertoTelemetria = servoAbertoAtual;
}

// =====================
// Servo
// =====================
void abrirServoColheita() {
  servoColheita.write(SERVO_ABERTO);
  servoAbertoAtual = true;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Servo aberto");
  lcd.setCursor(0, 1);
  lcd.print("Colheita ON");

  delay(1000);
}

void fecharServoColheita() {
  servoColheita.write(SERVO_FECHADO);
  servoAbertoAtual = false;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Servo fechado");
  lcd.setCursor(0, 1);
  lcd.print("Colheita OFF");

  delay(1000);
}

void acionarColheitaTemporaria() {
  abrirServoColheita();
  delay(1500);
  fecharServoColheita();
}

// =====================
// Wi-Fi
// =====================
void conectarWiFi() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Conectando");
  lcd.setCursor(0, 1);
  lcd.print("Wi-Fi...");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int tentativas = 0;

  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    tentativas++;
  }

  lcd.clear();

  if (WiFi.status() == WL_CONNECTED) {
    lcd.setCursor(0, 0);
    lcd.print("Wi-Fi OK");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());

    Serial.print("Wi-Fi conectado. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Falha Wi-Fi");
    lcd.setCursor(0, 1);
    lcd.print("Verifique rede");

    Serial.println("Falha ao conectar no Wi-Fi.");
  }

  delay(2000);
}

// =====================
// Callback MQTT - comandos
// =====================
void receberComandoMQTT(
  char* topic,
  byte* payload,
  unsigned int length
) {
  String mensagem = "";

  for (unsigned int i = 0; i < length; i++) {
    mensagem += (char)payload[i];
  }

  mensagem.trim();

  String comando = mensagem;

  // Aceita:
  // 1) Texto puro: ABRIR_SERVO
  // 2) JSON: {"comando":"ABRIR_SERVO"}
  StaticJsonDocument<256> doc;
  DeserializationError erro = deserializeJson(doc, mensagem);

  if (!erro && doc["comando"]) {
    comando = doc["comando"].as<String>();
  }

  comando.trim();
  comando.toUpperCase();

  Serial.print("Comando MQTT recebido no topico ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(comando);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Cmd MQTT:");
  lcd.setCursor(0, 1);
  lcd.print(comando);

  delay(1000);

  if (
    comando == "COLHER" ||
    comando == "LIGAR_BOMBA" ||
    comando == "ACIONAR_COLHEITA"
  ) {
    servoManualAberto = false;
    acionarColheitaTemporaria();

    Serial.println("Colheita acionada por comando MQTT.");
  }
  else if (
    comando == "ABRIR_SERVO" ||
    comando == "SERVO_ABRIR" ||
    comando == "ABRIR"
  ) {
    servoManualAberto = true;
    abrirServoColheita();

    Serial.println("Servo aberto por comando MQTT.");
  }
  else if (
    comando == "FECHAR_SERVO" ||
    comando == "SERVO_FECHAR" ||
    comando == "FECHAR"
  ) {
    servoManualAberto = false;
    fecharServoColheita();

    Serial.println("Servo fechado por comando MQTT.");
  }
  else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Comando");
    lcd.setCursor(0, 1);
    lcd.print("desconhecido");

    Serial.print("Comando desconhecido: ");
    Serial.println(comando);

    delay(1000);
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
    String clientId =
      String(MQTT_CLIENT_ID_BASE) + "-" +
      String((uint32_t)ESP.getEfuseMac(), HEX);

    bool conectado = mqttClient.connect(clientId.c_str());

    if (conectado) {
      mqttClient.subscribe(MQTT_TOPIC_COMANDOS);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("MQTT conectado");
      lcd.setCursor(0, 1);
      lcd.print("Comandos OK");

      Serial.print("MQTT conectado. Inscrito em: ");
      Serial.println(MQTT_TOPIC_COMANDOS);

      delay(2000);
    } else {
      tentativas++;

      Serial.print("Falha MQTT. Tentativa ");
      Serial.println(tentativas);

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

  int luminosidade = map(
    leituraADC,
    0,
    4095,
    0,
    15000
  );

  return constrain(luminosidade, 0, 15000);
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

  int turbidez = map(
    leituraADC,
    0,
    4095,
    0,
    1000
  );

  return constrain(turbidez, 0, 1000);
}

// =====================
// Status operacional
// =====================
String obterStatusTanque(
  float ph,
  int luminosidade,
  float temperatura,
  int turbidez,
  bool prontoColheita
) {
  if (ph <= 0.0) {
    return "PH_CRITICO";
  }

  if (ph < PH_MIN_IDEAL) {
    return "PH_BAIXO";
  }

  if (ph > PH_MAX_IDEAL) {
    return "PH_ALTO";
  }

  if (temperatura < TEMP_MIN_IDEAL) {
    return "TEMPERATURA_BAIXA";
  }

  if (temperatura > TEMP_MAX_IDEAL) {
    return "TEMPERATURA_ALTA";
  }

  if (turbidez < TURBIDEZ_MIN_IDEAL) {
    return "TURBIDEZ_FORA_PADRAO";
  }

  if (luminosidade < LUMINOSIDADE_MIN_IDEAL) {
    return "LUMINOSIDADE_BAIXA";
  }

  if (prontoColheita) {
    return "PRONTO_COLHEITA";
  }

  return "NORMAL";
}

String obterMensagemAlerta(String statusTanque) {
  if (statusTanque == "PH_CRITICO") {
    return "pH critico detectado no tanque.";
  }

  if (statusTanque == "PH_BAIXO") {
    return "pH abaixo da faixa ideal.";
  }

  if (statusTanque == "PH_ALTO") {
    return "pH acima da faixa ideal.";
  }

  if (statusTanque == "TEMPERATURA_BAIXA") {
    return "Temperatura abaixo da faixa ideal.";
  }

  if (statusTanque == "TEMPERATURA_ALTA") {
    return "Temperatura acima da faixa ideal.";
  }

  if (statusTanque == "TURBIDEZ_FORA_PADRAO") {
    return "Turbidez fora do padrao operacional.";
  }

  if (statusTanque == "LUMINOSIDADE_BAIXA") {
    return "Luminosidade insuficiente para o cultivo.";
  }

  return "Alerta operacional detectado.";
}

String obterSeveridadeAlerta(String statusTanque) {
  if (
    statusTanque == "PH_CRITICO" ||
    statusTanque == "TEMPERATURA_ALTA" ||
    statusTanque == "TEMPERATURA_BAIXA"
  ) {
    return "CRITICO";
  }

  return "ATENCAO";
}

// =====================
// Atuadores e display
// =====================
void atualizarAtuadores(bool sistemaSemAlerta) {
  if (sistemaSemAlerta) {
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
  String statusTanque
) {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("pH:");
  lcd.print(arredondar1Casa(ph));
  lcd.print(" T:");
  lcd.print(arredondar1Casa(temperatura));

  lcd.setCursor(0, 1);

  if (statusTanque.length() > 16) {
    lcd.print(statusTanque.substring(0, 16));
  } else {
    lcd.print(statusTanque);
  }
}

// =====================
// MQTT - Publicacao de alertas
// =====================
void publicarAlerta(
  String statusTanque,
  float ph,
  int luminosidade,
  float temperatura,
  int turbidez,
  bool prontoColheita
) {
  if (!ehAlertaCritico(statusTanque)) {
    return;
  }

  StaticJsonDocument<512> doc;

  doc["id_dispositivo"] = DISPOSITIVO_ID;
  doc["id_tanque"] = TANQUE_ID;
  doc["id_fazenda"] = FAZENDA_ID;

  doc["tipo_alerta"] = statusTanque;
  doc["severidade"] = obterSeveridadeAlerta(statusTanque);
  doc["status_alerta"] = "ABERTO";
  doc["mensagem"] = obterMensagemAlerta(statusTanque);

  doc["pH"] = arredondar1Casa(ph);
  doc["temp"] = arredondar1Casa(temperatura);
  doc["turbidez"] = turbidez;
  doc["luminosidade"] = luminosidade;

  doc["pronto_colheita"] = prontoColheita;
  doc["servo_aberto"] = servoAbertoAtual;

  char payload[512];
  serializeJson(doc, payload);

  Serial.print("ALERTA enviado via MQTT: ");
  Serial.println(payload);

  mqttClient.publish(MQTT_TOPIC_ALERTAS, payload);
}

void verificarEPublicarAlerta(
  String statusTanque,
  float ph,
  int luminosidade,
  float temperatura,
  int turbidez,
  bool prontoColheita
) {
  if (!ehAlertaCritico(statusTanque)) {
    ultimoAlertaPublicado = "";
    return;
  }

  if (statusTanque != ultimoAlertaPublicado) {
    publicarAlerta(
      statusTanque,
      ph,
      luminosidade,
      temperatura,
      turbidez,
      prontoColheita
    );

    ultimoAlertaPublicado = statusTanque;
  }
}

// =====================
// MQTT - Publicacao de telemetria
// =====================
void publicarTelemetria(
  float ph,
  int luminosidade,
  float temperatura,
  int turbidez,
  String statusTanque,
  bool prontoColheita
) {
  bool devePublicar = devePublicarTelemetria(
    ph,
    temperatura,
    turbidez,
    luminosidade,
    statusTanque,
    prontoColheita
  );

  if (!devePublicar) {
    Serial.println("Telemetria sem mudanca relevante. MQTT nao publicado.");
    return;
  }

  StaticJsonDocument<512> doc;

  // Campos alinhados com IotTelemetriaRequestDto da .NET
  doc["dispositivo_id"] = DISPOSITIVO_ID;
  doc["pH"] = arredondar1Casa(ph);
  doc["temp"] = arredondar1Casa(temperatura);
  doc["turbidez"] = turbidez;
  doc["luminosidade"] = luminosidade;
  doc["status"] = statusTanque;
  doc["pronto_colheita"] = prontoColheita;
  doc["servo_aberto"] = servoAbertoAtual;

  char payload[512];
  serializeJson(doc, payload);

  Serial.print("TELEMETRIA enviada via MQTT: ");
  Serial.println(payload);

  bool publicado = mqttClient.publish(
    MQTT_TOPIC_TELEMETRIA,
    payload
  );

  lcd.clear();

  if (publicado) {
    salvarUltimaTelemetria(
      ph,
      temperatura,
      turbidez,
      luminosidade,
      statusTanque,
      prontoColheita
    );

    lcd.setCursor(0, 0);
    lcd.print("MQTT publicado");
    lcd.setCursor(0, 1);
    lcd.print(statusTanque);
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

  servoColheita.attach(SERVO_PIN);

  servoColheita.write(SERVO_FECHADO);
  servoAbertoAtual = false;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Phycocarbon");
  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");

  delay(2000);

  conectarWiFi();

  mqttClient.setServer(
    MQTT_BROKER,
    MQTT_PORT
  );

  mqttClient.setCallback(
    receberComandoMQTT
  );

  conectarMQTT();
}

// =====================
// Loop principal
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

  ph = arredondar1Casa(ph);
  temperatura = arredondar1Casa(temperatura);

  bool prontoColheita = turbidez >= TURBIDEZ_COLHEITA;

  String statusTanque = obterStatusTanque(
    ph,
    luminosidade,
    temperatura,
    turbidez,
    prontoColheita
  );

  bool sistemaSemAlerta = !ehAlertaCritico(statusTanque);

  atualizarAtuadores(sistemaSemAlerta);

  atualizarDisplay(
    ph,
    luminosidade,
    temperatura,
    turbidez,
    statusTanque
  );

  verificarEPublicarAlerta(
    statusTanque,
    ph,
    luminosidade,
    temperatura,
    turbidez,
    prontoColheita
  );

  if (prontoColheita && !colheitaAutomaticaJaAcionada && !servoManualAberto) {
    acionarColheitaTemporaria();
    colheitaAutomaticaJaAcionada = true;
  }

  if (!prontoColheita) {
    colheitaAutomaticaJaAcionada = false;

    if (!servoManualAberto) {
      servoColheita.write(SERVO_FECHADO);
      servoAbertoAtual = false;
    }
  }

  unsigned long agora = millis();

  if (agora - ultimaPublicacao >= INTERVALO_PUBLICACAO_MS) {
    ultimaPublicacao = agora;

    publicarTelemetria(
      ph,
      luminosidade,
      temperatura,
      turbidez,
      statusTanque,
      prontoColheita
    );
  }

  delay(1000);
}