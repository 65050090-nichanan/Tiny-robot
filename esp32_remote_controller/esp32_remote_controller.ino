// ======================================================
// ESP32 Remote: Wi-Fi AP + หน้าเว็บบังคับหุ่น + ภาพสดจากกล้อง + จอ OLED โชว์รูปสัตว์
//
// หน้าที่ของบอร์ดนี้
//   1. ปล่อย Wi-Fi AP ชื่อ My_Robot ให้มือถือและ ESP32-CAM มาเกาะ
//   2. เสิร์ฟหน้าเว็บบังคับหุ่น พร้อมฝังภาพสดจากกล้อง
//   3. ส่งคำสั่งจากหน้าเว็บต่อไปให้ STM32 ทาง Serial2 (นี่คือส่วนที่เดิมขาดไป หุ่นเลยไม่ขยับ)
//   4. รับรหัสสัตว์จาก ESP32-CAM ทาง UDP แล้วเล่นภาพบนจอ OLED เป็นลำดับ
//
// จอ OLED มี 4 หน้า สลับกันเองตามสถานการณ์
//   ต้อนรับ  - รูปหุ่นยนต์ 2.5 วินาทีแรกหลังบูต
//   สถานะ    - MODE / SPEED / IP กล้อง ตอนหุ่นจอดนิ่ง
//   สแกน     - QR จำลองมีแถบกวาดเลื่อนวน เล่นตลอดเวลาที่หุ่นเคลื่อนที่
//   เจอแล้ว  - เครื่องหมายถูกกระพริบ 1 วินาที แล้วเปลี่ยนเป็นรูปสัตว์จนครบ 5 วินาที
//
// การต่อสาย
//   OLED SSD1306 (I2C):  SDA -> GPIO21, SCL -> GPIO22, VCC -> 3V3, GND -> GND
//   ไปหา STM32 (UART2):  GPIO17 (TX) -> STM32 Serial2 RX (PA3)
//                        GPIO16 (RX) <- STM32 Serial2 TX (PA2)   (ไม่ใช้ก็ได้)
//                        ต้องต่อ GND ร่วมกันด้วย
// ======================================================

#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>

// ---------- เลือกชิปของจอ ----------
// จอ OLED I2C หน้าตาเหมือนกันแต่ข้างในมีสองชิปที่คุยกันคนละแบบ
//   0 = SSD1306  พบมากในจอ 0.96 นิ้ว
//   1 = SH1106   พบมากในจอ 1.3 นิ้ว  (ต้องลงไลบรารี "Adafruit SH110X" ก่อน)
//
// อาการเมื่อตั้งเป็น SSD1306 ทั้งที่จอจริงเป็น SH1106:
//   ภาพขึ้นแค่แถบบนสุด ส่วนที่เหลือเป็นจุดขาวดำมั่วค้างอยู่
//   เพราะ SH1106 ไม่รองรับโหมด horizontal addressing ที่ไลบรารี SSD1306 ใช้
//   ข้อมูลทั้งเฟรมเลยไปกองอยู่ที่หน้าแรกหน้าเดียว
// ทดสอบว่าจอเป็นตัวไหนได้ด้วย tests/test_oled_sh1106/ หรือ tests/test_oled_raw/
//
// จอของหุ่นตัวนี้ทดสอบแล้วเป็น SH1106 จึงตั้งเป็น 1
#define OLED_DRIVER_SH1106 1

#if OLED_DRIVER_SH1106
  #include <Adafruit_SH110X.h>
  #define OLED_WHITE SH110X_WHITE
#else
  #include <Adafruit_SSD1306.h>
  #define OLED_WHITE SSD1306_WHITE
#endif

#include "animal_bitmaps.h"  // รูปเต็มจอ 128x64 สร้างจาก tools/png_to_bitmaps.py

// ---------- ค่าคงที่ตั้งค่าได้ ----------
const char *AP_SSID = "My_Robot2";        // ชื่อ Wi-Fi ที่หุ่นปล่อยออกมา
const char *AP_PASSWORD = "password1234";  // รหัสผ่าน Wi-Fi (ต้องยาวอย่างน้อย 8 ตัว)

const int STM32_RX_PIN = 16;  // ขา RX ของ ESP32 รับข้อมูลจาก STM32
const int STM32_TX_PIN = 17;  // ขา TX ของ ESP32 ส่งคำสั่งไป STM32
const long STM32_BAUD = 115200;  // ความเร็วสื่อสารกับ STM32 ต้องตรงกับฝั่ง STM32

const int OLED_SDA_PIN = 21;  // ขา SDA ของจอ OLED
const int OLED_SCL_PIN = 22;  // ขา SCL ของจอ OLED
const uint8_t OLED_ADDR = 0x3C;  // แอดเดรส I2C ของจอ (บางรุ่นเป็น 0x3D)
// ความเร็ว I2C ตอนสแกนหาจอก่อนเริ่มไลบรารี
// หมายเหตุ: ไลบรารี SH110X ตั้งความเร็วของมันเอง (ค่าเริ่มต้น 400 kHz) ตอนส่งภาพ
// ค่านี้จึงมีผลแค่ช่วงก่อน begin() เท่านั้น
// I2C Fast Mode ที่ส่งเข้าคอนสตรัคเตอร์ด้วย ไลบรารีจะได้ไม่ตั้งค่าเอง
// ที่ความเร็วนี้ภาพหนึ่งเฟรม 1024 ไบต์ใช้เวลาราว 23 ms ถ้าลดเหลือ 100000 จะกลายเป็น 92 ms
// ซึ่งกินเวลาของ loop จนหน้าเว็บและภาพเคลื่อนไหวหน่วงตามกันไปหมด
// ถ้าจอเพี้ยนหรือหลุดหลังเพิ่มความเร็ว ให้ลองลดเป็น 200000 ก่อน
const uint32_t OLED_I2C_HZ = 400000;
const int OLED_BOOT_WAIT_MS = 300;         // รอกี่มิลลิวินาทีก่อนสั่ง init ให้จอตั้งตัวเสร็จ
// ความสว่างของจอ 0x00 ถึง 0xFF -- คุมกระแสที่ป้อนให้แต่ละพิกเซลโดยตรง
// ค่ายิ่งต่ำยิ่งกินไฟน้อย ช่วยได้มากถ้าไฟเลี้ยงไม่นิ่งจนบอร์ดรีเซ็ตตัวเองตอนวาดรูปที่ติดไฟเยอะ
// 0x30 สว่างพออ่านได้สบายในร่ม ถ้าอยากสว่างขึ้นไล่ขึ้นทีละ 0x10
const uint8_t OLED_CONTRAST = 0x30;
const int OLED_W = 128;  // ความกว้างจอเป็นพิกเซล
const int OLED_H = 64;   // ความสูงจอเป็นพิกเซล

