// ======================================================
// STM32 Robot: PWM + PD (Safe Version - No Upload Pin Conflict)
// มอเตอร์: ซ้าย PA0, PA1 | ขวา PB0, PB1 (PWM Ready!)
// เซนเซอร์: PB5, PB6, PB7, PB8, PB9 (เดิมเป๊ะ)
//
// ฐานของไฟล์นี้คือเวอร์ชัน Safe ที่ยืนยันแล้วว่าเดินหน้า-ถอยหลังถูกทาง
// ส่วนที่เพิ่มเข้ามามีแค่ 4 อย่าง และไม่มีอันไหนแตะวิธีขับมอเตอร์เลย
//   1. Serial3 รับ 'S' จาก ESP32-CAM แล้วหยุด 5 วินาที
//   2. คำสั่ง SPEED:<ค่า> จากสไลเดอร์บนหน้าเว็บ
//   3. MIN_PWM กันไม่ให้สั่งมอเตอร์ต่ำกว่าย่านที่มันหมุนไหว
//   4. พิมพ์เวลาคอมไพล์ตอนบูต ไว้เช็กว่าอัปโหลดติดจริง
//
// ช่องทางสื่อสาร
//   Serial  (PA9/PA10)  115200 : Log ออก USB-TTL ไว้ดูตอนดีบัก
//   Serial2 (PA2/PA3)   115200 : รับคำสั่งจาก ESP32 รีโมท
//   Serial3 (PB10/PB11)   9600 : รับ 'S' จาก ESP32-CAM เมื่อสแกน QR เจอสัตว์
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
int baseSpeed = 180;  // ความเร็วพื้นฐานของมอเตอร์ (ค่า PWM ช่วง 0 - 255) ปรับได้จากสไลเดอร์บนเว็บ

// --- ค่าที่เกี่ยวกับแรงบิด ยกมาจาก experiments/stm32_robot_controller_previous ที่เคยวิ่งได้จริง ---
// MIN_PWM คือกำลังไฟขั้นต่ำที่ทำให้ล้อหมุน ต่ำกว่านี้มอเตอร์แค่ครางแต่ไม่ขยับ
// นี่คืออาการ "เดินเหมือนไม่มีแรง" ไฟล์ Safe ตัดค่านี้ทิ้งไป เลยออกตัวไม่ไหว
// ถ้ายังอืดให้ไล่ขึ้นทีละ 10 ถ้าแรงเกินจนคุมเส้นไม่อยู่ให้ลดลงทีละ 10
const int MIN_PWM = 135;
// ชดเชยมอเตอร์สองข้างแรงไม่เท่ากัน ค่าบวกคือหักกำลังฝั่งซ้ายออก
// ถ้าสั่งเดินหน้าแล้วหุ่นเบี่ยงไปข้างใดข้างหนึ่ง ให้ปรับค่านี้ ตั้ง 0 ถ้าไม่ต้องชดเชย
const int LEFT_MOTOR_OFFSET = 25;

const int MIN_SPEED = MIN_PWM;  // ขอบล่างของสไลเดอร์ ต่ำกว่านี้ตั้งไปก็ไม่มีผลเพราะโดน MIN_PWM ดันขึ้นอยู่ดี
const int MAX_SPEED = 255;      // ค่า PWM สูงสุด
const int TURN_SPEED_RATIO = 90;  // ความเร็วตอนหมุนตัว คิดเป็นเปอร์เซ็นต์ของ baseSpeed

const unsigned long QR_STOP_MS = 5000;  // หยุดกี่มิลลิวินาทีเมื่อสแกน QR เจอสัตว์

// เส้นชัยต้องเห็นค้างไว้นานเท่านี้ถึงจะนับว่าใช่ กันเซนเซอร์กระพริบแวบเดียวแล้วปิดออโต้
const unsigned long FINISH_CONFIRM_MS = 80;

