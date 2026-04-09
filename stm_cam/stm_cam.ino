#include <Arduino.h>

// ==========================================================
// --- การตั้งค่าพินมอเตอร์ (L298N / TB6612) ---
// ==========================================================
#define AIN1 PA0  // ควบคุมทิศทางล้อซ้าย 1
#define AIN2 PA1  // ควบคุมทิศทางล้อซ้าย 2 (PWM)
#define BIN1 PB0  // ควบคุมทิศทางล้อขวา 1
#define BIN2 PB1  // ควบคุมทิศทางล้อขวา 2 (PWM)

// ==========================================================
// --- การตั้งค่าพินเซนเซอร์อินฟราเรด (5 Channels) ---
// ==========================================================
const int pinL2 = PB9; // ซ้ายสุด
const int pinL1 = PB8; // ซ้ายใน
const int pinC  = PB7; // กลาง
const int pinR1 = PB6; // ขวาใน
const int pinR2 = PB5; // ขวาสุด

// ==========================================================
// --- ตัวแปรสำหรับระบบควบคุมและ PID ---
// ==========================================================
int remoteSpeed = 180;    // ความเร็วพื้นฐานที่รถวิ่ง (0-255)
float Kp = 50.0;          // ค่า Proportional (แก้การเลี้ยว)
float Kd = 25.0;          // ค่า Derivative (ลดการส่าย)
int lastError = 0;        // ตัวแปรเก็บค่าความผิดพลาดครั้งก่อนหน้า
bool isAutoMode = false;  // สถานะโหมด (True = วิ่งเอง / False = บังคับมือ)
uint32_t stopUntil = 0;   // ตัวแปรใช้ควบคุมเวลาหยุดรถโดยไม่ใช้ delay()

// ==========================================================
// --- ฟังก์ชัน SETUP: กำหนดค่าเริ่มต้น ---
// ==========================================================
void setup() {
  // เริ่มต้นการสื่อสารผ่าน Serial
  Serial.begin(115200);   // ช่องทาง Debug ดูผ่านคอมพิวเตอร์
  Serial2.begin(115200);  // ช่องทาง Bluetooth (พิน PA2, PA3)
  Serial3.begin(9600);    // ช่องทางรับข้อมูลจาก ESP32-CAM (พิน PB10, PB11)

  // กำหนดโหมดพินมอเตอร์ให้เป็น Output
  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);

  // กำหนดโหมดพินเซนเซอร์ให้เป็น Input แบบ Pull-up (อ่านค่าได้ 0 เมื่อเจอดำ)
  pinMode(pinL2, INPUT_PULLUP); pinMode(pinL1, INPUT_PULLUP);
  pinMode(pinC,  INPUT_PULLUP); pinMode(pinR1, INPUT_PULLUP);
  pinMode(pinR2, INPUT_PULLUP);

  // สั่งให้มอเตอร์หยุดนิ่งก่อนเริ่มทำงาน
  stopMotor();
  Serial.println("==========================================");
  Serial.println("STM32: FULL SYSTEM READY (5 ANIMALS MODE)");
  Serial.println("==========================================");
}

// ==========================================================
// --- ฟังก์ชัน LOOP: ส่วนทำงานหลัก ---
// ==========================================================
void loop() {
  
  // --- [1] ส่วนรับรหัสสัตว์จากกล้อง (Serial3) ---
  // Python ส่งรหัสสัตว์มา -> ESP32 รับแล้วส่ง 'S' ต่อมาที่นี่
  if (Serial3.available() > 0) {
    char camCmd = Serial3.read();
    
    // ถ้าได้รับรหัสตัว 'S' (หมายถึงเจอสัตว์ที่กำหนดไว้)
    if (camCmd == 'S') { 
      Serial.println(">> [EVENT] ANIMAL DETECTED! <<");
      Serial.println(">> ACTION: BRAKE AND STOP 5 SECONDS <<");
      
      // คำนวณเวลาที่จะให้หยุด (เวลาปัจจุบัน + 5000 มิลลิวินาที)
      stopUntil = millis() + 5000; 
      
      // สั่งเบรกมอเตอร์ทันที
      brakeMotor(); 

      // --- ส่วนสำคัญ: ล้างข้อมูลขยะใน Buffer ทิ้ง ---
      // เพื่อป้องกันไม่ให้สัญญาณรบกวนในสนามทำให้รถหยุดซ้ำซ้อน
      while(Serial3.available() > 0) {
        Serial3.read(); 
      }
    }
  }

  // --- [2] ส่วนตรวจสอบการหยุดรอ (Non-blocking Stop) ---
  // ถ้าเวลาปัจจุบันยังไม่ถึงเวลาที่กำหนดให้หยุด จะข้ามการทำงานอื่นทั้งหมด
  if (millis() < stopUntil) {
    stopMotor(); // บังคับมอเตอร์ให้หยุดนิ่ง
    return;      // ออกจาก loop ทันที (ไม่คำนวณ PID)
  }

  // --- [3] ส่วนรับคำสั่งจาก Bluetooth (Serial2) ---
  if (Serial2.available() > 0) {
    String cmd = Serial2.readStringUntil('\n');
    cmd.trim(); 
    cmd.toLowerCase();

    // เช็คคำสั่งเปลี่ยนโหมด
    if (cmd.indexOf("auto") >= 0) {
      isAutoMode = true;
      lastError = 0; 
      Serial.println("MODE SWITCHED: AUTO");
    } 
    else if (cmd.indexOf("manual") >= 0) {
      isAutoMode = false;
      stopMotor();
      Serial.println("MODE SWITCHED: MANUAL");
    }

    // เช็คคำสั่งปรับความเร็ว (ตัวอย่าง speed:200)
    if (cmd.indexOf("speed") >= 0 || cmd.indexOf("value") >= 0) {
      String val = "";
      for (int i=0; i<cmd.length(); i++) {
        if (isDigit(cmd[i])) val += cmd[i];
      }
      if (val.length() > 0) {
        remoteSpeed = constrain(val.toInt(), 100, 255);
        Serial.print("NEW SPEED SET: "); Serial.println(remoteSpeed);
      }
    }

    // การควบคุมด้วยมือ (ทำงานเฉพาะตอนไม่ได้เป็น Auto)
    if (!isAutoMode) {
      if (cmd.indexOf("forward") >= 0)        setMotorPWM(remoteSpeed, remoteSpeed);
      else if (cmd.indexOf("backward") >= 0)   setMotorPWM(-remoteSpeed, -remoteSpeed);
      else if (cmd.indexOf("left") >= 0)        setMotorPWM(-remoteSpeed, remoteSpeed);
      else if (cmd.indexOf("right") >= 0)       setMotorPWM(remoteSpeed, -remoteSpeed);
      else if (cmd.indexOf("stop") >= 0)        stopMotor();
    }
  }

  // --- [4] ส่วนคำนวณการวิ่งอัตโนมัติ (PID Control) ---
  if (isAutoMode) {
    runPID();
  }
}

