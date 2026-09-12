// ======================================================
// คุยกับจอ OLED ด้วยคำสั่งดิบๆ ผ่าน Wire อย่างเดียว ไม่ใช้ไลบรารีจอเลย
//
// ใช้ตอนที่ลองไลบรารีแล้วไม่ได้ผล เพื่อตอบ 3 คำถามนี้ให้ชัด
//   1. จอรับคำสั่งไหม (ดูจาก ACK ตอนส่ง)
//   2. จอสูงกี่หน้า (1 หน้า = 8 แถวพิกเซล จอ 64 แถว = 8 หน้า, จอ 32 แถว = 4 หน้า)
//   3. ต้องชดเชยคอลัมน์เท่าไหร่ (SSD1306 ใช้ 0, SH1106 ใช้ 2)
//
// ใช้โหมด page addressing ซึ่งชิป OLED ทุกตัวรองรับ
// ต่างจากไลบรารี Adafruit_SSD1306 ที่ใช้ horizontal addressing ซึ่ง SH1106 ไม่มี
//
// ต่อสาย: SDA -> GPIO32, SCL -> GPIO33, VCC -> 3V3, GND -> GND
// เปิด Serial Monitor ที่ 115200
// ======================================================

#include <Wire.h>

const int SDA_PIN = 32;
const int SCL_PIN = 33;
const int WIDTH = 128;

uint8_t addr = 0x3C;  // จะถูกแทนที่ด้วยค่าที่สแกนเจอ

// ส่งคำสั่ง 1 ไบต์ คืนค่า true ถ้าจอตอบรับ (ACK)
bool cmd(uint8_t c) {
  Wire.beginTransmission(addr);
  Wire.write(0x00);  // ไบต์นำหน้าบอกว่าต่อไปเป็นคำสั่ง
  Wire.write(c);
  return Wire.endTransmission() == 0;
}

// ลำดับคำสั่งเปิดจอ ใส่คำสั่งของทั้ง SSD1306 และ SH1106 ไว้ทั้งคู่
// ชิปที่ไม่รู้จักคำสั่งไหนจะข้ามไปเอง ไม่พัง
void initPanel(uint8_t mux) {
  cmd(0xAE);              // ปิดจอก่อนตั้งค่า
  cmd(0xD5); cmd(0x80);   // ความถี่สแกน
  cmd(0xA8); cmd(mux);    // จำนวนแถวที่สแกน 0x3F = 64 แถว, 0x1F = 32 แถว
  cmd(0xD3); cmd(0x00);   // ไม่เลื่อนภาพแนวตั้ง
  cmd(0x40);              // เริ่มที่แถวบนสุด
  cmd(0x8D); cmd(0x14);   // เปิดปั๊มแรงดัน แบบ SSD1306
  cmd(0xAD); cmd(0x8B);   // เปิดปั๊มแรงดัน แบบ SH1106
  cmd(0xA1);              // กลับซ้ายขวา
  cmd(0xC8);              // กลับบนล่าง
  cmd(0xDA); cmd(mux == 0x3F ? 0x12 : 0x02);  // การจัดเรียงขา COM ต้องตรงกับความสูงจอ
  cmd(0x81); cmd(0x7F);   // ความสว่างกลางๆ
  cmd(0xD9); cmd(0xF1);
  cmd(0xDB); cmd(0x40);
  cmd(0xA4);              // แสดงตามข้อมูลใน RAM
  cmd(0xA6);              // สีปกติ ไม่กลับขาวดำ
  cmd(0xAF);              // เปิดจอ
}

// เขียนข้อมูลลงหน้าที่กำหนด ทั้งแถว
void fillPage(uint8_t page, uint8_t pattern, uint8_t colOffset) {
  cmd(0xB0 + page);                      // เลือกหน้า
  cmd(0x00 | (colOffset & 0x0F));        // คอลัมน์เริ่มต้น 4 บิตล่าง
  cmd(0x10 | (colOffset >> 4));          // คอลัมน์เริ่มต้น 4 บิตบน

  // ส่งทีละ 16 ไบต์ เพราะบัฟเฟอร์ของ Wire มีจำกัด
  for (int x = 0; x < WIDTH; x += 16) {
    Wire.beginTransmission(addr);
    Wire.write(0x40);  // ไบต์นำหน้าบอกว่าต่อไปเป็นข้อมูลภาพ
    for (int i = 0; i < 16; i++) Wire.write(pattern);
    Wire.endTransmission();
  }
}

void fillAll(uint8_t pattern, uint8_t colOffset, int pages) {
  for (int p = 0; p < pages; p++) fillPage(p, pattern, colOffset);
}

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println("\n=== คุยกับจอแบบดิบๆ ===");

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  Serial.println("ไล่หาอุปกรณ์บนสาย I2C...");
  bool found = false;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  เจอที่ 0x%02X\n", a);
      if (!found) { addr = a; found = true; }
    }
  }
  if (!found) {
    Serial.println("  ไม่เจออะไรเลย -> สายมีปัญหา หยุดทำงาน");
    while (true) delay(1000);
  }
  Serial.printf("ใช้แอดเดรส 0x%02X\n", addr);
  Serial.println("จอตอบรับคำสั่ง: " + String(cmd(0xAE) ? "ใช่" : "ไม่"));
}

void loop() {
  // ---------- รอบที่ 1: ตั้งเป็นจอ 64 แถว ไล่ติดทีละหน้า ----------
  Serial.println("\n########## ตั้งค่าเป็นจอ 64 แถว (8 หน้า) ##########");
  initPanel(0x3F);
  fillAll(0x00, 0, 8);  // ล้างจอให้ดำหมดก่อน
  delay(500);

  Serial.println("ไล่ติดทีละหน้า หน้าละ 1.5 วินาที -- นับดูว่ามีกี่แถบ และแถบไล่ลงครบจอไหม");
  for (int p = 0; p < 8; p++) {
    fillAll(0x00, 0, 8);
    fillPage(p, 0xFF, 0);
    Serial.printf("  หน้า %d (แถวพิกเซล %d-%d)\n", p, p * 8, p * 8 + 7);
    delay(1500);
  }

  Serial.println("ขาวเต็มจอ 3 วินาที -- ขาวหมดทั้งจอหรือเปล่า");
  fillAll(0xFF, 0, 8);
  delay(3000);

  // ---------- รอบที่ 2: ชดเชยคอลัมน์ 2 สำหรับ SH1106 ----------
  Serial.println("\n########## แบบชดเชยคอลัมน์ 2 (สไตล์ SH1106) ##########");
  fillAll(0x00, 2, 8);
  delay(300);
  fillAll(0xFF, 2, 8);
  Serial.println("ขาวเต็มจอ 3 วินาที -- แบบนี้ขอบซ้ายสะอาดกว่าเดิมไหม");
  delay(3000);

  // ---------- รอบที่ 3: ตั้งเป็นจอ 32 แถว ----------
  Serial.println("\n########## ตั้งค่าเป็นจอ 32 แถว (4 หน้า) ##########");
  initPanel(0x1F);
  fillAll(0x00, 0, 4);
  delay(300);
  fillAll(0xFF, 0, 4);
  Serial.println("ขาวเต็มจอ 3 วินาที -- แบบนี้เต็มจอกว่าแบบ 64 แถวไหม");
  delay(3000);

  Serial.println("\n--- ครบรอบ เริ่มใหม่ ---");
}
