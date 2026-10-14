// ======================================================
// STM32 Robot: PWM + PID (Safe Version - No Upload Pin Conflict)
// มอเตอร์: ซ้าย PA0, PA1 | ขวา PB0, PB1 (PWM Ready!)
// เซนเซอร์: PB5, PB6, PB7, PB8, PB9 (เดิมเป๊ะ)
// ======================================================

const int pinL2 = PB9;  // กำหนดขา PB9 เป็นเซนเซอร์ตัวซ้ายสุด (L2)
const int pinL1 = PB8;  // กำหนดขา PB8 เป็นเซนเซอร์ตัวซ้ายกลาง (L1)
const int pinC  = PB7;  // กำหนดขา PB7 เป็นเซนเซอร์ตัวกลาง (C)
const int pinR1 = PB6;  // กำหนดขา PB6 เป็นเซนเซอร์ตัวขวากลาง (R1)
const int pinR2 = PB5;  // กำหนดขา PB5 เป็นเซนเซอร์ตัวขวาสุด (R2)

#define AIN1 PA0  // กำหนดนิยาม AIN1 ใช้ขา PA0 (ควบคุมทิศทาง/ความเร็วมอเตอร์ซ้าย)
#define AIN2 PA1  // กำหนดนิยาม AIN2 ใช้ขา PA1 (ควบคุมทิศทาง/ความเร็วมอเตอร์ซ้าย)
#define BIN1 PB0  // มอเตอร์ขวา (ย้ายมา PB0 รองรับสัญญาณ PWM)
#define BIN2 PB1  // มอเตอร์ขวา (ย้ายมา PB1 รองรับสัญญาณ PWM)

float Kp = 80.0;  // ค่าอัตราส่วนขยาย Proportional (ปรับความไวตามระยะห่างจากเส้น)
float Kd = 30.0;  // ค่าอัตราส่วนขยาย Derivative (ช่วยลดอาการส่ายและชะลอการเลี้ยวแรงเกินไป)
int lastError = 0;  // ตัวแปรเก็บค่าความผิดพลาด (Error) ของรอบที่แล้ว
int baseSpeed = 180;  // ความเร็วพื้นฐานของมอเตอร์ (ค่า PWM ช่วง 0 - 255)

bool isAutoMode = false;  // ตัวแปรเก็บสถานะโหมด (true = อัตโนมัติด้วย PID, false = บังคับมือ)

void setup() {
  Serial.begin(115200);   // UART1 (PA9, PA10) ยังใช้งานดู Log ได้ปกติ เริ่มการสื่อสาร USB
  Serial2.begin(115200);  // UART2 (PA2, PA3) เริ่มการสื่อสารบลูทูธ

  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);  // ตั้งค่าขาควบคุมมอเตอร์ซ้ายเป็นขาออก (OUTPUT)
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);  // ตั้งค่าขาควบคุมมอเตอร์ขวาเป็นขาออก (OUTPUT)
  
  pinMode(pinL2, INPUT_PULLUP); pinMode(pinL1, INPUT_PULLUP);  // ตั้งค่าขาเซนเซอร์ L2, L1 เป็น INPUT ดึงแรงดันขึ้น (PULLUP)
  pinMode(pinC,  INPUT_PULLUP); pinMode(pinR1, INPUT_PULLUP);  // ตั้งค่าขาเซนเซอร์ C, R1 เป็น INPUT ดึงแรงดันขึ้น (PULLUP)
  pinMode(pinR2, INPUT_PULLUP);  // ตั้งค่าขาเซนเซอร์ R2 เป็น INPUT ดึงแรงดันขึ้น (PULLUP)

  stopMotor();  // สั่งให้มอเตอร์หยุดหมุนก่อนเริ่มต้นการทำงาน
}

void loop() {
  if (Serial2.available() > 0) {  // ตรวจสอบว่ามีข้อมูลส่งมาจากบลูทูธ (Serial2) หรือไม่
    String command = Serial2.readStringUntil('\n');  // อ่านข้อความจนกระทั่งเจอการขึ้นบรรทัดใหม่
    command.trim(); command.toLowerCase();  // ตัดช่องว่างหน้า-หลัง และแปลงข้อความให้เป็นตัวพิมพ์เล็กทั้งหมด
    
    if (command.indexOf("auto") >= 0) isAutoMode = true;  // ถ้ามีคำว่า "auto" ให้เปิดโหมดอัตโนมัติ
    else if (command.indexOf("manual") >= 0) { isAutoMode = false; stopMotor(); }  // ถ้ามีคำว่า "manual" ให้ปิดโหมดออโต้แล้วหยุดมอเตอร์

    if (!isAutoMode) {  // ถ้าอยู่ในโหมด MANUAL (บังคับมือ)
      if      (command.indexOf("forward") >= 0)  setSpeed(200, 200);   // คำสั่ง "forward" ให้เดินหน้า ความเร็ว 200
      else if (command.indexOf("backward") >= 0) setSpeed(-200, -200); // คำสั่ง "backward" ให้ถอยหลัง ความเร็ว 200
      else if (command.indexOf("left") >= 0)     setSpeed(-180, 180);  // คำสั่ง "left" ให้หมุนกลับตัวไปทางซ้าย
      else if (command.indexOf("right") >= 0)    setSpeed(180, -180);  // คำสั่ง "right" ให้หมุนกลับตัวไปทางขวา
      else if (command.indexOf("stop") >= 0)     stopMotor();          // คำสั่ง "stop" ให้หยุดมอเตอร์
    }
  }

  if (isAutoMode) runPID();  // ถ้าเปิดโหมดอัตโนมัติ ให้ประมวลผลการวิ่งตามเส้นด้วยระบบ PID
}

