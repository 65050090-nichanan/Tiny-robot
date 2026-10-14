// ======================================================
// ทดสอบว่าขา GPIO ยังดีอยู่ไหม
//
// ใช้ตอบคำถามว่า "GPIO21 เสียหรือเปล่า" โดยไม่ต้องเดา
//
// วิธีใช้ -- สำคัญมาก
//   1. ถอดสายทุกเส้นออกจาก ESP32 ให้หมด เหลือแค่สาย USB
//      (จอ OLED, สายไป STM32, ทุกอย่าง)
//   2. อัปโหลดสเก็ตช์นี้
//   3. เปิด Serial Monitor ที่ 115200
//
// หลักการ: เปิดตัวต้านทานดึงขึ้นภายในของ ESP32 ที่ขานั้น
// ขาที่ดีและไม่มีอะไรต่ออยู่ ต้องอ่านได้ HIGH เสมอ
// ถ้าอ่านได้ LOW ทั้งที่ไม่มีอะไรต่อ แปลว่าขานั้นมีปัญหาจริง
// ======================================================

const int PINS[] = { 21, 22, 32, 33, 16, 17 };
const int PIN_COUNT = sizeof(PINS) / sizeof(PINS[0]);

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("=== ทดสอบขา GPIO ===");
  Serial.println("ถอดสายทุกเส้นออกจากบอร์ดก่อน เหลือแค่ USB");
  Serial.println("ขาที่ดีต้องอ่านได้ HIGH ทุกขา");
  Serial.println();

  for (int i = 0; i < PIN_COUNT; i++) {
    pinMode(PINS[i], INPUT_PULLUP);
  }
  delay(50);  // รอให้แรงดันบนขาขึ้นจนนิ่ง
}

void loop() {
  int bad = 0;
  Serial.println("--------------------------------");
  for (int i = 0; i < PIN_COUNT; i++) {
    int level = digitalRead(PINS[i]);
    Serial.print("  GPIO");
    Serial.print(PINS[i]);
    Serial.print(PINS[i] < 10 ? "  : " : " : ");
    if (level == HIGH) {
      Serial.println("HIGH  ปกติ");
    } else {
      Serial.println("LOW   <-- ผิดปกติ ขานี้มีปัญหาหรือยังมีอะไรต่ออยู่");
      bad++;
    }
  }
  Serial.print("สรุป: ");
  if (bad == 0) {
    Serial.println("ทุกขาปกติดี");
  } else {
    Serial.print(bad);
    Serial.println(" ขาผิดปกติ -- ถ้าแน่ใจว่าถอดสายหมดแล้ว แปลว่าขานั้นเสียจริง");
  }
  delay(3000);
}