bool isAutoMode = false;      // ตัวแปรเก็บสถานะโหมด (true = อัตโนมัติด้วย PD, false = บังคับมือ)
unsigned long stopUntil = 0;  // หยุดนิ่งไปจนถึงเวลานี้ (millis) ใช้ตอนเจอ QR — 0 คือไม่ได้ถูกสั่งหยุด
unsigned long finishSince = 0;  // เริ่มเห็นดำเกือบครบแถวตั้งแต่เมื่อไหร่ — 0 คือตอนนี้ไม่เห็น

void setup() {
  Serial.begin(115200);   // UART1 (PA9, PA10) ยังใช้งานดู Log ได้ปกติ เริ่มการสื่อสาร USB
  Serial2.begin(115200);  // UART2 (PA2, PA3) รับคำสั่งจาก ESP32 รีโมท
  Serial3.begin(9600);    // UART3 (PB10, PB11) รับสัญญาณหยุดจาก ESP32-CAM

  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);  // ตั้งค่าขาควบคุมมอเตอร์ซ้ายเป็นขาออก (OUTPUT)
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);  // ตั้งค่าขาควบคุมมอเตอร์ขวาเป็นขาออก (OUTPUT)

  pinMode(pinL2, INPUT_PULLUP); pinMode(pinL1, INPUT_PULLUP);  // ตั้งค่าขาเซนเซอร์ L2, L1 เป็น INPUT ดึงแรงดันขึ้น (PULLUP)
  pinMode(pinC,  INPUT_PULLUP); pinMode(pinR1, INPUT_PULLUP);  // ตั้งค่าขาเซนเซอร์ C, R1 เป็น INPUT ดึงแรงดันขึ้น (PULLUP)
  pinMode(pinR2, INPUT_PULLUP);  // ตั้งค่าขาเซนเซอร์ R2 เป็น INPUT ดึงแรงดันขึ้น (PULLUP)

  stopMotor();  // สั่งให้มอเตอร์หยุดหมุนก่อนเริ่มต้นการทำงาน

  // พิมพ์เวลาที่คอมไพล์ออกมาด้วย จะได้เช็กได้แน่ๆ ว่าบอร์ดกำลังรันโค้ดเวอร์ชันไหนอยู่
  Serial.print("STM32 robot ready | build ");
  Serial.print(__DATE__); Serial.print(' '); Serial.println(__TIME__);
}

void loop() {
  handleQrStopSignal();   // เช็กสัญญาณหยุดจากกล้องก่อน เพราะมีความสำคัญสูงสุด
  handleRemoteCommand();  // อ่านคำสั่งจากรีโมท

  // ระหว่างหยุดตามคำสั่ง QR ให้ล้อนิ่งไว้ ไม่ต้องคิดอะไรทั้งนั้น
  if (stopUntil != 0) {
    if (millis() < stopUntil) return;
    stopUntil = 0;  // ครบ 5 วินาทีแล้ว กลับไปทำงานโหมดเดิม
    lastError = 0;  // ล้างค่า error เก่า กันหุ่นกระตุกตอนออกตัว
    Serial.println("QR stop finished");
  }

  if (isAutoMode) runPID();  // ถ้าเปิดโหมดอัตโนมัติ ให้ประมวลผลการวิ่งตามเส้นด้วยระบบ PD
}

// รับตัวอักษร 'S' จาก ESP32-CAM แล้วเบรกหยุด 5 วินาที
void handleQrStopSignal() {
  bool gotStop = false;
  while (Serial3.available() > 0) {  // อ่านให้หมดบัฟเฟอร์ กันตัวอักษรเก่าค้างแล้วไปสั่งหยุดซ้ำรอบหน้า
    if (Serial3.read() == 'S') gotStop = true;
  }
  if (!gotStop) return;

  brakeMotor();  // เบรกให้หยุดทันที ไม่ปล่อยไหล
  stopUntil = millis() + QR_STOP_MS;
  Serial.println("QR animal detected -> stop 5s");
}

