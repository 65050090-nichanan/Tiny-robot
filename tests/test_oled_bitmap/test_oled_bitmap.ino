#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "animal_bitmaps.h"

#define OLED_SDA   21
#define OLED_SCL   22
#define OLED_ADDR  0x3C

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

struct Frame {
  const char *name;
  const unsigned char *bmp;
};

const Frame FRAMES[] = {
  {"start",    bmp_start},
  {"scan1",    bmp_scan1},
  {"scan2",    bmp_scan2},
  {"scan3",    bmp_scan3},
  {"scan4",    bmp_scan4},
  {"detected", bmp_detected},
  {"dog",      bmp_dog},
  {"cat",      bmp_cat},
  {"bird",     bmp_bird},
  {"lion",     bmp_lion},
  {"tiger",    bmp_tiger}
};

const int FRAME_COUNT = sizeof(FRAMES) / sizeof(FRAMES[0]);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(100000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("ไม่สามารถเริ่มจอ SSD1306 ได้");
    while (true) {
      delay(1000);
    }
  }

  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.display();

  Serial.println("เริ่มทดสอบ Bitmap");
}

void loop() {
  for (int i = 0; i < FRAME_COUNT; i++) {
    display.clearDisplay();

    display.drawBitmap(
      0,
      0,
      FRAMES[i].bmp,
      OLED_BMP_W,
      OLED_BMP_H,
      SSD1306_WHITE
    );

    display.display();

    Serial.print("แสดงรูป: ");
    Serial.println(FRAMES[i].name);

    delay(2000);
  }

  Serial.println("--- ครบรอบ เริ่มใหม่ ---");
}