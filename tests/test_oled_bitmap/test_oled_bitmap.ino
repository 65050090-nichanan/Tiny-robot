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
// ต่อสาย: SDA -> GPIO32, SCL -> GPIO33, VCC -> 3V3, GND -> GND
// ======================================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#include "animal_bitmaps.h"  // สำเนาจาก esp32_remote_controller/

const int OLED_SDA_PIN = 32;
const int OLED_SCL_PIN = 33;
const uint8_t OLED_ADDR = 0x3C;

Adafruit_SH1106G display(128, 64, &Wire, -1);

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

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println("\n=== ทดสอบวาด bitmap บนจอ SH1106 ===");

  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  Wire.setClock(100000);

  display.begin(OLED_ADDR, false);
  display.setContrast(0x7F);
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

void loop() {
  for (int i = 0; i < FRAME_COUNT; i++) {
    display.clearDisplay();
    display.drawBitmap(0, 0, FRAMES[i].bmp, OLED_BMP_W, OLED_BMP_H, SH110X_WHITE);
    display.display();
    Serial.printf("แสดงรูป: %s\n", FRAMES[i].name);
    delay(2000);
  }
  Serial.println("--- ครบรอบ เริ่มใหม่ ---");
}