// อ่านและตีความคำสั่งจาก ESP32 รีโมท
void handleRemoteCommand() {
  if (Serial2.available() <= 0) return;

  String command = Serial2.readStringUntil('\n');  // อ่านข้อความจนกระทั่งเจอการขึ้นบรรทัดใหม่
  command.trim(); command.toLowerCase();  // ตัดช่องว่างหน้า-หลัง และแปลงข้อความให้เป็นตัวพิมพ์เล็กทั้งหมด
  if (command.length() == 0) return;

  // ปรับความเร็วได้ตลอด ไม่ว่าจะอยู่โหมดไหนหรือกำลังหยุดรอ QR อยู่
  if (command.startsWith("speed:")) {
    baseSpeed = constrain(command.substring(6).toInt(), MIN_SPEED, MAX_SPEED);
    Serial.print("speed = "); Serial.println(baseSpeed);
    return;
  }

  if (command.indexOf("auto") >= 0) {    // ถ้ามีคำว่า "auto" ให้เปิดโหมดอัตโนมัติ
    isAutoMode = true;
    lastError = 0;
    Serial.println("mode = AUTO");
    return;
  }
  if (command.indexOf("manual") >= 0) {  // ถ้ามีคำว่า "manual" ให้ปิดโหมดออโต้แล้วหยุดมอเตอร์
    isAutoMode = false;
    stopMotor();
    Serial.println("mode = MANUAL");
    return;
  }

  if (command.indexOf("stop") >= 0) {    // คำสั่ง "stop" ให้หยุดมอเตอร์ ใช้ได้ทุกโหมดเพื่อความปลอดภัย
    stopMotor();
    return;
  }

  if (isAutoMode) return;      // โหมด AUTO ไม่รับคำสั่งบังคับทิศทาง ปล่อยให้ PD ทำงานไป
  if (stopUntil != 0) return;  // กำลังหยุดรอ QR อยู่ อย่าเพิ่งขยับ

  int turn = baseSpeed * TURN_SPEED_RATIO / 100;  // ตอนหมุนตัวใช้ความเร็วน้อยกว่าตอนวิ่งตรงเล็กน้อย
  if      (command.indexOf("forward") >= 0)  setSpeed(baseSpeed, baseSpeed);   // คำสั่ง "forward" ให้เดินหน้า
  else if (command.indexOf("backward") >= 0) setSpeed(-baseSpeed, -baseSpeed); // คำสั่ง "backward" ให้ถอยหลัง
  else if (command.indexOf("left") >= 0)     setSpeed(-turn, turn);            // คำสั่ง "left" ให้หมุนกลับตัวไปทางซ้าย
  else if (command.indexOf("right") >= 0)    setSpeed(turn, -turn);            // คำสั่ง "right" ให้หมุนกลับตัวไปทางขวา
}

// พิมพ์สภาพเซนเซอร์ออกมาเป็นระยะตอนอยู่โหมดออโต้
//
// ถ้ากด AUTO แล้วหุ่นไม่ขยับ บรรทัดนี้ตอบได้ทันทีว่าทำไม
//   ดำครบ 5 ดวงทั้งที่วางบนพื้นขาว = เซนเซอร์กลับลอจิก หรือสายสัญญาณไม่ได้ต่อ
//   ขาวครบ 5 ดวงทั้งที่คร่อมเส้นอยู่ = เซนเซอร์สูงเกินไป หรือยังไม่ได้ปรับความไว
//   ค่าไม่เปลี่ยนเลยตอนโบกมือผ่าน     = เซนเซอร์ไม่ได้จ่ายไฟ
void reportSensors(const int *s, int blackCount) {
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint < 300) return;
  lastPrint = millis();
  Serial.print("sensors L2 L1 C R1 R2 = ");
  for (int i = 0; i < 5; i++) { Serial.print(s[i]); Serial.print(' '); }
  Serial.print("| black="); Serial.print(blackCount);
  Serial.print(" base=");   Serial.println(baseSpeed);
}

