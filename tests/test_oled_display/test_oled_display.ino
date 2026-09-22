// ======================================================
// ไล่หาความเร็ว I2C ที่จอรับไหว
//
// อาการที่ใช้ตัวนี้แก้: ภาพขึ้นแค่ส่วนบนของจอ ส่วนล่างเป็นหิมะค้างอยู่
// แปลว่าข้อมูลภาพ 1024 ไบต์ส่งไปได้บางส่วนแล้วขาดกลางทาง
//
// สเก็ตช์นี้จะวาด "ขาวเต็มจอ" ที่ความเร็วต่างกันทีละระดับ ระดับละ 3 วินาที
// พร้อมพิมพ์ความเร็วออก Serial Monitor (115200)
// ให้จ้องจอแล้วจำไว้ว่า "ระดับไหนที่ขาวเต็มจอโดยไม่มีหิมะเหลือ"
//
// ต่อสาย: SDA -> GPIO21, SCL -> GPIO22, VCC -> 3V3, GND -> GND
// ======================================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const int OLED_SDA_PIN = 21;
const int OLED_SCL_PIN = 22;
const int OLED_W = 128;
const int OLED_H = 64;
const uint8_t OLED_ADDR = 0x3C;

// ไล่จากเร็วไปช้า ระดับที่ภาพเต็มจอครั้งแรกคือค่าที่ควรใช้จริง (เผื่อความปลอดภัยลดอีกขั้น)
const uint32_t SPEEDS[] = { 400000, 200000, 100000, 50000, 20000 };
const int SPEED_COUNT = sizeof(SPEEDS) / sizeof(SPEEDS[0]);

Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1);

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== ไล่หาความเร็ว I2C ที่จอรับไหว ===");

  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  Wire.setClock(100000);  // ตอน init ใช้ความเร็วปลอดภัยไว้ก่อน

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("display.begin() ไม่ผ่าน -> จอไม่ตอบสนอง หยุดทำงาน");
    while (true) delay(1000);
  }
  Serial.println("display.begin() ผ่าน เริ่มไล่ความเร็ว");
  Serial.println("ดูที่จอ: ระดับไหนขาวเต็มจอไม่มีหิมะเหลือ = ระดับนั้นใช้ได้");
}

void loop() {
  for (int i = 0; i < SPEED_COUNT; i++) {
    uint32_t hz = SPEEDS[i];
    Serial.printf("\n[%d/%d] ทดสอบที่ %lu Hz (%lu kHz)\n", i + 1, SPEED_COUNT,
                  (unsigned long)hz, (unsigned long)hz / 1000);

    Wire.setClock(hz);

    // ล้างจอด้วยสีดำก่อน เพื่อลบภาพของรอบที่แล้วออกให้หมด
    // ถ้าความเร็วนี้ส่งไม่รอด ส่วนที่ลบไม่ทันจะค้างเป็นภาพเก่าให้เห็นชัด
    display.clearDisplay();
    display.display();
    delay(400);

    // วาดขาวเต็มจอ เว้นกรอบดำ 2 พิกเซลไว้ดูขอบ
    display.fillScreen(SSD1306_WHITE);
    display.fillRect(2, 2, OLED_W - 4, 12, SSD1306_BLACK);
    display.setTextColor(SSD1306_BLACK);
    display.setTextSize(1);
    display.setCursor(6, 4);
    display.printf("%lu kHz", (unsigned long)hz / 1000);

    unsigned long t0 = micros();
    display.display();
    unsigned long took = micros() - t0;
    Serial.printf("      ส่งข้อมูล 1024 ไบต์ใช้เวลา %lu us\n", took);

    delay(3000);
  }
  Serial.println("\n--- ครบรอบแล้ว เริ่มใหม่ ---");
}