const unsigned int ANIMAL_UDP_PORT = 1235;  // พอร์ต UDP ที่รอรับรหัสสัตว์จาก ESP32-CAM
const unsigned long ANIMAL_SHOW_MS = 5000;  // เวลารวมของช่วง "เจอแล้ว" + รูปสัตว์ (ให้เท่ากับเวลาที่หุ่นหยุด)
const unsigned long DETECT_SHOW_MS = 1000;  // ในช่วงนั้น กระพริบภาพ "เจอแล้ว" กี่มิลลิวินาทีก่อนเปลี่ยนเป็นรูปสัตว์
const unsigned long START_SCREEN_MS = 2500;  // ค้างหน้าต้อนรับกี่มิลลิวินาทีนับจากบูต
const unsigned long CAM_TIMEOUT_MS = 10000;  // ถ้าไม่ได้ข่าวจากกล้องเกินเท่านี้ ถือว่ากล้องหลุด

// ---------- วัตถุหลัก ----------
WebServer server(80);                               // เว็บเซิร์ฟเวอร์เสิร์ฟหน้า HTML
WebSocketsServer webSocket = WebSocketsServer(81);  // ช่องทางส่งคำสั่งเรียลไทม์จากหน้าเว็บ
WiFiUDP udp;                                        // ตัวรับ UDP จาก ESP32-CAM
// -1 คือไม่ใช้ขา reset สองตัวท้ายคือความเร็ว I2C ตอนส่งข้อมูลและตอนพัก
// ถ้าไม่ใส่ ไลบรารีจะตั้ง 400 kHz ให้เองโดยไม่สนค่าที่เราตั้งไว้ใน Wire.setClock
#if OLED_DRIVER_SH1106
Adafruit_SH1106G display(OLED_W, OLED_H, &Wire, -1, OLED_I2C_HZ, OLED_I2C_HZ);
#else
Adafruit_SSD1306 display(OLED_W, OLED_H, &Wire, -1, OLED_I2C_HZ, OLED_I2C_HZ);
#endif

// ---------- สถานะระบบ ----------
bool isAutoMode = false;   // โหมดปัจจุบัน (true = AUTO, false = MANUAL)
int robotSpeed = 180;      // ความเร็วล่าสุดที่ตั้งจากสไลเดอร์ (ต้องตรงกับ baseSpeed ฝั่ง STM32)
bool hasOled = false;      // เจอจอ OLED ตอนบูตหรือไม่ ถ้าไม่เจอจะข้ามการวาดทั้งหมด

String lastTelemetry = "";         // บรรทัดสถานะล่าสุดจาก STM32 ให้ PC มาดึงไปเก็บลง CSV

String camIp = "";                 // IP ของ ESP32-CAM ที่เรียนรู้มาจากแพ็กเก็ต UDP
unsigned long lastCamSeen = 0;     // เวลาล่าสุดที่ได้ยินเสียงกล้อง

int currentAnimal = -1;            // ดัชนีสัตว์ที่กำลังโชว์อยู่ (-1 = ไม่มี)
unsigned long animalUntil = 0;     // โชว์รูปสัตว์ไปจนถึงเวลานี้ (millis)

// การวาดจอ OLED หนึ่งเฟรมต้องส่งข้อมูล 1KB ผ่าน I2C ถ้าวาดทุกครั้งที่ขยับสไลเดอร์จะหน่วง
// จึงแค่ตั้งธงไว้ แล้วให้ loop() ค่อยวาดจริงแบบจำกัดจำนวนครั้งต่อวินาที
bool idleDirty = false;            // มีสถานะเปลี่ยน รอวาดจอใหม่
unsigned long lastIdleDraw = 0;    // เวลาที่วาดหน้าสถานะครั้งล่าสุด
const unsigned long IDLE_DRAW_MS = 120;  // วาดหน้าสถานะถี่สุดทุกกี่มิลลิวินาที

bool isMoving = false;             // หุ่นกำลังเคลื่อนที่อยู่ไหม ดูจากคำสั่งล่าสุดที่ส่งไป STM32
unsigned long detectUntil = 0;     // กระพริบภาพ "เจอแล้ว" ไปจนถึงเวลานี้
bool animalDrawn = false;          // วาดรูปสัตว์ไปแล้วหรือยัง จะได้ไม่วาดซ้ำทุกรอบ loop
bool detectDrawn = false;          // วาดภาพ "เจอแล้ว" ไปแล้วหรือยัง
int animFrame = 0;                 // เฟรมปัจจุบันของภาพเคลื่อนไหว
unsigned long lastAnimDraw = 0;    // เวลาที่วาดเฟรมภาพเคลื่อนไหวครั้งล่าสุด