void runPID() {
  int s[5] = {digitalRead(pinL2), digitalRead(pinL1), digitalRead(pinC), digitalRead(pinR1), digitalRead(pinR2)};  // อ่านค่าจากเซนเซอร์ทั้ง 5 ตัวเข้าอาร์เรย์ s

  int blackCount = 0;  // ตัวแปรนับจำนวนเซนเซอร์ที่ตรวจพบเส้นสีดำ
  for(int i=0; i<5; i++) if(s[i] == 0) blackCount++;  // ถ้าค่าเซนเซอร์เป็น 0 (พบสีดำ) ให้เพิ่มจำนวน blackCount

  reportSensors(s, blackCount);

  // เห็นดำตั้งแต่ 4 ดวงขึ้นไป = เข้าเส้นชัย ให้เบรกแล้วปิดโหมดออโต้
  //
  // แต่ต้องเห็นค้างไว้จริงๆ ไม่ใช่แวบเดียว เดิมเห็นปุ๊บปิดปั๊บ ซึ่งเกิดได้ตั้งแต่รอบแรก
  // ของ loop จากเซนเซอร์กระพริบตอนออกตัว จากสายหลวม หรือจากเซนเซอร์ที่ลอจิกกลับด้าน
  // อาการที่เห็นคือกด AUTO แล้วหุ่นไม่ขยับเลยสักนิด เพราะออโต้ปิดตัวเองไปก่อนจะได้สั่งล้อ
  //
  // เส้นชัยของจริงกว้างกว่านี้มาก หุ่นคร่อมมันนานเกิน 80 มิลลิวินาทีแน่นอน จึงไม่พลาด
  if (blackCount >= 4) {
    if (finishSince == 0) finishSince = millis();
    if (millis() - finishSince < FINISH_CONFIRM_MS) return;
    brakeMotor();
    isAutoMode = false;
    finishSince = 0;
    Serial.println("finish line -> auto off");
    return;
  }
  finishSince = 0;

  int error = 0;  // ตัวแปรเก็บค่าเบี่ยงเบนจากศูนย์กลางเส้น
  if      (s[0] == 0) error = -4;  // เซนเซอร์ซ้ายสุดเจอเส้น ดำ = เบี่ยงขวามาก (Error -4)
  else if (s[1] == 0) error = -2;  // เซนเซอร์ซ้ายกลางเจอเส้น ดำ = เบี่ยงขวาเล็กน้อย (Error -2)
  else if (s[2] == 0) error = 0;   // เซนเซอร์กลางเจอเส้น ดำ = อยู่ตรงกลางพอดี (Error 0)
  else if (s[3] == 0) error = 2;   // เซนเซอร์ขวากลางเจอเส้น ดำ = เบี่ยงซ้ายเล็กน้อย (Error 2)
  else if (s[4] == 0) error = 4;   // เซนเซอร์ขวาสุดเจอเส้น ดำ = เบี่ยงซ้ายมาก (Error 4)
  else if (blackCount == 0) { setSpeed(-100, -100); return; }  // ถ้าไม่เจอสีดำเลยสักดวง (หลุดเส้น) ให้ถอยหลังช้าๆ เพื่อกลับเข้าหาเส้น

  int output = (Kp * error) + (Kd * (error - lastError));  // คำนวณค่าควบคุม PD: (Kp * Error) + (Kd * ผลต่างของ Error)
  lastError = error;  // บันทึกค่า Error ปัจจุบันไว้ใช้เป็น lastError ในรอบถัดไป

  setSpeed(baseSpeed + output, baseSpeed - output);  // ปรับความเร็วมอเตอร์ซ้าย-ขวา ตามผลลัพธ์ PD ที่คำนวณได้
}

