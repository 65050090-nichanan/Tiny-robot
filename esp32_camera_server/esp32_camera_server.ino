// ======================================================
// ESP32-CAM: กล้องของหุ่น + สะพานส่งสัญญาณหยุดจาก QR
//
// หน้าที่ของบอร์ดนี้
//   1. เกาะ Wi-Fi ชื่อ My_Robot5 ที่ ESP32 รีโมทปล่อยออกมา (เมื่อก่อนเกาะเราเตอร์บ้าน คนละวงกับรีโมท)
//   2. เปิดเว็บกล้อง: ภาพนิ่งที่ http://<ip>/capture และภาพสดที่ http://<ip>:81/stream
//   3. รับรหัสสัตว์จาก PC ทาง UDP พอร์ต 1234 แล้วส่ง 'S' ให้ STM32 หยุด
//   4. ส่งรหัสสัตว์ตัวเดียวกันต่อไปให้ ESP32 รีโมท เพื่อวาดรูปสัตว์ขึ้นจอ OLED
//
// การต่อสาย
//   GPIO14 (TX) -> STM32 Serial3 RX (PB11)   ต้องต่อ GND ร่วมกันด้วย
// ======================================================

#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiUdp.h>

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// ---------- ค่าคงที่ตั้งค่าได้ ----------
const char *ssid = "My_Robot9";           // ต้องตรงกับ AP_SSID ใน esp32_remote_controller.ino
const char *password = "password1234";   // ต้องตรงกับ AP_PASSWORD ใน esp32_remote_controller.ino

const unsigned int LOCAL_UDP_PORT = 1234;   // พอร์ตที่รอรับรหัสสัตว์จาก PC
const unsigned int REMOTE_UDP_PORT = 1235;  // พอร์ตของ ESP32 รีโมทที่รอรับรหัสสัตว์
const unsigned long PING_INTERVAL_MS = 2000;  // ส่งสัญญาณบอกว่ายังอยู่ทุกกี่มิลลิวินาที

// รีโมทส่ง 'F' ตอนเข้า AUTO และ 'V' ตอนกลับ MANUAL มาที่พอร์ต 1234 เดิม
// โปรไฟล์ AUTO ลดจำนวนพิกเซลต่อเฟรม จึงส่งภาพใหม่ได้ทันและไม่สะสมภาพเก่าในเบราว์เซอร์
const char CAMERA_PROFILE_FAST = 'F';
const char CAMERA_PROFILE_DETAIL = 'V';
const framesize_t DETAIL_FRAME_SIZE = FRAMESIZE_VGA;
const int DETAIL_JPEG_QUALITY = 12;
const framesize_t FAST_FRAME_SIZE = FRAMESIZE_CIF;  // 400x296: คมกว่า QVGA แต่เบากว่า VGA มาก
const int FAST_JPEG_QUALITY = 10;

const int STM32_TX_PIN = 14;  // ขาส่งข้อมูลไป STM32
const int STM32_RX_PIN = 15;  // ขารับข้อมูลจาก STM32 (ยังไม่ได้ใช้ แต่จองไว้)

WiFiUDP udp;
IPAddress remoteIp;                 // IP ของ ESP32 รีโมท = เกตเวย์ของ AP
unsigned long lastPing = 0;         // เวลาที่ส่ง ping ครั้งล่าสุด

void startCameraServer();

// รหัสสัตว์ที่ยอมรับ ตรงกับ ANIMAL_MAP ใน pc_qr_scanner/qr_scanner.py
bool isAnimalCode(char c) {
  return c == 'D' || c == 'C' || c == 'B' || c == 'L' || c == 'T';
}

// เปลี่ยนความละเอียดระหว่างเฟรม ไม่ต้องรีสตาร์ตกล้องหรือทำให้การเชื่อมต่อ /stream หลุด
void applyCameraProfile(bool fast) {
  sensor_t *sensor = esp_camera_sensor_get();
  if (!sensor) return;

  sensor->set_framesize(sensor, fast ? FAST_FRAME_SIZE : DETAIL_FRAME_SIZE);
  sensor->set_quality(sensor, fast ? FAST_JPEG_QUALITY : DETAIL_JPEG_QUALITY);
  Serial.printf("Camera profile: %s\n", fast ? "FAST (CIF)" : "DETAIL (VGA)");
}

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, STM32_RX_PIN, STM32_TX_PIN);  // ช่องส่ง 'S' ไป STM32

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM; config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM; config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM; config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM; config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM; config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM; config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM; config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM; config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000; config.pixel_format = PIXFORMAT_JPEG;
  // เริ่มด้วย VGA สำหรับอ่าน QR ตอนจอดนิ่ง แล้วให้รีโมทสลับเป็น CIF ตอน AUTO
  // กล้องจองสองบัฟเฟอร์และส่งภาพล่าสุดเสมอ จึงไม่ค้างแสดงภาพในอดีตเมื่อหุ่นกำลังวิ่ง
  config.frame_size = DETAIL_FRAME_SIZE;
  config.jpeg_quality = DETAIL_JPEG_QUALITY;
  config.fb_count = 2;
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = CAMERA_FB_IN_PSRAM;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("เปิดกล้องไม่สำเร็จ (error 0x%x)\n", err);
    return;
  }

  WiFi.begin(ssid, password);
  Serial.print("กำลังต่อ Wi-Fi ");
  Serial.print(ssid);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println();
  WiFi.setSleep(false);  // ไม่ให้ Wi-Fi STA หลับระหว่างส่ง MJPEG ต่อเนื่อง

  remoteIp = WiFi.gatewayIP();  // เกตเวย์ของ AP ก็คือ ESP32 รีโมท (ปกติคือ 192.168.4.1)
  Serial.print("IP กล้อง: ");   Serial.println(WiFi.localIP());
  Serial.print("IP รีโมท: ");   Serial.println(remoteIp);
  Serial.print("ภาพสด: http://"); Serial.print(WiFi.localIP()); Serial.println(":81/stream");

  startCameraServer();
  udp.begin(LOCAL_UDP_PORT);
}

void loop() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char incoming = udp.read();
    if (incoming == CAMERA_PROFILE_FAST) {
      applyCameraProfile(true);
    } else if (incoming == CAMERA_PROFILE_DETAIL) {
      applyCameraProfile(false);
    } else if (isAnimalCode(incoming)) {
      Serial1.write('S');  // บอก STM32 ให้หยุด 5 วินาที

      // ส่งรหัสสัตว์ตัวจริงให้รีโมท เพื่อให้จอ OLED รู้ว่าเป็นสัตว์อะไร
      udp.beginPacket(remoteIp, REMOTE_UDP_PORT);
      udp.write((uint8_t)incoming);
      udp.endPacket();

      Serial.print("Animal: "); Serial.print(incoming); Serial.println(" -> STOP + OLED");
    }
  }

  // ส่งสัญญาณบอกรีโมทเป็นระยะว่ากล้องยังอยู่ รีโมทจะได้รู้ IP ไว้ทำลิงก์ภาพสด
  if (WiFi.status() == WL_CONNECTED && millis() - lastPing > PING_INTERVAL_MS) {
    lastPing = millis();
    udp.beginPacket(remoteIp, REMOTE_UDP_PORT);
    udp.write((uint8_t)'P');
    udp.endPacket();
  }

  delay(1);
}
