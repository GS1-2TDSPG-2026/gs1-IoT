void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("==================================");
  Serial.println("Phycocarbon IoT - ESP32 iniciado");
  Serial.println("Projeto base funcionando no Arduino IDE");
  Serial.println("==================================");
}

void loop() {
  Serial.println("ESP32 ativo...");
  delay(5000);
}