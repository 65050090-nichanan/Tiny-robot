// ======================================================
// ทดสอบว่าจอเป็นชิป SH1106 หรือไม่
//
// ใช้เมื่อ: ภาพขึ้นแค่แถบบนของจอ ส่วนที่เหลือเป็นหิมะ และ I2C ส่งข้อมูลครบปกติ
// สาเหตุคือ SH1106 ไม่รองรับโหมด horizontal addressing ที่ไลบรารี SSD1306 ใช้
// ข้อมูลทั้งเฟรมเลยไปกองที่หน้าแรกหน้าเดียว
//
// ต้องลงไลบรารี "Adafruit SH110X" ก่อน (Tools -> Manage Libraries)
//
// ต่อสายเหมือนเดิม: SDA -> GPIO21, SCL -> GPIO22, VCC -> 3V3, GND -> GND
//
// ถ้าสเก็ตช์นี้ทำให้จอขาวเต็มและข้อความอ่านออก = ยืนยันว่าเป็น SH1106
// ให้กลับไปตั้ง OLED_DRIVER_SH1106 เป็น 1 ในสเก็ตช์หลัก
// ======================================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

const int OLED_SDA_PIN = 21;
const int OLED_SCL_PIN = 22;
const uint8_t OLED_ADDR = 0x3C;
// ความสว่างของจอ 0x00 ถึง 0xFF -- คุมกระแสที่ป้อนให้แต่ละพิกเซลโดยตรง
// ค่ายิ่งต่ำยิ่งกินไฟน้อย ช่วยได้มากถ้าไฟเลี้ยงไม่นิ่งจนบอร์ดรีเซ็ตตัวเองตอนวาดรูปที่ติดไฟเยอะ
// 0x30 สว่างพออ่านได้สบายในร่ม ถ้าอยากสว่างขึ้นไล่ขึ้นทีละ 0x10
const uint8_t OLED_CONTRAST = 0x30;

Adafruit_SH1106G display(128, 64, &Wire, -1, 400000, 400000);

// รอจนจอตอบรับก่อนค่อยสั่ง init
// วงจรภายในจอตั้งตัวช้ากว่า ESP32 บูตเสร็จ ถ้ายิงคำสั่งไปตอนจอยังไม่พร้อม
// คำสั่งจะหายไปเงียบๆ แล้วจอค้างดำทั้งที่สายดีอยู่
bool waitForDisplay() {
  for (int attempt = 1; attempt <= 20; attempt++) {
    Wire.beginTransmission(OLED_ADDR);
    if (Wire.endTransmission() == 0) {
      Serial.print("จอตอบรับในครั้งที่ ");
      Serial.println(attempt);
      return true;
    }
    delay(100);
  }
  Serial.println("จอไม่ตอบรับเลย -- เช็กสาย VCC GND SDA SCL");
  return false;
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== ทดสอบจอด้วยไดรเวอร์ SH1106 ===");

  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  Wire.setClock(400000);

  waitForDisplay();
  // พารามิเตอร์ตัวที่สองคือ reset ตั้ง false เพราะโมดูล I2C ไม่มีขา reset
  display.begin(OLED_ADDR, false);
  display.setContrast(OLED_CONTRAST);
  Serial.println("เริ่มจอแล้ว ถ้าจอเต็มและอ่านออก = เป็น SH1106 แน่นอน");
}

void loop() {
  // ลายที่ 1: กรอบชิดขอบ + เส้นทแยง ดูว่าเต็มจอจริงไหม
  display.clearDisplay();
  display.drawRect(0, 0, 128, 64, SH110X_WHITE);
  display.drawLine(0, 0, 127, 63, SH110X_WHITE);
  display.drawLine(127, 0, 0, 63, SH110X_WHITE);
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(28, 28);
  display.print("SH1106 OK");
  display.display();
  Serial.println("ลายที่ 1: กรอบ + เส้นทแยง");
  delay(3000);

  // ลายที่ 2: ขาวเต็มจอ ถ้าขาวหมดจริงแปลว่าเขียนครบทั้ง 8 หน้า
  display.clearDisplay();
  display.fillRect(0, 0, 128, 64, SH110X_WHITE);
  display.display();
  Serial.println("ลายที่ 2: ขาวเต็มจอ");
  delay(3000);

  // ลายที่ 3: แถบไล่ลงมา ดูว่าแถวล่างสุดมาถึงไหม
  display.clearDisplay();
  for (int y = 0; y < 64; y += 8) display.fillRect(0, y, 128, 4, SH110X_WHITE);
  display.display();
  Serial.println("ลายที่ 3: แถบแนวนอน");
  delay(3000);
}