// ภาพเคลื่อนไหวชุดกำลังสแกน เล่นวนไปเรื่อยๆ ตอนหุ่นเคลื่อนที่
// ภาพชุดที่เล่นวนตอนหุ่นเคลื่อนที่ เล่นไปเรื่อยๆ จนกว่าจะเจอ QR
//
// แต่ละเฟรมกำหนดเวลาของตัวเองได้ เพราะภาพชุดนี้เป็นการกวาดสายตามองหา ไม่ใช่ภาพเคลื่อนไหวจังหวะเท่ากัน
// ตอนมองไปข้างใดข้างหนึ่งต้องค้างไว้พอให้เห็นว่ากำลังมองอะไรอยู่ ส่วนตอนกลับมามองตรงเป็นจังหวะพักสายตา
struct ScanFrame {
  const unsigned char *bitmap;
  unsigned long ms;      // ค้างภาพนี้ไว้กี่มิลลิวินาทีก่อนไปเฟรมถัดไป
};

const ScanFrame SCAN_FRAMES[] = {
  { bmp_scan1, 600 },    // มองตรง
  { bmp_scan2, 450 },    // กวาดไปทางซ้าย
  { bmp_scan3, 450 },    // แล้วกวาดไปทางขวา
};
const int SCAN_FRAME_COUNT = sizeof(SCAN_FRAMES) / sizeof(SCAN_FRAMES[0]);



// ---------- ตารางสัตว์: รหัส 1 ตัวอักษร -> ชื่อ + รูป ----------
struct Animal {
  char code;                     // รหัสที่ ESP32-CAM ส่งมา
  const char *name;              // ชื่อที่พิมพ์ขึ้นจอและหน้าเว็บ
  const unsigned char *bitmap;   // รูป 64x64 ใน PROGMEM
};

const Animal ANIMALS[] = {
  { 'D', "DOG",   bmp_dog },
  { 'C', "CAT",   bmp_cat },
  { 'B', "BIRD",  bmp_bird },
  { 'L', "LION",  bmp_lion },
  { 'T', "TIGER", bmp_tiger },
};
const int ANIMAL_COUNT = sizeof(ANIMALS) / sizeof(ANIMALS[0]);

