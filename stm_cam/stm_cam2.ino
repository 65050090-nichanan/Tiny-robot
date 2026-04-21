#include <Arduino.h>

// ==========================================================
// --- การตั้งค่าพินมอเตอร์ (STM32 Pin Mapping) ---
// ==========================================================
#define AIN1 PA0  // มอเตอร์ซ้าย 1
#define AIN2 PA1  // มอเตอร์ซ้าย 2 (PWM)
#define BIN1 PB0  // มอเตอร์ขวา 1
#define BIN2 PB1  // มอเตอร์ขวา 2 (PWM)

// --- การตั้งค่าพินเซนเซอร์ (5 Channels) ---
const int pinL2 = PB9; // ซ้ายสุด
const int pinL1 = PB8; // ซ้ายใน
const int pinC  = PB7; // กลาง
const int pinR1 = PB6; // ขวาใน
const int pinR2 = PB5; // ขวาสุด

// ==========================================================
// --- ตัวแปรปรับจูน (Tuning Parameters - ฉบับมอเตอร์ใหม่) ---
// ==========================================================
int remoteSpeed = 160;      // ความเร็วพื้นฐานที่แนะนำ (150-180)
float Kp = 48.0;            // ปรับลดจาก 55 -> 48 เพื่อความนุ่มนวล (มอเตอร์ใหม่ไม่ต้องอัดแรงดึงเยอะ)
float Kd = 25.0;            // ปรับเพิ่มจาก 22 -> 25 เพื่อช่วยลดอาการส่าย (Damping)

// *** จุดสำคัญ: ปรับให้เหลือ 5 เพราะมอเตอร์ใหม่สมดุลดีแล้ว ***
// ถ้าวิ่งจริงแล้วยังเอียงซ้ายนิดๆ ให้เพิ่มเป็น 10 / ถ้าตรงแล้วให้ใช้ 5 หรือ 0
int LEFT_MOTOR_OFFSET = 5; 

int MIN_PWM = 115;          // ลดจาก 135 -> 115 เพราะมอเตอร์ใหม่มีความฝืดน้อยกว่า
int lastError = 0;
bool isAutoMode = false;
uint32_t stopUntil = 0;

// ==========================================================
// --- ฟังก์ชันควบคุมมอเตอร์ (ระบบชดเชยแรงบิด) ---
// ==========================================================

void setMotorPWM(int L, int R) {
  // ระบบชดเชยล้อซ้าย (มอเตอร์ใหม่มักจะกินซ้ายน้อยลงมาก)
  if (L > 0) L -= LEFT_MOTOR_OFFSET;
  else if (L < 0) L += LEFT_MOTOR_OFFSET;

  // ควบคุมไม่ให้ค่า PWM เกินหรือต่ำกว่าขีดจำกัด
  if (L > 0) L = constrain(L, MIN_PWM, 255);
  else if (L < 0) L = constrain(L, -255, -MIN_PWM);
  else L = 0;
  
  if (R > 0) R = constrain(R, MIN_PWM, 255);
  else if (R < 0) R = constrain(R, -255, -MIN_PWM);
  else R = 0;

  // ขับมอเตอร์ล้อซ้าย
  if (L >= 0) { analogWrite(AIN1, 0); analogWrite(AIN2, L); }
  else { analogWrite(AIN1, abs(L)); analogWrite(AIN2, 0); }

  // ขับมอเตอร์ล้อขวา
  if (R >= 0) { analogWrite(BIN1, 0); analogWrite(BIN2, R); }
  else { analogWrite(BIN1, abs(R)); analogWrite(BIN2, 0); }
}

void stopMotor() {
  analogWrite(AIN1, 0); analogWrite(AIN2, 0);
  analogWrite(BIN1, 0); analogWrite(BIN2, 0);
}

void brakeMotor() {
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, HIGH);
  delay(150); 
  stopMotor();
}

// ==========================================================
// --- ระบบนำทาง (PID & Recovery Logic) ---
// ==========================================================

