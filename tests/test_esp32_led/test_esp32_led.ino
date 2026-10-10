#define LED_PIN 2 // ขา LED บนบอร์ดส่วนใหญ่

void setup() {
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_PIN, HIGH); // เปิดไฟ
  delay(500);                  // รอ 0.5 วินาที
  digitalWrite(LED_PIN, LOW);  // ปิดไฟ
  delay(500);
}