// ---------- HTML หน้าควบคุม ----------
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <style>
        * { box-sizing: border-box; -webkit-user-select: none; touch-action: manipulation; }
        body { font-family: sans-serif; text-align: center; background-color: #1a1a2e; color: white; margin: 0; padding: 16px; }
        h2 { margin: 0 0 14px 0; }
        .card { background: #16213e; padding: 14px; border-radius: 15px; margin: 0 auto 16px auto; max-width: 340px; border: 1px solid #0f3460; }
        #cam-wrap { position: relative; max-width: 340px; margin: 0 auto 16px auto; }
        #cam { width: 100%; border-radius: 15px; background: #0f3460; display: block; min-height: 120px; }
        #cam-msg { padding: 30px 10px; color: #8a8aa8; font-size: 14px; }
        #animal-badge { position: absolute; left: 50%; transform: translateX(-50%); bottom: 10px;
                        background: rgba(15,157,88,0.92); color: #fff; font-size: 20px; font-weight: bold;
                        padding: 8px 22px; border-radius: 30px; display: none; }
        #status-text { color: #e94560; font-weight: bold; }
        .grid { display: grid; grid-template-columns: repeat(3, 1fr); grid-gap: 14px; max-width: 300px; margin: 0 auto 16px auto; }
        .btn { width: 100%; aspect-ratio: 1/1; border-radius: 20px; font-size: 28px; color: white; border: none; cursor: pointer; background-color: #0f3460; }
        .btn:active { background-color: #2980b9; }
        .btn-toggle { background-color: #e94560; font-size: 16px; font-weight: bold; }
        .btn-toggle.auto-on { background-color: #0f9d58; }
        .empty { visibility: hidden; }
        .slider { width: 100%; height: 10px; border-radius: 5px; background: #0f3460; outline: none; -webkit-appearance: none; }
        .slider::-webkit-slider-thumb { -webkit-appearance: none; width: 25px; height: 25px; border-radius: 50%; background: #e94560; border: 2px solid white; }
        #link { color: #8a8aa8; font-size: 12px; }
    </style>
</head>
<body>
    <h2>TINY ROBOT</h2>

    <div id="cam-wrap">
        <div id="cam-msg">กำลังรอกล้องเชื่อมต่อ...</div>
        <img id="cam" style="display:none">
        <div id="animal-badge"></div>
    </div>

    <div class="card">
        MODE: <span id="status-text">MANUAL</span> | SPEED: <span id="speed-val">180</span><br>
        <span id="link">CAM: -</span>
    </div>

    <div class="grid">
        <div class="empty"></div>
        <button class="btn" onmousedown="sendCmd('forward')" onmouseup="sendCmd('stop')" ontouchstart="sendCmd('forward')" ontouchend="sendCmd('stop')">&#9650;</button>
        <div class="empty"></div>
        <button class="btn" onmousedown="sendCmd('left')" onmouseup="sendCmd('stop')" ontouchstart="sendCmd('left')" ontouchend="sendCmd('stop')">&#9664;</button>
        <button id="main-btn" class="btn btn-toggle" onclick="toggleMode()">STOP<br><small>MANUAL</small></button>
        <button class="btn" onmousedown="sendCmd('right')" onmouseup="sendCmd('stop')" ontouchstart="sendCmd('right')" ontouchend="sendCmd('stop')">&#9654;</button>
        <div class="empty"></div>
        <button class="btn" onmousedown="sendCmd('backward')" onmouseup="sendCmd('stop')" ontouchstart="sendCmd('backward')" ontouchend="sendCmd('stop')">&#9660;</button>
        <div class="empty"></div>
    </div>

    <div class="card">
        <label>Speed: <span id="speed-val2">180</span></label><br><br>
        <!-- ลากลงสุดถึง 0 ได้ 0 คือสั่งจอด ส่วนค่าระหว่าง 1 ถึง 134 ฝั่ง STM32 จะดันขึ้นเป็น 135
             เพราะต่ำกว่านั้นมอเตอร์ได้แค่คราง ไม่ได้หมุนช้าลง -->
        <input type="range" min="0" max="255" value="180" class="slider" oninput="updateSpeed(this.value)">
    </div>

<script>
var websocket;
var isAuto = false;
var camUrl = "";
var badgeTimer = null;

function initWebSocket() {
    websocket = new WebSocket('ws://' + window.location.hostname + ':81/');
    websocket.onmessage = onMessage;
    websocket.onopen = function() { document.getElementById('link').innerText = 'CAM: ' + (camUrl ? 'ok' : '-'); };
    // ถ้าสายหลุดให้ลองต่อใหม่เอง หน้าเว็บจะได้ไม่ตายถาวร
    websocket.onclose = function() { setTimeout(initWebSocket, 2000); };
}

function onMessage(evt) {
    var msg = evt.data;
    if (msg.startsWith('CAMIP:')) {
        setCamera(msg.substring(6));
    } else if (msg.startsWith('ANIMAL:')) {
        showAnimal(msg.substring(7));
    }
}

function setCamera(ip) {
    document.getElementById('link').innerText = ip ? ('CAM: ' + ip) : 'CAM: -';
    var img = document.getElementById('cam');
    var msg = document.getElementById('cam-msg');
    if (!ip) {                       // กล้องหลุด กลับไปแสดงข้อความรอ
        camUrl = "";
        img.style.display = 'none';
        img.removeAttribute('src');
        msg.style.display = 'block';
        return;
    }
    var url = 'http://' + ip + ':81/stream';
    if (url === camUrl) return;      // IP เดิม ไม่ต้องโหลดสตรีมใหม่
    camUrl = url;
    img.onload = function() { msg.style.display = 'none'; img.style.display = 'block'; };
    img.src = url;
}

function showAnimal(name) {
    var badge = document.getElementById('animal-badge');
    badge.innerText = '🐾 ' + name;
    badge.style.display = 'block';
    if (badgeTimer) clearTimeout(badgeTimer);
    badgeTimer = setTimeout(function() { badge.style.display = 'none'; }, 5000);
}

function sendCmd(cmd) {
    if (!websocket || websocket.readyState !== 1) return;
    if (!isAuto || cmd === 'stop') websocket.send(cmd);
}

function toggleMode() {
    if (!websocket || websocket.readyState !== 1) {   // ยังต่อไม่ติด กดไปก็ไม่มีอะไรเกิดขึ้น
        document.getElementById('link').innerText = 'ยังต่อกับหุ่นไม่ติด รอสักครู่...';
        return;
    }
    isAuto = !isAuto;
    var btn = document.getElementById('main-btn');
    var status = document.getElementById('status-text');
    if (isAuto) {
        btn.classList.add('auto-on');
        btn.innerHTML = "AUTO<br><small>RUNNING</small>";
        status.innerText = "AUTO";
        status.style.color = "#0f9d58";
        websocket.send("MODE:AUTO");
    } else {
        btn.classList.remove('auto-on');
        btn.innerHTML = "STOP<br><small>MANUAL</small>";
        status.innerText = "MANUAL";
        status.style.color = "#e94560";
        websocket.send("MODE:MANUAL");
        websocket.send("stop");
    }
}

function updateSpeed(s) {
    document.getElementById('speed-val').innerText = s;
    document.getElementById('speed-val2').innerText = s;
    if (websocket && websocket.readyState === 1) websocket.send("SPEED:" + s);
}

window.onload = initWebSocket;
</script>
</body>
</html>
)rawliteral";

// ======================================================
// จอ OLED
// ======================================================

// จอจะตอบรับก็ต่อเมื่อวงจรภายในตั้งตัวเสร็จ ซึ่งช้ากว่าตอน ESP32 บูตเสร็จ
// ถ้ายิงคำสั่ง init ไปตอนจอยังไม่พร้อม คำสั่งจะหายไปเงียบๆ แล้วจอค้างดำทั้งที่สายดีอยู่
// อาการคือเดี๋ยวขึ้นเดี๋ยวไม่ขึ้น เปลี่ยนไปตามจังหวะที่เสียบไฟ
// จึงต้องเคาะถามก่อนว่าจอตอบรับหรือยัง แล้วค่อยสั่ง init
// ไล่เรียกทุกแอดเดรสบนบัสแล้วรายงานว่าใครตอบบ้าง
// ใช้ตอนหาจอไม่เจอ เพื่อแยกว่า "ไม่มีอะไรต่ออยู่เลย" กับ "มีจอแต่คนละแอดเดรส"
void scanI2CBus() {
  int found = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.print("  เจออุปกรณ์ที่ 0x");
      Serial.println(a, HEX);
      found++;
    }
  }
  if (found == 0) {
    Serial.println("  ไม่เจออุปกรณ์ใดๆ บนบัสเลย");
    Serial.print("  เช็กว่าสายจอย้ายมาที่ SDA=GPIO");
    Serial.print(OLED_SDA_PIN);
    Serial.print(" SCL=GPIO");
    Serial.print(OLED_SCL_PIN);
    Serial.println(" แล้วหรือยัง และ VCC กับ GND ต่อครบไหม");
  }
}

// สั่ง init ครั้งเดียวโดยไม่รอ ใช้ตอนที่รู้แล้วว่าจอตอบรับอยู่
bool beginDisplayOnce() {
#if OLED_DRIVER_SH1106
  display.begin(OLED_ADDR, false);  // false = ไม่ใช้ขา reset โมดูล I2C ไม่มีขานี้
  display.setContrast(OLED_CONTRAST);
  return true;
#else
  return display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
#endif
}

// วนรอจนจอตอบรับแล้วค่อยสั่ง init -- ใช้เฉพาะใน setup() เท่านั้น
// ห้ามเรียกจาก loop() เพราะบล็อกได้นานถึง 2 วินาที ซึ่งจะทำให้หน้าเว็บค้าง
bool initDisplay() {
  // รอให้วงจรภายในจอตั้งตัวเสร็จก่อน ซึ่งช้ากว่าตอน ESP32 บูตเสร็จ
  // ถ้ายิงคำสั่ง init ไปเร็วเกินคำสั่งจะหายไปเงียบๆ แล้วจอค้างดำ
  delay(OLED_BOOT_WAIT_MS);
  beginDisplayOnce();
  Serial.println("สั่งเริ่มจอแล้ว");
  return true;
}


// ---------- ส่งภาพขึ้นจอเอง ไม่ผ่าน display() ของไลบรารี ----------
// ไลบรารีส่งภาพเป็นก้อนใหญ่ ถ้าสัญญาณบนสายไม่นิ่งพอ ก้อนจะขาดกลางคัน
// แล้วภาพเข้าจอไม่ครบเฟรม ซึ่งเป็นอาการที่เจออยู่
//
// ตัวนี้ส่งทีละหน้า (หนึ่งหน้า = 8 แถวพิกเซล) และแบ่งข้อมูลเป็นก้อนละ 16 ไบต์
// ก้อนเล็กลงทำให้แต่ละรายการสั้นลง โอกาสขาดกลางคันน้อยลง
// และถ้าหน้าไหนพลาด จะเสียแค่แถบเดียว ไม่ลามทั้งเฟรมเหมือนตอนส่งก้อนใหญ่
//
// ยังใช้ Adafruit_GFX วาดลงบัฟเฟอร์เหมือนเดิมทุกอย่าง เปลี่ยนแค่ขั้นตอนส่งออกจอ

#if OLED_DRIVER_SH1106
const uint8_t OLED_COL_OFFSET = 2;  // SH1106 มี RAM 132 คอลัมน์ แต่จอจริงเริ่มที่คอลัมน์ 2
#else
const uint8_t OLED_COL_OFFSET = 0;  // SSD1306 เริ่มที่คอลัมน์ 0 พอดี
#endif
const int OLED_CHUNK = 16;          // ส่งข้อมูลภาพครั้งละกี่ไบต์

// ส่งคำสั่งหนึ่งไบต์ไปที่จอ
// หมายเหตุสำคัญ: ห้ามเชื่อค่าที่ Wire.endTransmission() คืนมาบนเครื่องนี้
//
// มันรายงานว่าล้มเหลวทั้งที่ข้อมูลส่งถึงจอจริงและภาพขึ้นปกติ พิสูจน์แล้วสองครั้ง
// ครั้งแรกทำให้ตัวเคาะถามบอกว่าจอเสียจนโค้ดเลิกวาดจอทั้งหมด ภาพค้างที่เฟรมเดิม
// ครั้งที่สองทำให้ตัวตรวจจับบัสค้างสั่งเริ่มบัสใหม่ทุกเฟรมจนไม่มีเฟรมไหนได้วาดเลย
//
// จึงส่งไปเลยโดยไม่ตรวจผล ซึ่งเป็นแบบที่วนภาพครบทุกรูปได้จริง
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
#if OLED_DRIVER_SH1106
  oledCmd(0xAD); oledCmd(0x8B);   // เปิดปั๊มแรงดัน แบบ SH1106
#else
  oledCmd(0x8D); oledCmd(0x14);   // เปิดปั๊มแรงดัน แบบ SSD1306
#endif
  oledCmd(0xA1);                  // กลับซ้ายขวา
  oledCmd(0xC8);                  // กลับบนล่าง
  oledCmd(0xDA); oledCmd(0x12);   // การจัดเรียงขา COM
  oledCmd(0x81); oledCmd(OLED_CONTRAST);
  oledCmd(0xA4);                  // แสดงตามข้อมูลในหน่วยความจำ
  oledCmd(0xA6);                  // สีปกติ ไม่กลับขาวดำ
  oledCmd(0xAF);                  // เปิดจอ (ถ้าเปิดอยู่แล้วไม่มีผลอะไร)
}

// รายงานว่าเฟรมหนึ่งใช้เวลาจริงเท่าไหร่ และส่งได้กี่หน้าจาก 8 หน้า
//
// นี่คือตัววัดที่ตัดสินได้ว่าปัญหา "จอไม่เปลี่ยน" อยู่ตรงไหน
//   ราว 30 ms ครบ 8 หน้า = ข้อมูลออกจากบอร์ดครบแล้ว ปัญหาอยู่ที่จอหรือสาย
//   ราว 150 ms ไม่ครบ 8  = บัสหน่วงจนงบหมด เฟรมถูกตัดทิ้งทุกใบ จอจึงไม่เคยเปลี่ยน
//
// พิมพ์ 10 เฟรมแรกให้หมด หลังจากนั้นพิมพ์อย่างมากวินาทีละครั้ง กันล้น Serial
void reportFrame(unsigned long ms, uint8_t pages) {
  static uint16_t count = 0;
  static unsigned long lastPrint = 0;
  count++;
  if (count > 10 && millis() - lastPrint < 1000) return;
  lastPrint = millis();
  Serial.print("เฟรมที่ ");   Serial.print(count);
  Serial.print(" ใช้ ");       Serial.print(ms);
  Serial.print(" ms ส่งได้ "); Serial.print(pages);
  Serial.println("/8 หน้า");
}

void pushFrame() {
  uint8_t *buf = display.getBuffer();
  if (!buf) return;

  // จำกัดเวลาที่ยอมให้ใช้วาดจอหนึ่งเฟรม
  //
  // ปกติเฟรมหนึ่งใช้เวลาราว 25-50 มิลลิวินาที แต่ถ้าบัสมีปัญหา ทุกรายการจะรอจน
  // หมดเวลาก่อนยอมแพ้ หนึ่งเฟรมมีเกือบ 90 รายการ รวมแล้วกินเวลาหลายวินาที
  // ระหว่างนั้น loop() ติดอยู่ในนี้ ไม่ได้ไปให้บริการ WebSocket กับเว็บเซิร์ฟเวอร์
  // หน้าเว็บจึงต่อไม่ติดและปุ่มกดไม่ทำงาน คือจอที่เสียลากทั้งระบบลงไปด้วย
  //
  // วัดด้วยนาฬิกาเท่านั้น ไม่แตะค่าที่ Wire.endTransmission() คืนมา
  // ซึ่งบนเครื่องนี้รายงานว่าล้มเหลวทั้งที่ส่งผ่านจริง และทำโค้ดพังมาแล้วสองรอบ
  // เฟรมปกติจบเร็วกว่างบนี้หลายเท่า จึงไม่มีทางตัดเฟรมที่ยังดีอยู่ทิ้ง
  const unsigned long budgetMs = 150;
  const unsigned long started = millis();

  oledResync();   // ปลดชิปจอออกจากสภาพค้างกลางคำสั่ง ถ้าบังเอิญค้างอยู่

  uint8_t pagesDone = 0;

  for (uint8_t page = 0; page < OLED_H / 8; page++) {
    oledCmd(0xB0 + page);                              // เลือกหน้าที่จะเขียน
    oledCmd(0x00 | (OLED_COL_OFFSET & 0x0F));          // คอลัมน์เริ่มต้น 4 บิตล่าง
    oledCmd(0x10 | ((OLED_COL_OFFSET >> 4) & 0x0F));   // คอลัมน์เริ่มต้น 4 บิตบน

    const uint8_t *row = buf + (int)page * OLED_W;
    for (int x = 0; x < OLED_W; x += OLED_CHUNK) {
      Wire.beginTransmission(OLED_ADDR);
      Wire.write(0x40);   // ไบต์นำหน้าบอกว่าต่อไปเป็นข้อมูลภาพ
      Wire.write(row + x, OLED_CHUNK);
      Wire.endTransmission();
    }
    pagesDone++;

    if (millis() - started > budgetMs) break;   // ใช้เวลาเกินงบ ทิ้งเฟรมนี้ไป
  }

  reportFrame(millis() - started, pagesDone);
}


// รูปทุกใบใน assets/oled/ วาดมาเต็มจอ 128x64 พร้อมตัวหนังสือในตัวอยู่แล้ว
// จึงแค่พ่นลงจอตรงๆ ไม่ต้องพิมพ์ข้อความซ้อนทับ
void drawFullScreen(const unsigned char *bitmap) {
  if (!hasOled) return;
  display.clearDisplay();
  display.drawBitmap(0, 0, bitmap, OLED_BMP_W, OLED_BMP_H, OLED_WHITE);
  pushFrame();
}

// หน้าจอต้อนรับตอนเปิดเครื่อง โชว์ระหว่างที่ยังตั้งค่า Wi-Fi อยู่
void drawStartScreen() { drawFullScreen(bmp_start); }

// วาดหน้าจอสถานะปกติ (ตอนที่ไม่ได้โชว์รูปสัตว์)
void drawIdleScreen() {
  if (!hasOled) return;
  display.clearDisplay();
  display.setTextColor(OLED_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println("TINY BOT");

  display.setTextSize(1);
  display.setCursor(0, 22);
  display.print("MODE : ");
  display.println(isAutoMode ? "AUTO" : "MANUAL");
  display.setCursor(0, 34);
  display.print("SPEED: ");
  display.println(robotSpeed);
  display.setCursor(0, 46);
  display.print("CAM  : ");
  display.println(camIp.length() ? camIp : String("waiting"));

  pushFrame();
}

// หน้าจอกำลังสแกน เล่นวนตอนหุ่นเคลื่อนที่ จุดท้ายข้อความวิ่งตามเฟรมไปด้วย
void drawScanScreen(int frame) { drawFullScreen(SCAN_FRAMES[frame].bitmap); }

// หน้าจอเจอแล้ว กระพริบสั้นๆ ก่อนจะเฉลยว่าเป็นสัตว์อะไร
void drawDetectScreen() { drawFullScreen(bmp_detected); }

// วาดรูปสัตว์เต็มจอ: รูป 64x64 ด้านซ้าย ชื่อสัตว์ด้านขวา
void drawAnimalScreen(int idx) { drawFullScreen(ANIMALS[idx].bitmap); }

// กระพริบจอกลับสีครั้งเดียวตอนวินาทีที่ 10 เพื่อถามจอว่ายังฟังคำสั่งอยู่ไหม
//
// คำสั่งกลับสีไม่แตะข้อมูลภาพเลย จอแค่อ่านหน่วยความจำเดิมกลับขาวเป็นดำ
// จึงแยกสองอย่างที่ปนกันอยู่ออกจากกันได้
//   จอกระพริบ แต่ภาพไม่เคยเปลี่ยน = จอรับคำสั่งได้ แต่ข้อมูลภาพเขียนไม่เข้า
//   จอไม่กระพริบเลย                = จอไม่รับอะไรอีกแล้ว ค้างสนิทตั้งแต่เฟรมแรก
//
// ตอบได้ด้วยตาเปล่า ไม่ต้องเปิด Serial และไม่ต้องเชื่อค่าที่ Wire คืนมา
void blinkOnce() {
  static bool done = false;
  if (done || !hasOled || millis() < 10000) return;
  done = true;
  Serial.println("ทดสอบ: สั่งจอกลับสี 400 ms -- ถ้าจอกระพริบแปลว่ายังรับคำสั่งอยู่");
  oledCmd(0xA7);   // กลับขาวดำ
  delay(400);
  oledCmd(0xA6);   // กลับมาปกติ
}

// ======================================================
// สื่อสารกับ STM32
// ======================================================

// ส่งคำสั่งไป STM32 พร้อมขึ้นบรรทัดใหม่ (ฝั่ง STM32 อ่านด้วย readStringUntil('\n'))
void sendToStm32(const String &cmd) {
  Serial2.print(cmd);
  Serial2.print('\n');
  Serial.print(">> STM32: ");
  Serial.println(cmd);
}

// เก็บบรรทัดสถานะที่ STM32 ส่งกลับมา ไว้ให้ PC มาดึงไปลง CSV
//
// ตั้งใจไม่ส่งอะไรเข้าหน้าเว็บ ค่าพวกนี้ใช้ตอนวิเคราะห์ย้อนหลังใน Excel
// ไม่ใช่ตอนขับ การเพิ่มของลงหน้าเว็บมีแต่จะเพิ่มโอกาสที่ปุ่มบังคับจะพัง
//
// อ่านทีละไบต์แบบไม่รอ เพราะ loop() ต้องไปให้บริการ WebSocket กับเว็บเซิร์ฟเวอร์ต่อ
// readStringUntil จะบล็อกจนครบ timeout ถ้าสายหลุด ซึ่งทำให้หน้าเว็บค้าง
void pumpStm32Serial() {
  static char line[96];
  static uint8_t len = 0;

  while (Serial2.available() > 0) {
    char c = Serial2.read();
    if (c == '\r') continue;
    if (c != '\n') {
      if (len < sizeof(line) - 1) line[len++] = c;
      continue;
    }
    line[len] = '\0';
    if (len > 2 && line[0] == 'T' && line[1] == ':') lastTelemetry = String(line + 2);
    len = 0;
  }
}

// ======================================================
// WebSocket
// ======================================================

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  if (type == WStype_CONNECTED) {
    // ลูกค้าใหม่เข้ามา บอก IP กล้องให้รู้ทันที หน้าเว็บจะได้เริ่มโหลดภาพ
    String hello = "CAMIP:" + camIp;  // ไลบรารีรับเป็น String& จึงส่งค่าชั่วคราวตรงๆ ไม่ได้
    webSocket.sendTXT(num, hello);
    return;
  }
  if (type != WStype_TEXT) return;

  String msg = String((char *)payload).substring(0, length);

  if (msg == "MODE:AUTO") {
    isAutoMode = true;
    isMoving = true;              // โหมดออโต้ = วิ่งตามเส้นไปเรื่อยๆ
  } else if (msg == "MODE:MANUAL") {
    isAutoMode = false;
    isMoving = false;
  } else if (msg.startsWith("SPEED:")) {
    robotSpeed = msg.substring(6).toInt();
  } else if (msg == "stop") {
    isMoving = false;
  } else {
    isMoving = true;              // forward / backward / left / right
  }

  sendToStm32(msg);  // ส่งต่อทุกคำสั่งให้ STM32 เป็นคนตัดสินใจ
  idleDirty = true;  // ขอให้ loop() วาดจอสถานะใหม่เมื่อถึงคิว
}

// เลือกว่าตอนนี้จอควรแสดงอะไร แล้ววาดให้แค่เท่าที่จำเป็น
// ลำดับความสำคัญ: เจอแล้ว -> รูปสัตว์ -> กำลังสแกน -> หน้าสถานะ
void updateScreen() {
  if (!hasOled) return;
  unsigned long now = millis();

  if (currentAnimal >= 0) {
    if (now < detectUntil) {                       // ช่วงแรก บอกว่าเจอ QR แล้ว
      if (!detectDrawn) {
        detectDrawn = true;
        drawDetectScreen();
      }
      return;
    }
    if (now < animalUntil) {                       // ช่วงสอง เฉลยว่าเป็นสัตว์อะไร วาดครั้งเดียวพอ
      if (!animalDrawn) {
        animalDrawn = true;
        drawAnimalScreen(currentAnimal);
      }
      return;
    }
    currentAnimal = -1;                            // หมดเวลาแล้ว กลับไปหน้าปกติ
    animFrame = 0;
    idleDirty = true;
  }

  if (isMoving) {                                  // หุ่นวิ่งอยู่ = กำลังมองหา QR
    int frame = animFrame % SCAN_FRAME_COUNT;
    if (lastAnimDraw == 0 || now - lastAnimDraw >= SCAN_FRAMES[frame].ms) {
      lastAnimDraw = now;
      drawScanScreen(frame);
      animFrame++;
    }
    idleDirty = true;                              // พอหยุดวิ่งจะได้กลับไปวาดหน้าสถานะทันที
    return;
  }

  if (idleDirty && now - lastIdleDraw > IDLE_DRAW_MS) {
    idleDirty = false;
    lastIdleDraw = now;
    drawIdleScreen();
  }
}

// ======================================================
// UDP จาก ESP32-CAM
// ======================================================

void handleUdp() {
  int packetSize = udp.parsePacket();
  if (packetSize <= 0) return;

  char buf[16];
  int len = udp.read(buf, sizeof(buf) - 1);
  if (len <= 0) return;
  buf[len] = '\0';

  // ทุกแพ็กเก็ตที่เข้ามาคือหลักฐานว่ากล้องยังอยู่ จำ IP ไว้ใช้ทำลิงก์ภาพสด
  String from = udp.remoteIP().toString();
  lastCamSeen = millis();
  if (from != camIp) {
    camIp = from;
    String note = "CAMIP:" + camIp;  // ไลบรารีรับเป็น String& จึงส่งค่าชั่วคราวตรงๆ ไม่ได้
    webSocket.broadcastTXT(note);
    Serial.println("พบกล้องที่ " + camIp);
  }

  // ตัวอักษรแรกคือรหัสสัตว์ ถ้าเป็นแพ็กเก็ตแจ้งว่ายังอยู่ ('P') ก็ไม่ต้องทำอะไรต่อ
  for (int i = 0; i < ANIMAL_COUNT; i++) {
    if (buf[0] != ANIMALS[i].code) continue;
    currentAnimal = i;
    detectUntil = millis() + DETECT_SHOW_MS;       // กระพริบ "เจอแล้ว" ก่อน
    animalUntil = millis() + ANIMAL_SHOW_MS;       // แล้วค่อยเฉลยรูปสัตว์จนครบเวลารวม
    detectDrawn = false;
    animalDrawn = false;
    animFrame = 0;
    lastAnimDraw = 0;
    String note = String("ANIMAL:") + ANIMALS[i].name;
    webSocket.broadcastTXT(note);
    Serial.printf("เจอสัตว์: %s\n", ANIMALS[i].name);
    return;
  }
}

// ======================================================
// setup / loop
// ======================================================

void setup() {
  Serial.begin(115200);
  delay(300);  // รอให้พอร์ตพร้อม ไม่งั้นข้อความแรกๆ จะหายไป
  Serial.println("\n=== Tiny Robot remote ===");
  Serial.printf("build %s %s | ไดรเวอร์จอ: %s\n", __DATE__, __TIME__,
                OLED_DRIVER_SH1106 ? "SH1106" : "SSD1306");

  Serial2.begin(STM32_BAUD, SERIAL_8N1, STM32_RX_PIN, STM32_TX_PIN);  // ช่องส่งคำสั่งไป STM32
  Serial.println("[1] Serial2 ไป STM32 พร้อม");

  // จอ OLED: ถ้าหาไม่เจอก็ปล่อยผ่าน ส่วนอื่นของระบบยังทำงานได้ปกติ
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  Wire.setClock(OLED_I2C_HZ);
  Serial.printf("[2] I2C พร้อม SDA=%d SCL=%d\n", OLED_SDA_PIN, OLED_SCL_PIN);

  Serial.println("[3] กำลังรอจอตอบรับ");
  hasOled = initDisplay();
  Serial.print("[4] hasOled=");
  Serial.println(hasOled);

  drawStartScreen();  // ขึ้นหน้าต้อนรับทันที แล้วปล่อยให้ setup ทำงานต่อเบื้องหลัง
  Serial.println("[5] วาดหน้าต้อนรับแล้ว -- ตอนนี้จอควรขึ้นรูปหุ่นยนต์");

  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.println("[6] เปิด Wi-Fi AP แล้ว");
  Serial.print("เปิดหน้าเว็บที่ http://");
  Serial.println(WiFi.softAPIP());

  server.on("/", []() { server.send_P(200, "text/html", index_html); });

  // ให้ PC มาดึงค่าสถานะล่าสุดไปเก็บลง CSV ตอบเป็นบรรทัดเดียวคั่นด้วยจุลภาค
  // ยังไม่เคยได้ยินอะไรจาก STM32 จะตอบบรรทัดว่าง ฝั่ง PC จะได้รู้ว่าสายยังไม่ติด
  server.on("/telemetry", []() { server.send(200, "text/plain", lastTelemetry); });

  server.begin();

  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);

  udp.begin(ANIMAL_UDP_PORT);

  sendToStm32("stop");  // กันหุ่นค้างคำสั่งเก่าตอนบอร์ดรีเซ็ต
  Serial.println("[7] เว็บเซิร์ฟเวอร์ WebSocket และ UDP พร้อม");

  // ค้างหน้าต้อนรับไว้ให้ครบเวลานับจากบูต ถ้า setup ใช้เวลาไปแล้วเกินนี้ก็ข้ามไปเลย
  while (millis() < START_SCREEN_MS) delay(10);
  drawIdleScreen();
  Serial.println("[8] setup เสร็จสมบูรณ์ จอควรเปลี่ยนเป็นหน้าสถานะแล้ว");
}

void loop() {
  webSocket.loop();
  server.handleClient();
  handleUdp();

  // กล้องเงียบไปนาน ถือว่าหลุด ล้าง IP ทิ้งเพื่อให้หน้าเว็บเลิกรอสตรีมที่ตายแล้ว
  if (camIp.length() && millis() - lastCamSeen > CAM_TIMEOUT_MS) {
    camIp = "";
    webSocket.broadcastTXT("CAMIP:");
    idleDirty = true;
    Serial.println("ขาดการติดต่อกับกล้อง");
  }

  pumpStm32Serial();  // รับค่าสถานะจาก STM32 เก็บไว้ให้ PC มาดึง
  updateScreen();  // เครื่องสถานะของจอ ตัดสินใจเองว่ารอบนี้ต้องวาดอะไรไหม
  blinkOnce();     // ทดสอบครั้งเดียวว่าจอยังฟังคำสั่งอยู่ไหม

  // เต้นหัวใจทุก 5 วินาที ไว้ดูว่าบอร์ดยังไม่ค้างและสถานะจอเป็นยังไง
  static unsigned long lastBeat = 0;
  if (millis() - lastBeat > 5000) {
    lastBeat = millis();
    Serial.printf("ยังทำงานอยู่ %lus | hasOled=%d isMoving=%d animal=%d cam=%s\n",
                  millis() / 1000, hasOled, isMoving, currentAnimal,
                  camIp.length() ? camIp.c_str() : "-");
  }
}