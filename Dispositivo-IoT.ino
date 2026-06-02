const int LED_VERMELHO_PIN = 25;
const int LED_VERDE_PIN = 26;

void setup() {
  Serial.begin(115200);

  pinMode(LED_VERMELHO_PIN, OUTPUT);
  pinMode(LED_VERDE_PIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_VERDE_PIN, HIGH);
  digitalWrite(LED_VERMELHO_PIN, LOW);
  delay(1000);

  digitalWrite(LED_VERDE_PIN, LOW);
  digitalWrite(LED_VERMELHO_PIN, HIGH);
  delay(1000);
}