// ดันค่าที่น้อยเกินไปขึ้นให้พ้นย่านที่มอเตอร์ไม่ยอมหมุน โดยไม่แตะเครื่องหมาย (ทิศทางคงเดิม)
int applyMinPwm(int v) {
  if (v == 0) return 0;
  int mag = abs(v);
  if (mag < MIN_PWM) mag = MIN_PWM;
  return (v > 0) ? mag : -mag;
}

void setSpeed(int left, int right) {
  left = constrain(left, -255, 255);    // จำกัดช่วงความเร็วมอเตอร์ซ้ายให้อยู่ระหว่าง -255 ถึง 255
  right = constrain(right, -255, 255);  // จำกัดช่วงความเร็วมอเตอร์ขวาให้อยู่ระหว่าง -255 ถึง 255

  if (left > 0) left -= LEFT_MOTOR_OFFSET;        // ชดเชยมอเตอร์ซ้ายที่แรงไม่เท่าขวา หักกำลังออกทั้งสองทิศ
  else if (left < 0) left += LEFT_MOTOR_OFFSET;

  left = applyMinPwm(left);
  right = applyMinPwm(right);

  // ===== ส่วนนี้เหมือนโค้ดเดิมที่ยืนยันแล้วว่าเดินถูกทาง ห้ามสลับคู่ขา =====
  if (left >= 0) { analogWrite(AIN1, 0); analogWrite(AIN2, left); }  // ถ้าค่าเป็นบวก สั่งมอเตอร์ซ้ายหมุนเดินหน้าด้วยความเร็วแบบ PWM
  else { analogWrite(AIN1, abs(left)); analogWrite(AIN2, 0); }       // ถ้าค่าเป็นลบ สั่งมอเตอร์ซ้ายหมุนถอยหลังด้วยความเร็วแบบ PWM (ใช้ค่าสัมบูรณ์)

  if (right >= 0) { analogWrite(BIN1, 0); analogWrite(BIN2, right); }  // ถ้าค่าเป็นบวก สั่งมอเตอร์ขวาหมุนเดินหน้าด้วยความเร็วแบบ PWM
  else { analogWrite(BIN1, abs(right)); analogWrite(BIN2, 0); }        // ถ้าค่าเป็นลบ สั่งมอเตอร์ขวาหมุนถอยหลังด้วยความเร็วแบบ PWM (ใช้ค่าสัมบูรณ์)
}

void stopMotor() {
  analogWrite(AIN1, 0); analogWrite(AIN2, 0);  // ปล่อยสัญญาณ PWM เป็น 0 ทั้งสองขาของมอเตอร์ซ้าย (หยุดหมุน/ปล่อยไหล)
  analogWrite(BIN1, 0); analogWrite(BIN2, 0);  // ปล่อยสัญญาณ PWM เป็น 0 ทั้งสองขาของมอเตอร์ขวา (หยุดหมุน/ปล่อยไหล)
}

void brakeMotor() {
  // บน STM32duino เมื่อสั่ง analogWrite ไปแล้ว ขานั้นจะถูกยึดไว้ในโหมด Alternate Function ของ Timer
  // การสั่ง digitalWrite ทับเฉยๆ จะไม่มีผล ต้อง pinMode กลับเป็น OUTPUT ก่อน ไม่งั้นเบรกไม่ทำงานจริง
  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, HIGH);  // ช็อตขั้วมอเตอร์ซ้ายด้วยสัญญาณ HIGH ทั้งคู่ (Active Brake ล็อคล้อหยุดทันที)
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, HIGH);  // ช็อตขั้วมอเตอร์ขวาด้วยสัญญาณ HIGH ทั้งคู่ (Active Brake ล็อคล้อหยุดทันที)
  delay(150); stopMotor();  // ล็อคค้างไว้ 150 มิลลิวินาที แล้วตัดสัญญาณไฟกลับเป็นหยุดปกติ
}
