void setup() {
  Serial.begin(9600);   // ส่งไป STM32
}

void loop() {
  Serial.println("HELLO");
  delay(2000);
}