// ==========================================================
// --- ฟังก์ชันย่อยสำหรับการทำงานต่างๆ ---
// ==========================================================

void runPID() {
  int s0 = digitalRead(pinL2); 
  int s1 = digitalRead(pinL1);
  int s2 = digitalRead(pinC);  
  int s3 = digitalRead(pinR1);
  int s4 = digitalRead(pinR2); 

  int blackCount = (s0 == 0) + (s1 == 0) + (s2 == 0) + (s3 == 0) + (s4 == 0);

  // 1. เจอเส้นดำ (00000) -> หยุด
  if (blackCount >= 4) { 
    brakeMotor();
    return;
  }

  // 2. แก้ปัญหา "หลุดขวาแล้วนิ่ง" (11111)
  if (blackCount == 0) {
    if (lastError >= 1) { 
      // ถ้าหลุดขวา (error เป็นบวก) ต้องสั่งล้อซ้ายเดินหน้าแรงๆ ล้อขวาถอยหลัง
      // เพิ่มความเร็วเป็น 220 เพื่อให้มอเตอร์มีกำลังพอแม้แบตจะอ่อน
      setMotorPWM(220, -180); 
    } 
    else if (lastError <= -1) { 
      // ถ้าหลุดซ้าย
      setMotorPWM(-180, 220); 
    }
    else {
      setMotorPWM(-120, -120); 
    }
    return; 
  }

  // 3. ปรับค่าน้ำหนัก Error ให้ไวขึ้น
  int error = 0;
  if      (s0 == 0) error = -6; // ซ้ายสุด
  else if (s1 == 0) error = -2;
  else if (s4 == 0) error = 6;  // ขวาสุด
  else if (s3 == 0) error = 2;
  else if (s2 == 0) error = 0;
  else error = lastError;

  int turn = (int)(Kp * error + Kd * (error - lastError));
  turn = constrain(turn, -200, 200); 

  // --- การชดเชยแรงบิดที่สัมพันธ์กับความเร็ว ---
  // เพิ่มความเร็วพื้นฐานขึ้นเล็กน้อยเมื่อเริ่มวิ่งไปนานๆ หรือบวก Offset ล้อซ้ายเพิ่ม
  int leftSpeed = remoteSpeed + turn + 15; // เพิ่มชดเชยล้อซ้ายเป็น 15
  int rightSpeed = remoteSpeed - turn;

  setMotorPWM(leftSpeed, rightSpeed);
  lastError = error; 
}

void setMotorPWM(int L, int R) {
  // เพิ่ม minOutput เป็น 125 เพื่อให้มอเตอร์ขยับได้แม้แบตอ่อน
  int minOutput = 125; 

  if (L > 0) L = constrain(L, minOutput, 255);
  else if (L < 0) L = constrain(L, -255, -minOutput);
  else L = 0;

  if (R > 0) R = constrain(R, minOutput, 255);
  else if (R < 0) R = constrain(R, -255, -minOutput);
  else R = 0;

  // ควบคุมล้อซ้าย
  if (L >= 0) { analogWrite(AIN1, 0); analogWrite(AIN2, L); }
  else { analogWrite(AIN1, abs(L)); analogWrite(AIN2, 0); }

  // ควบคุมล้อขวา
  if (R >= 0) { analogWrite(BIN1, 0); analogWrite(BIN2, R); }
  else { analogWrite(BIN1, abs(R)); analogWrite(BIN2, 0); }
}
void stopMotor() {
  analogWrite(AIN1, 0); analogWrite(AIN2, 0);
  analogWrite(BIN1, 0); analogWrite(BIN2, 0);
}

void brakeMotor() {
  // จ่ายไฟเต็มพิกัดทั้งสองทิศทางในช่วงสั้นๆ เพื่อหยุดล้อทันที
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, HIGH);
  delay(150); 
  stopMotor();
}