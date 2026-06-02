const int LED_VERMELHO_PIN = 25;
const int LED_VERDE_PIN = 26;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_VERMELHO_PIN, OUTPUT);
  pinMode(LED_VERDE_PIN, OUTPUT);

  digitalWrite(LED_VERMELHO_PIN, LOW);
  digitalWrite(LED_VERDE_PIN, LOW);

  Serial.println("==================================");
  Serial.println("Phycocarbon IoT - ESP32 iniciado");
  Serial.println("Circuito base com LEDs configurado");
  Serial.println("==================================");
}

void loop() {
  Serial.println("Status: tanque em operacao normal");

  digitalWrite(LED_VERDE_PIN, HIGH);
  digitalWrite(LED_VERMELHO_PIN, LOW);
  delay(3000);

  Serial.println("Status: simulando alerta critico");

  digitalWrite(LED_VERDE_PIN, LOW);
  digitalWrite(LED_VERMELHO_PIN, HIGH);
  delay(3000);
}