void runPID() {
  int s0 = digitalRead(pinL2); 
  int s1 = digitalRead(pinL1);
  int s2 = digitalRead(pinC);  
  int s3 = digitalRead(pinR1);
  int s4 = digitalRead(pinR2); 

  int blackCount = (s0 == 0) + (s1 == 0) + (s2 == 0) + (s3 == 0) + (s4 == 0);

  // [1] ตรวจพบเส้นหยุดหรือทางแยกใหญ่ (ดำ 4-5 ดวง)
  if (blackCount >= 4) { 
    brakeMotor();
    isAutoMode = false; 
    return;
  }

  // [2] กรณีหลุดออกจากเส้น (ขาวทั้งหมด) -> ระบบถอยหลังกู้คืนตำแหน่ง
  if (blackCount == 0) {
    if (lastError >= 2)       setMotorPWM(-120, -160); // หลุดขวา -> ถอยเบี่ยงซ้าย
    else if (lastError <= -2)  setMotorPWM(-160, -120); // หลุดซ้าย -> ถอยเบี่ยงขวา
    else                       setMotorPWM(-140, -140); // หลุดกลาง -> ถอยตรง
    return; 
  }

  // [3] คำนวณค่า Error
  int error = 0;
  if      (s0 == 0) error = -4; 
  else if (s1 == 0) error = -2; 
  else if (s4 == 0) error = 4;  
  else if (s3 == 0) error = 2;  
  else if (s2 == 0) error = 0;  
  else error = lastError;

  // [4] คำนวณผลลัพธ์ PID
  float P = Kp * error;
  float D = Kd * (error - lastError);
  int turn = (int)(P + D);
  
  int leftSpeed = remoteSpeed + turn;
  int rightSpeed = remoteSpeed - turn;

  setMotorPWM(leftSpeed, rightSpeed);
  lastError = error; 
}

// ==========================================================
// --- ส่วนการทำงานหลัก (Setup & Loop) ---
// ==========================================================

void setup() {
  Serial.begin(115200);   
  Serial2.begin(115200);  // รับคำสั่งจากหน้าเว็บ/รีโมท
  Serial3.begin(9600);    // รับคำสั่งหยุดรถจาก ESP32-CAM

  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
  pinMode(pinL2, INPUT_PULLUP); pinMode(pinL1, INPUT_PULLUP);
  pinMode(pinC,   INPUT_PULLUP); pinMode(pinR1, INPUT_PULLUP);
  pinMode(pinR2, INPUT_PULLUP);

  stopMotor();
  Serial.println("Robot Ready: Optimized for New Motors");
}

void loop() {
  // [1] ตรวจจับสัญญาณ 'S' (Stop) จากระบบ Vision
  if (Serial3.available() > 0) {
    if (Serial3.read() == 'S') { 
      stopUntil = millis() + 5000; // หยุดรอ 5 วินาที
      brakeMotor(); 
      while(Serial3.available() > 0) Serial3.read(); 
    }
  }

  if (millis() < stopUntil) {
    stopMotor();
    return;
  }

  // [2] ประมวลผลคำสั่งจาก Serial2 (Remote Control)
  if (Serial2.available() > 0) {
    String cmd = Serial2.readStringUntil('\n');
    cmd.trim(); cmd.toLowerCase();

    if (cmd.indexOf("auto") >= 0) { isAutoMode = true; lastError = 0; } 
    else if (cmd.indexOf("manual") >= 0) { isAutoMode = false; stopMotor(); }

    if (cmd.indexOf("speed") >= 0) {
      String val = "";
      for (char c : cmd) if (isDigit(c)) val += c;
      if (val.length() > 0) remoteSpeed = constrain(val.toInt(), 120, 220);
    }

    if (!isAutoMode) {
      if (cmd.indexOf("forward") >= 0)       setMotorPWM(remoteSpeed, remoteSpeed);
      else if (cmd.indexOf("backward") >= 0) setMotorPWM(-remoteSpeed, -remoteSpeed);
      else if (cmd.indexOf("left") >= 0)     setMotorPWM(-remoteSpeed, remoteSpeed);
      else if (cmd.indexOf("right") >= 0)    setMotorPWM(remoteSpeed, -remoteSpeed);
      else if (cmd.indexOf("stop") >= 0)     stopMotor();
    }
  }

  // [3] รันระบบ PID เมื่ออยู่ในโหมด Auto
  if (isAutoMode) runPID();
}