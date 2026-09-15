#include <Arduino.h>

// --- การตั้งค่าพินเซนเซอร์ ---
const int pinL2 = PB9;
const int pinL1 = PB8;
const int pinC  = PB7;
const int pinR1 = PB6;
const int pinR2 = PB5;

// --- การตั้งค่าพินมอเตอร์ ---
#define AIN1 PA0  
#define AIN2 PA1
#define BIN1 PB0  
#define BIN2 PB1

// --- ตัวแปรสถานะ ---
bool isAutoMode = false;
int sensorValues[5]; 

// สถานะการหยุดชั่วคราว (สำหรับ QR Code)
bool isQRWaiting = false;
unsigned long qrStopTimer = 0;

void setup() {
  // Serial: USB Debug
  Serial.begin(115200);   
  
  // Serial2: จาก ESP32 Remote (Manual/Auto)
  Serial2.begin(115200);  

  // Serial3: จาก ESP32-CAM (รับตัว 'D' เพื่อหยุดรถ)
  // บอร์ดส่วนใหญ่จะเปิด Serial3 ให้เลยที่ขา PB10(TX), PB11(RX)
  Serial3.begin(115200);  

  // ตั้งค่ามอเตอร์
  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
  
  // ตั้งค่าเซนเซอร์
  pinMode(pinL2, INPUT_PULLUP);
  pinMode(pinL1, INPUT_PULLUP);
  pinMode(pinC,  INPUT_PULLUP);
  pinMode(pinR1, INPUT_PULLUP);
  pinMode(pinR2, INPUT_PULLUP);

  stopMotor();
  Serial.println(">>> System Ready: QR Scan (Serial3) & Remote (Serial2) <<<");
}

void loop() {
  readSensors();

  // --- 1. รับคำสั่งจาก ESP32-CAM (Serial3) เพื่อหยุดรถชั่วคราว ---
  if (Serial3.available() > 0) {
    char qrCmd = Serial3.read();
    if (qrCmd == 'D' && isAutoMode && !isQRWaiting) {
      Serial.println(">>>> QR DETECTED: DOG! STOPPING 5 SEC <<<<");
      brakeMotor();       // เบรกทันที
      isQRWaiting = true; // เข้าสู่สถานะรอ
      qrStopTimer = millis();
    }
  }

  // --- 2. รับคำสั่งจาก Bluetooth/Remote (Serial2) ---
  if (Serial2.available() > 0) {
    String command = Serial2.readStringUntil('\n');
    command.trim();         
    command.toLowerCase();  
    
    if (command.indexOf("auto") >= 0) {
      isAutoMode = true;
      isQRWaiting = false; // รีเซ็ตสถานะหยุดถ้ากดเปลี่ยนโหมด
      stopMotor(); 
      Serial.println(">>>> MODE: AUTO <<<<");
    } 
    else if (command.indexOf("manual") >= 0) {
      isAutoMode = false;
      isQRWaiting = false;
      stopMotor();
      Serial.println(">>>> MODE: MANUAL <<<<");
    }

    if (!isAutoMode) {
      if      (command.indexOf("forward") >= 0)  moveForward();
      else if (command.indexOf("backward") >= 0) moveBackward();
      else if (command.indexOf("left") >= 0)     turnLeft();
      else if (command.indexOf("right") >= 0)    turnRight();
      else if (command.indexOf("stop") >= 0)     stopMotor();
    }
  }

  // --- 3. Logic การขับเคลื่อน ---
  if (isAutoMode) {
    if (isQRWaiting) {
      // ถ้ากำลังรอ (หยุดรถจาก QR)
      stopMotor();
      // ถ้าครบ 5 วินาที ให้กลับไปวิ่งต่อ
      if (millis() - qrStopTimer > 5000) {
        isQRWaiting = false;
        Serial.println(">>>> 5 SEC PASSED: RESUMING... <<<<");
      }
    } else {
      // วิ่งตามเส้นปกติ
      runLineFollower();
    }
  }

  // Debug ข้อมูล
  static uint32_t lastDebug = 0;
  if (millis() - lastDebug > 200) {
    printSensorDebug();
    lastDebug = millis();
  }
}

// --- ฟังก์ชันอ่านเซนเซอร์ ---
void readSensors() {
  sensorValues[0] = digitalRead(pinL2);
  sensorValues[1] = digitalRead(pinL1);
  sensorValues[2] = digitalRead(pinC);
  sensorValues[3] = digitalRead(pinR1);
  sensorValues[4] = digitalRead(pinR2);
}

// --- ฟังก์ชันเดินตามเส้น ---
void runLineFollower() {
  int L2 = sensorValues[0];
  int L1 = sensorValues[1];
  int C  = sensorValues[2];
  int R1 = sensorValues[3];
  int R2 = sensorValues[4];

  // เช็คเส้นชัย (ดำ 4 ดวงขึ้นไป)
  int blackCount = (L2==0) + (L1==0) + (C==0) + (R1==0) + (R2==0);
  if (blackCount >= 4) { 
    brakeMotor();
    isAutoMode = false;
    Serial.println(">>>> FINISH LINE! STOPPING... <<<<");
    return;
  }
  
  // กรณีหลุดเส้น
  if (L2 == 1 && L1 == 1 && C == 1 && R1 == 1 && R2 == 1) {
    moveBackward();
    return;
  }

  // การตัดสินใจปกติ
  if (C == 0) {
    moveForward(); 
  } 
  else if (L1 == 0 || L2 == 0) {
    turnLeft();    
  } 
  else if (R1 == 0 || R2 == 0) {
    turnRight();   
  }
}

// --- ฟังก์ชันควบคุมมอเตอร์ ---
void moveForward() {
  digitalWrite(AIN1, LOW);  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);
}

void moveBackward() {
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);  digitalWrite(BIN2, HIGH);
}

void turnLeft() {
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);
}

void turnRight() {
  digitalWrite(AIN1, LOW);  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, LOW);  digitalWrite(BIN2, HIGH);
}

void stopMotor() {
  digitalWrite(AIN1, LOW);  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);  digitalWrite(BIN2, LOW);
}

void brakeMotor() {
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, HIGH);
  delay(150);
  stopMotor();
}

void printSensorDebug() {
  Serial.print("Sensors: ");
  for(int i=0; i<5; i++) { Serial.print(sensorValues[i]); Serial.print(" "); }
  if (isQRWaiting) Serial.println("| [QR STOPPING]");
  else Serial.println(isAutoMode ? "| [AUTO]" : "| [MANUAL]");
}