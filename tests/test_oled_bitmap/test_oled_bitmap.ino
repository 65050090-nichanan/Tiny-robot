// ======================================================
// ทดสอบว่ารูปที่แปลงมาขึ้นจอ SH1106 ได้จริงไหม
//
// สเก็ตช์นี้คือ tests/test_oled_sh1106 ที่ใช้ได้แล้ว บวกการวาด bitmap เข้าไปอย่างเดียว
// ไม่มี Wi-Fi ไม่มี WebSocket ไม่มี UDP ไม่มี Serial2 เพื่อตัดตัวแปรอื่นออกให้หมด
//
// ผลที่ได้บอกอะไร
//   เห็นรูปวนครบ 11 ใบ  -> โค้ดวาดจอกับไฟล์รูปไม่มีปัญหา ปัญหาอยู่ที่ส่วนอื่นของสเก็ตช์หลัก
//   จอดำตลอด            -> ปัญหาอยู่ที่การวาด bitmap เอง
//
// ต้องมีไลบรารี Adafruit SH110X และ Adafruit GFX
// ต่อสาย: SDA -> GPIO21, SCL -> GPIO22, VCC -> 3V3, GND -> GND
// ======================================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#include "animal_bitmaps.h"  // สำเนาจาก esp32_remote_controller/

const int OLED_SDA_PIN = 21;
const int OLED_SCL_PIN = 22;
const uint8_t OLED_ADDR = 0x3C;
const int LED_PIN = 2;   // ไฟ LED บนบอร์ด ESP32 DevKit ส่วนใหญ่อยู่ที่ขานี้
// ความสว่างของจอ 0x00 ถึง 0xFF -- คุมกระแสที่ป้อนให้แต่ละพิกเซลโดยตรง
// ค่ายิ่งต่ำยิ่งกินไฟน้อย ช่วยได้มากถ้าไฟเลี้ยงไม่นิ่งจนบอร์ดรีเซ็ตตัวเองตอนวาดรูปที่ติดไฟเยอะ
// 0x30 สว่างพออ่านได้สบายในร่ม ถ้าอยากสว่างขึ้นไล่ขึ้นทีละ 0x10
const uint8_t OLED_CONTRAST = 0x30;

Adafruit_SH1106G display(128, 64, &Wire, -1, 400000, 400000);

struct Frame {
  const char *name;
  const unsigned char *bmp;
};

const Frame FRAMES[] = {
  { "start",    bmp_start },
  { "scan1",    bmp_scan1 },
  { "scan2",    bmp_scan2 },
  { "scan3",    bmp_scan3 },
  { "scan4",    bmp_scan4 },
  { "detected", bmp_detected },
  { "dog",      bmp_dog },
  { "cat",      bmp_cat },
  { "bird",     bmp_bird },
  { "lion",     bmp_lion },
  { "tiger",    bmp_tiger },
};
const int FRAME_COUNT = sizeof(FRAMES) / sizeof(FRAMES[0]);

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
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  delay(400);
  Serial.println("\n=== ทดสอบวาด bitmap บนจอ SH1106 ===");

  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  Wire.setClock(400000);

  waitForDisplay();
  display.begin(OLED_ADDR, false);
  display.setContrast(OLED_CONTRAST);
  Serial.println("เริ่มจอแล้ว");

  // ยืนยันก่อนว่าวาดเส้นธรรมดายังขึ้นอยู่ ถ้าขั้นนี้ยังไม่ขึ้นก็ไม่ต้องดูขั้นต่อไป
  display.clearDisplay();
  display.drawRect(0, 0, 128, 64, SH110X_WHITE);
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(2);
  display.setCursor(14, 24);
  display.print("LINE OK");
  display.display();
  Serial.println("ขั้นที่ 1: วาดกรอบกับข้อความ -- ควรเห็นคำว่า LINE OK");
  delay(3000);
}

// เคาะถามจอว่ายังตอบอยู่ไหม โดยส่งคำสั่ง NOP ที่ไม่เปลี่ยนอะไรบนจอ
bool displayAlive() {
  Wire.beginTransmission(OLED_ADDR);
  Wire.write(0x00);
  Wire.write(0xE3);
  return Wire.endTransmission() == 0;
}

void loop() {
  for (int i = 0; i < FRAME_COUNT; i++) {
    display.clearDisplay();
    display.drawBitmap(0, 0, FRAMES[i].bmp, OLED_BMP_W, OLED_BMP_H, SH110X_WHITE);
    display.display();

    bool alive = displayAlive();
    Serial.print("แสดงรูป: ");
    Serial.print(FRAMES[i].name);
    Serial.println(alive ? "   จอตอบรับ" : "   จอไม่ตอบแล้ว");

    // ใช้ไฟ LED บนบอร์ดรายงานสถานะ จะได้อ่านผลได้ไม่ต้องเปิด Serial Monitor
    //   กะพริบช้าๆ เฟรมละครั้ง = โปรแกรมยังวิ่งอยู่ และจอยังตอบรับ
    //   กะพริบถี่รัวๆ            = โปรแกรมยังวิ่ง แต่จอหยุดตอบแล้ว
    //   ดับสนิทไม่ขยับ          = โปรแกรมค้างหรือบอร์ดรีเซ็ต
    if (alive) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(2000);
    } else {
      for (int k = 0; k < 20; k++) {
        digitalWrite(LED_PIN, k % 2);
        delay(100);
      }
    }
  }
  Serial.println("--- ครบรอบ เริ่มใหม่ ---");
}
