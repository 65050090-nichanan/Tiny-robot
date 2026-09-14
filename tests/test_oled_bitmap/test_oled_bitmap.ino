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

// ---------- ส่งภาพขึ้นจอเอง ไม่ผ่าน display() ของไลบรารี ----------
// ไลบรารีส่งภาพเป็นก้อนใหญ่ ถ้าสัญญาณบนสายไม่นิ่งพอ ก้อนจะขาดกลางคัน
// แล้วภาพเข้าจอไม่ครบเฟรม ซึ่งเป็นอาการที่เจออยู่
//
// ตัวนี้ส่งทีละหน้า (หนึ่งหน้า = 8 แถวพิกเซล) และแบ่งข้อมูลเป็นก้อนละ 16 ไบต์
// ก้อนเล็กลงทำให้แต่ละรายการสั้นลง โอกาสขาดกลางคันน้อยลง
// และถ้าหน้าไหนพลาด จะเสียแค่แถบเดียว ไม่ลามทั้งเฟรมเหมือนตอนส่งก้อนใหญ่
//
// ยังใช้ Adafruit_GFX วาดลงบัฟเฟอร์เหมือนเดิมทุกอย่าง เปลี่ยนแค่ขั้นตอนส่งออกจอ

const uint8_t OLED_COL_OFFSET = 2;  // SH1106 มี RAM 132 คอลัมน์ แต่จอจริงเริ่มที่คอลัมน์ 2
const int OLED_CHUNK = 16;          // ส่งข้อมูลภาพครั้งละกี่ไบต์

// ส่งคำสั่งหนึ่งไบต์ไปที่จอ
void oledCmd(uint8_t c) {
  Wire.beginTransmission(OLED_ADDR);
  Wire.write(0x00);   // ไบต์นำหน้าบอกว่าต่อไปเป็นคำสั่ง
  Wire.write(c);
  Wire.endTransmission();
}

// ส่งค่าตั้งต้นของจอใหม่ ก่อนวาดทุกเฟรม
//
// ถ้าสัญญาณขาดกลางคำสั่ง ชิปจอจะค้างรอไบต์ที่หายไป แล้วกลืนทุกอย่างที่ส่งตามมา
// เป็นพารามิเตอร์ของคำสั่งนั้น ข้อมูลภาพจึงไม่เคยเข้าถึงหน่วยความจำจอ
// ภาพค้างที่เฟรมเดิม ทั้งที่จอยังตอบรับทุกอย่างตามปกติ ต้องรีบอร์ดถึงจะหาย
//
// การส่งค่าตั้งต้นใหม่ทำให้ชิปหลุดจากสภาพนั้นเอง ทุกไบต์ที่ค้างจะถูกกลืนเป็น
// พารามิเตอร์ของคำสั่งชุดนี้แล้วจบไป หลังจากนั้นก็กลับมารับข้อมูลภาพได้ตามปกติ
//
// ราคาของมันคือคำสั่งราว 20 ไบต์ เทียบกับข้อมูลภาพ 1024 ไบต์ต่อเฟรม คือไม่ถึง 2%
// จงใจไม่ส่งคำสั่งปิดจอ (0xAE) นำหน้า เพราะจะทำให้ภาพกระพริบทุกเฟรม
void oledResync() {
  oledCmd(0xD5); oledCmd(0x80);   // ความถี่สแกน
  oledCmd(0xA8); oledCmd(0x3F);   // จำนวนแถวที่สแกน 64 แถว
  oledCmd(0xD3); oledCmd(0x00);   // ไม่เลื่อนภาพแนวตั้ง
  oledCmd(0x40);                  // เริ่มที่แถวบนสุด
  oledCmd(0xAD); oledCmd(0x8B);   // เปิดปั๊มแรงดัน แบบ SH1106
  oledCmd(0xA1);                  // กลับซ้ายขวา
  oledCmd(0xC8);                  // กลับบนล่าง
  oledCmd(0xDA); oledCmd(0x12);   // การจัดเรียงขา COM
  oledCmd(0x81); oledCmd(OLED_CONTRAST);
  oledCmd(0xA4);                  // แสดงตามข้อมูลในหน่วยความจำ
  oledCmd(0xA6);                  // สีปกติ ไม่กลับขาวดำ
  oledCmd(0xAF);                  // เปิดจอ (ถ้าเปิดอยู่แล้วไม่มีผลอะไร)
}

void pushFrame() {
  uint8_t *buf = display.getBuffer();
  if (!buf) return;

  oledResync();   // ปลดชิปจอออกจากสภาพค้างกลางคำสั่ง ถ้าบังเอิญค้างอยู่

  for (uint8_t page = 0; page < 64 / 8; page++) {
    oledCmd(0xB0 + page);                              // เลือกหน้าที่จะเขียน
    oledCmd(0x00 | (OLED_COL_OFFSET & 0x0F));          // คอลัมน์เริ่มต้น 4 บิตล่าง
    oledCmd(0x10 | ((OLED_COL_OFFSET >> 4) & 0x0F));   // คอลัมน์เริ่มต้น 4 บิตบน

    const uint8_t *row = buf + (int)page * 128;
    for (int x = 0; x < 128; x += OLED_CHUNK) {
      Wire.beginTransmission(OLED_ADDR);
      Wire.write(0x40);   // ไบต์นำหน้าบอกว่าต่อไปเป็นข้อมูลภาพ
      Wire.write(row + x, OLED_CHUNK);
      Wire.endTransmission();
    }
  }
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  delay(400);
  Serial.println("\n=== ทดสอบวาด bitmap บนจอ SH1106 ===");

  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  Wire.setClock(400000);

  delay(300);   // รอให้จอตั้งตัวเสร็จก่อนสั่ง init
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
  pushFrame();
  Serial.println("ขั้นที่ 1: วาดกรอบกับข้อความ -- ควรเห็นคำว่า LINE OK");
  delay(3000);
}

void loop() {
  for (int i = 0; i < FRAME_COUNT; i++) {
    display.clearDisplay();
    display.drawBitmap(0, 0, FRAMES[i].bmp, OLED_BMP_W, OLED_BMP_H, SH110X_WHITE);
    pushFrame();

    Serial.print("แสดงรูป: ");
    Serial.println(FRAMES[i].name);

    // ไฟ LED สลับทุกเฟรม ไว้ดูว่าโปรแกรมยังวิ่งอยู่ไหม โดยไม่ต้องเปิด Serial Monitor
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    delay(2000);
  }
  Serial.println("--- ครบรอบ เริ่มใหม่ ---");
}