void runPID() {
  int s[5] = {digitalRead(pinL2), digitalRead(pinL1), digitalRead(pinC), digitalRead(pinR1), digitalRead(pinR2)};  // อ่านค่าจากเซนเซอร์ทั้ง 5 ตัวเข้าอาร์เรย์ s
  
  int blackCount = 0;  // ตัวแปรนับจำนวนเซนเซอร์ที่ตรวจพบเส้นสีดำ
  for(int i=0; i<5; i++) if(s[i] == 0) blackCount++;  // ถ้าค่าเซนเซอร์เป็น 0 (พบสีดำ) ให้เพิ่มจำนวน blackCount
  if (blackCount >= 4) { brakeMotor(); isAutoMode = false; return; }  // ถ้าเจอสีดำตั้งแต่ 4 ดวงขึ้นไป (เข้าเส้นชัย) ให้เบรกทันทีและปิดโหมดออโต้

  int error = 0;  // ตัวแปรเก็บค่าเบี่ยงเบนจากศูนย์กลางเส้น
  if      (s[0] == 0) error = -4;  // เซนเซอร์ซ้ายสุดเจอเส้น ดำ = เบี่ยงขวามาก (Error -4)
  else if (s[1] == 0) error = -2;  // เซนเซอร์ซ้ายกลางเจอเส้น ดำ = เบี่ยงขวาเล็กน้อย (Error -2)
  else if (s[2] == 0) error = 0;   // เซนเซอร์กลางเจอเส้น ดำ = อยู่ตรงกลางพอดี (Error 0)
  else if (s[3] == 0) error = 2;   // เซนเซอร์ขวากลางเจอเส้น ดำ = เบี่ยงซ้ายเล็กน้อย (Error 2)
  else if (s[4] == 0) error = 4;   // เซนเซอร์ขวาสุดเจอเส้น ดำ = เบี่ยงซ้ายมาก (Error 4)
  else if (blackCount == 0) { setSpeed(-100, -100); return; }  // ถ้าไม่เจอสีดำเลยสักดวง (หลุดเส้น) ให้ถอยหลังช้าๆ เพื่อกลับเข้าหาเส้น

  int output = (Kp * error) + (Kd * (error - lastError));  // คำนวณค่าควบคุม PID: (Kp * Error) + (Kd * ผลต่างของ Error)
  lastError = error;  // บันทึกค่า Error ปัจจุบันไว้ใช้เป็น lastError ในรอบถัดไป

  setSpeed(baseSpeed + output, baseSpeed - output);  // ปรับความเร็วมอเตอร์ซ้าย-ขวา ตามผลลัพธ์ PID ที่คำนวณได้
}

void setSpeed(int left, int right) {
  left = constrain(left, -255, 255);    // จำกัดช่วงความเร็วมอเตอร์ซ้ายให้อยู่ระหว่าง -255 ถึง 255
  right = constrain(right, -255, 255);  // จำกัดช่วงความเร็วมอเตอร์ขวาให้อยู่ระหว่าง -255 ถึง 255

  if (left >= 0) { analogWrite(AIN1, 0); analogWrite(AIN2, left); }  // ถ้าค่าเป็นบวก สั่งมอเตอร์ซ้ายหมุนเดินหน้าด้วยความเร็วแบบ PWM
  else { analogWrite(AIN1, abs(left)); analogWrite(AIN2, 0); }       // ถ้าค่าเป็นลบ สั่งมอเตอร์ซ้ายหมุนถอยหลังด้วยความเร็วแบบ PWM (ใช้ค่าสัมบูรณ์)

  if (right >= 0) { analogWrite(BIN1, 0); analogWrite(BIN2, right); }  // ถ้าค่าเป็นบวก สั่งมอเตอร์ขวาหมุนเดินหน้าด้วยความเร็วแบบ PWM
  else { analogWrite(BIN1, abs(right)); analogWrite(BIN2, 0); }       // ถ้าค่าเป็นลบ สั่งมอเตอร์ขวาหมุนถอยหลังด้วยความเร็วแบบ PWM (ใช้ค่าสัมบูรณ์)
}

void stopMotor() {
  analogWrite(AIN1, 0); analogWrite(AIN2, 0);  // ปล่อยสัญญาณ PWM เป็น 0 ทั้งสองขาของมอเตอร์ซ้าย (หยุดหมุน/ปล่อยไหล)
  analogWrite(BIN1, 0); analogWrite(BIN2, 0);  // ปล่อยสัญญาณ PWM เป็น 0 ทั้งสองขาของมอเตอร์ขวา (หยุดหมุน/ปล่อยไหล)
}

void brakeMotor() {
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, HIGH);  // ช็อตขั้วมอเตอร์ซ้ายด้วยสัญญาณ HIGH ทั้งคู่ (Active Brake ล็อคล้อหยุดทันที)
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, HIGH);  // ช็อตขั้วมอเตอร์ขวาด้วยสัญญาณ HIGH ทั้งคู่ (Active Brake ล็อคล้อหยุดทันที)
  delay(150); stopMotor();  // ล็อคค้างไว้ 150 มิลลิวินาที แล้วตัดสัญญาณไฟกลับเป็นหยุดปกติ
}
