#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiUdp.h>

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

const char *ssid = "NICHANAN";
const char *password = "ni0825763371";

WiFiUDP udp;
unsigned int localPort = 1234;

void startCameraServer(); 

void setup() {
  Serial.begin(115200); 
  Serial1.begin(9600, SERIAL_8N1, 15, 14); // TX พิน 14 ส่งไป STM32

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
  config.frame_size = FRAMESIZE_QVGA; // ลดขนาดภาพเพื่อกันค้าง
  config.jpeg_quality = 15; config.fb_count = 2;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) return;

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  
  startCameraServer();
  udp.begin(localPort);
}

void loop() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char incoming = udp.read();
    // ถ้าเจอสัตว์ชนิดใดก็ตาม ส่ง 'S' ไปสั่ง STM32
    if (incoming == 'D' || incoming == 'C' || incoming == 'B' || incoming == 'L' || incoming == 'T') { 
      Serial1.write('S'); 
      Serial.print("Animal: "); Serial.print(incoming); Serial.println(" -> SENT STOP");
    }
  }
  delay(1);
}