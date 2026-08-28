#include <WiFi.h>              // รวมไลบรารีสำหรับการใช้งาน WiFi (สำหรับบอร์ด ESP32)
#include <WebServer.h>         // รวมไลบรารีสำหรับสร้าง Web Server ให้บริการหน้าเว็บ
#include <WebSocketsServer.h>  // รวมไลบรารีสำหรับสร้าง WebSocket Server เพื่อการสื่อสารเรียลไทม์

const char* ssid = "My_Robot";          // กำหนดชื่อชื่อเครือข่าย WiFi (SSID) ที่หุ่นยนต์จะปล่อย Hotspot
const char* password = "password1234";  // กำหนดรหัสผ่านสำหรับการเชื่อมต่อ WiFi

WebServer server(80);                     // สร้างวัตถุ WebServer ที่พอร์ต 80 (พอร์ตมาตรฐาน HTTP)
WebSocketsServer webSocket = WebSocketsServer(81);  // สร้างวัตถุ WebSocketsServer ที่พอร์ต 81 สำหรับส่งรับข้อมูลเรียลไทม์

// --- ตัวแปรจำลองสถานะ ---
bool isAutoMode = false;  // สร้างตัวแปรเก็บสถานะโหมดการทำงาน (false = MANUAL, true = AUTO)
int robotSpeed = 150;     // สร้างตัวแปรเก็บความเร็วของหุ่นยนต์ (ค่าเริ่มต้น 150)

