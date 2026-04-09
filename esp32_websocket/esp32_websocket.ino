#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

const char* ssid = "My_Robot";
const char* password = "password1234";

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// --- ตัวแปรจำลองสถานะ ---
bool isAutoMode = false;
int robotSpeed = 150;

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
)rawliteral";

// --- ฟังก์ชันแสดงผลบน Monitor ---
void logStatus(String action) {
    Serial.print(">>> MODE: ");
    Serial.print(isAutoMode ? "AUTO" : "MANUAL");
    Serial.print(" | ACTION: ");
    Serial.print(action);
    Serial.print(" | SPEED: ");
    Serial.println(robotSpeed);
}

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_TEXT) {
        String msg = String((char*)payload);
        if (msg == "MODE:AUTO") {
            isAutoMode = true;
            logStatus("SWITCHED TO AUTO");
        } 
        else if (msg == "MODE:MANUAL") {
            isAutoMode = false;
            logStatus("SWITCHED TO MANUAL");
        }
        else if (msg.startsWith("SPEED:")) {
            robotSpeed = msg.substring(6).toInt();
            logStatus("SPEED_UPDATED");
        } 
        else {
            logStatus(msg);
        }
    }
}

void setup() {
    Serial.begin(115200);
    WiFi.softAP(ssid, password);
    
    Serial.println("");
    Serial.println("Robot Logic Tester Started!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());

    server.on("/", []() {
        server.send_P(200, "text/html", index_html);
    });
    server.begin();

    webSocket.begin();
    webSocket.onEvent(onWebSocketEvent);
} // <--- จุดที่ Error เดิม มักจะขาดปีกกานี้ หรืออันก่อนหน้า

void loop() {
    webSocket.loop();
    server.handleClient();

    if (isAutoMode) {
        static unsigned long lastMsg = 0;
        if (millis() - lastMsg > 2000) {
            Serial.println("[Auto Mode] Processing logic...");
            lastMsg = millis();
        }
    }
}