// --- HTML หน้าควบคุม ---
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <style>
        * { box-sizing: border-box; -webkit-user-select: none; touch-action: manipulation; }
        body { font-family: sans-serif; text-align: center; background-color: #1a1a2e; color: white; margin: 0; padding: 20px; }
        .status-bar { background: #16213e; padding: 15px; border-radius: 15px; margin-bottom: 20px; border: 1px solid #0f3460; }
        #status-text { color: #e94560; font-weight: bold; }
        .grid { display: grid; grid-template-columns: repeat(3, 1fr); grid-gap: 15px; max-width: 300px; margin: 0 auto 30px auto; }
        .btn { width: 100%; aspect-ratio: 1/1; border-radius: 20px; font-size: 28px; color: white; border: none; cursor: pointer; background-color: #0f3460; }
        .btn:active { background-color: #2980b9; }
        .btn-toggle { background-color: #e94560; font-size: 16px; font-weight: bold; }
        .btn-toggle.auto-on { background-color: #0f9d58; }
        .empty { visibility: hidden; }
        .slider-container { background: #16213e; padding: 20px; border-radius: 15px; max-width: 300px; margin: 0 auto; }
        .slider { width: 100%; height: 10px; border-radius: 5px; background: #0f3460; outline: none; -webkit-appearance: none; }
        .slider::-webkit-slider-thumb { -webkit-appearance: none; width: 25px; height: 25px; border-radius: 50%; background: #e94560; border: 2px solid white; }
    </style>
</head>
<body>
    <h2>LOGIC TESTER</h2>
    <div class="status-bar">
        MODE: <span id="status-text">MANUAL</span> | SPEED: <span id="speed-val">150</span>
    </div>
    <div class="grid">
        <div class="empty"></div>
        <button class="btn" onmousedown="sendCmd('forward')" onmouseup="sendCmd('stop')" ontouchstart="sendCmd('forward')" ontouchend="sendCmd('stop')">▲</button>
        <div class="empty"></div>
        <button class="btn" onmousedown="sendCmd('left')" onmouseup="sendCmd('stop')" ontouchstart="sendCmd('left')" ontouchend="sendCmd('stop')">◀</button>
        <button id="main-btn" class="btn btn-toggle" onclick="toggleMode()">STOP<br><small>MANUAL</small></button>
        <button class="btn" onmousedown="sendCmd('right')" onmouseup="sendCmd('stop')" ontouchstart="sendCmd('right')" ontouchend="sendCmd('stop')">▶</button>
        <div class="empty"></div>
        <button class="btn" onmousedown="sendCmd('backward')" onmouseup="sendCmd('stop')" ontouchstart="sendCmd('backward')" ontouchend="sendCmd('stop')">▼</button>
        <div class="empty"></div>
    </div>
    <div class="slider-container">
        <label>Speed: <span id="speed-val2">150</span></label><br><br>
        <input type="range" min="0" max="255" value="150" class="slider" oninput="updateSpeed(this.value)">
    </div>
    <script>
        var gateway = `ws://${window.location.hostname}:81/`;
        var websocket;
        var isAuto = false;
        function initWebSocket() {
            websocket = new WebSocket(gateway);
            websocket.onopen = function() { console.log('WebSocket Connected'); };
        }
        function sendCmd(cmd) {
            if(!isAuto || cmd === 'stop') websocket.send(cmd);
        }
        function toggleMode() {
            isAuto = !isAuto;
            var btn = document.getElementById('main-btn');
            var status = document.getElementById('status-text');
            if(isAuto) {
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
            websocket.send("SPEED:" + s);
        }
        window.onload = initWebSocket;
    </script>
</body>
</html>
)rawliteral";  // ปิดกั้นข้อมูล HTML ที่เก็บในหน่วยความจำ PROGMEM

// --- ฟังก์ชันแสดงผลบน Monitor ---
void logStatus(String action) {  // สร้างฟังก์ชัน logStatus เพื่อแสดงสถานะปัจจุบันผ่าน Serial Monitor
    Serial.print(">>> MODE: ");  // พิมพ์ข้อความนำหน้าโหมด
    Serial.print(isAutoMode ? "AUTO" : "MANUAL");  // ตรวจสอบโหมด ถ้า isAutoMode เป็น true พิมพ์ "AUTO" ถ้าไม่ใช่พิมพ์ "MANUAL"
    Serial.print(" | ACTION: ");  // พิมพ์ข้อความนำหน้าคำสั่งที่ทำ
    Serial.print(action);        // พิมพ์ชื่อคำสั่งที่ได้รับ
    Serial.print(" | SPEED: ");   // พิมพ์ข้อความนำหน้าค่าความเร็ว
    Serial.println(robotSpeed);  // พิมพ์ค่าความเร็วปัจจุบันแล้วขึ้นบรรทัดใหม่
}

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {  // ฟังก์ชันจัดการเหตุการณ์เมื่อมีข้อมูล WebSocket เข้ามา
    if (type == WStype_TEXT) {  // ตรวจสอบว่าชนิดของข้อมูลเป็นข้อความตัวอักษร (TEXT) หรือไม่
        String msg = String((char*)payload);  // แปลงข้อมูล payload ที่ได้รับให้อยู่ในรูปแบบตัวแปร String
        if (msg == "MODE:AUTO") {             // ถ้าข้อความที่ส่งมาคือ "MODE:AUTO"
            isAutoMode = true;                 // สลับเข้าสู่โหมดอัตโนมัติ (AUTO)
            logStatus("SWITCHED TO AUTO");    // แสดงผลบน Monitor ว่าสลับไปโหมด AUTO แล้ว
        } 
        else if (msg == "MODE:MANUAL") {       // ถ้าข้อความที่ส่งมาคือ "MODE:MANUAL"
            isAutoMode = false;                // สลับเข้าสู่โหมดบังคับมือ (MANUAL)
            logStatus("SWITCHED TO MANUAL");  // แสดงผลบน Monitor ว่าสลับไปโหมด MANUAL แล้ว
        }
        else if (msg.startsWith("SPEED:")) {             // ถ้าข้อความขึ้นต้นด้วยคำว่า "SPEED:"
            robotSpeed = msg.substring(6).toInt();       // ตัดเอาตัวเลขหลังคำว่า "SPEED:" แล้วแปลงเป็นจำนวนเต็มเก็บใน robotSpeed
            logStatus("SPEED_UPDATED");                  // แสดงผลบน Monitor ว่ามีการอัปเดตความเร็ว
        } 
        else {                  // ถ้าเป็นข้อความคำสั่งทิศทางอื่นๆ (เช่น forward, backward, stop)
            logStatus(msg);     // บันทึกและแสดงผลข้อความคำสั่งนั้นๆ บน Serial Monitor
        }
    }
}

void setup() {
    Serial.begin(115200);         // เริ่มการสื่อสารผ่าน Serial บอร์ด ESP32 ที่ความเร็ว 115200 bps
    WiFi.softAP(ssid, password);  // ตั้งค่าและเริ่มปล่อยสัญญาณ WiFi Access Point (AP) ตาม SSID และ Password ที่กำหนด
    
    Serial.println("");                               // พิมพ์ขึ้นบรรทัดใหม่เพื่อความสะอาดของหน้าจอ Monitor
    Serial.println("Robot Logic Tester Started!");    // พิมพ์ข้อความแจ้งเตือนว่าโปรแกรมเริ่มทำงานแล้ว
    Serial.print("IP Address: ");                      // พิมพ์ข้อความนำหน้าแสดงหมายเลข IP
    Serial.println(WiFi.softAPIP());                  // พิมพ์หมายเลข IP Address ของ Access Point ออกทาง Serial Monitor

    server.on("/", []() {                           // กำหนด URL เมื่อมีการเรียกเข้าหน้าหลัก "/" ของ WebServer
        server.send_P(200, "text/html", index_html); // ส่งตอบกลับเป็นโค้ด HTML ที่เก็บบันทึกไว้ในหน่วยความจำ PROGMEM ด้วย HTTP Status 200
    });
    server.begin();  // เริ่มต้นการทำงานของ Web Server

    webSocket.begin();                      // เริ่มต้นการทำงานของ WebSocket Server
    webSocket.onEvent(onWebSocketEvent);   // ลงทะเบียนฟังก์ชันที่จะเรียกใช้เมื่อมีเหตุการณ์ส่งรับข้อมูลผ่าน WebSocket
} // <--- จุดที่ Error เดิม มักจะขาดปีกกานี้ หรืออันก่อนหน้า

void loop() {
    webSocket.loop();       // ให้ระบบ WebSocket Server คอยทำงานวนลูปจัดการรับส่งข้อมูลอย่างต่อเนื่อง
    server.handleClient();  // ให้ Web Server คอยรับคำขอเชื่อมต่อจากผู้ใช้อย่างต่อเนื่อง

    if (isAutoMode) {                                   // ตรวจสอบว่าอยู่ในโหมด AUTO หรือไม่
        static unsigned long lastMsg = 0;               // สร้างตัวแปรเก็บเวลาครั้งล่าสุดที่พิมพ์ข้อมูล Auto (ไม่ถูกคืนค่าเมื่อจบ loop)
        if (millis() - lastMsg > 2000) {                // ตรวจสอบว่าผ่านไปครบทุก 2000 มิลลิวินาที (2 วินาที) หรือยัง
            Serial.println("[Auto Mode] Processing logic...");  // พิมพ์แสดงผลว่าโหมด Auto กำลังทำงานอยู่
            lastMsg = millis();                         // อัปเดตเวลาล่าสุดให้เท่ากับเวลาปัจจุบัน
        }
    }
}
