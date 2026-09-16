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

// ---------- ลอจิกของเซนเซอร์ ----------
// บอร์ดเซนเซอร์สะท้อนแสงมีสองแบบ แบบหนึ่งออก LOW ตอนเจอสีดำ อีกแบบออก HIGH
// ตั้งค่านี้ให้ตรงกับบอร์ดที่ใช้จริง แล้วโค้ดที่เหลือคิดในหน่วยเดียวกันหมดคือ 1 = อยู่บนเส้นดำ
//
// บอร์ดของหุ่นตัวนี้ออก LOW ตอนเจอสีดำ ยืนยันจากโค้ดเวอร์ชันที่วิ่งได้จริง ซึ่งเช็ก s[i] == 0 ว่าเป็นดำ
// สังเกตจากไฟบนบอร์ดได้ด้วย ไฟดวงไหนติดคือดวงนั้นอยู่บนพื้นขาว ดวงที่ทับเส้นดำอยู่จะดับ
//
// ตั้งผิดแล้วอาการชัดมาก หุ่นที่วางคร่อมเส้นพอดีจะมีเซนเซอร์อยู่นอกเส้น 4 ดวง เหลือกลางดวงเดียวที่ทับเส้น
// ถ้าลอจิกกลับด้าน โค้ดจะนับ 4 ดวงที่อยู่บนพื้นขาวว่าเป็นสีดำ แล้วประกาศว่าถึงเส้นชัยทันที
// หุ่นจึงเบรกแล้วปิดออโต้ตั้งแต่วินาทีแรกที่วางลงบนเส้น ทั้งที่ยังไม่ได้เคลื่อนไปไหนเลย
const bool SENSOR_BLACK_IS_LOW = true;

// ---------- เมื่อหลุดออกจากเส้น ----------
// false = ไม่เจอเส้นแล้วหยุดเลย  true = ออกตามหาเส้นเอง ถอยแล้วกวาดไปมา
//
// การตามหาทำให้หุ่นขยับตลอดเวลาแม้ตอนที่ไม่รู้ว่าเส้นอยู่ไหน ซึ่งบนสนามจริงกลายเป็นอาการกระตุก
// และมักไถลออกไปไกลกว่าเดิมจนหาไม่เจอ หยุดนิ่งแล้วให้คนยกกลับมาวางตรงจุดที่ถูกมักจบเร็วกว่า
const bool SEARCH_WHEN_LOST = false;

// ต้องไม่เจอเส้นค้างไว้นานเท่านี้ก่อน ถึงจะถือว่าหลุดจริง
//
// ทางโค้งมีจังหวะที่เส้นหลุดออกนอกแถวเซนเซอร์ไปชั่วขณะ เป็นเรื่องปกติของโค้งแคบ
// ถ้าตัดสินทันทีที่ไม่เจอ หุ่นจะหยุดทุกโค้ง ระหว่างรอยืนยันจึงเลี้ยวต่อไปทางเดิม
// ซึ่งเป็นทิศที่พาให้กลับไปเจอเส้นพอดีเมื่อเส้นแค่เลยออกนอกแถวชั่วคราว
const unsigned long LOST_CONFIRM_MS = 150;

// ---------- การตามหาเส้น ใช้เมื่อ SEARCH_WHEN_LOST เป็น true ----------
const unsigned long BACKUP_MS = 400;           // ถอยกลับก่อนนานเท่านี้ เผื่อแค่เลยโค้งมานิดเดียว
const unsigned long SWEEP_STEP_MS = 350;       // ขากวาดแรกนานเท่านี้ ขาถัดๆ ไปจะกว้างขึ้นเรื่อยๆ
const unsigned long SEARCH_GIVE_UP_MS = 6000;  // หาไม่เจอภายในเวลานี้ ถือว่าหลุดจริง หยุดแล้วปิดออโต้

// ---------- เงื่อนไขการหยุดที่เส้นชัย ----------
// ตั้ง false ถ้าไม่อยากให้หุ่นหยุดเองเลย ไม่ว่าจะเจอแถบดำกว้างแค่ไหน
const bool STOP_AT_FINISH_LINE = true;
// ต้องเห็นดำกี่ดวงถึงจะเข้าข่ายเส้นชัย ใช้ 5 คือต้องดำทั้งแถว
// เดิมใช้ 4 ซึ่งเกิดขึ้นได้ตอนข้ามทางแยกหรือเข้าโค้งหักศอก ไม่ใช่แค่ตอนถึงเส้นชัย
const int FINISH_BLACK_COUNT = 5;
// ต้องเห็นค้างไว้นานเท่านี้ถึงจะนับว่าใช่
// ทางแยกกว้างไม่กี่เซนติเมตร หุ่นข้ามพ้นในเวลาสั้นกว่านี้มาก จึงไม่โดนนับ
const unsigned long FINISH_CONFIRM_MS = 300;

bool isAutoMode = false;      // ตัวแปรเก็บสถานะโหมด (true = อัตโนมัติด้วย PD, false = บังคับมือ)
unsigned long stopUntil = 0;  // หยุดนิ่งไปจนถึงเวลานี้ (millis) ใช้ตอนเจอ QR — 0 คือไม่ได้ถูกสั่งหยุด
unsigned long finishSince = 0;  // เริ่มเห็นดำเกือบครบแถวตั้งแต่เมื่อไหร่ — 0 คือตอนนี้ไม่เห็น
unsigned long lostSince = 0;    // เริ่มหลุดออกจากเส้นตั้งแต่เมื่อไหร่ — 0 คือยังอยู่บนเส้น

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
    lostSince = 0;
    finishSince = 0;
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

// อ่านเซนเซอร์หนึ่งตัว คืนค่า 1 = อยู่บนเส้นดำ, 0 = อยู่บนพื้นขาว
// ไม่ว่าบอร์ดจะออกลอจิกทางไหน โค้ดที่เรียกใช้ก็ไม่ต้องรู้
int readBlack(int pin) {
  int raw = digitalRead(pin);
  return SENSOR_BLACK_IS_LOW ? (raw == LOW) : (raw == HIGH);
}

// พิมพ์สภาพเซนเซอร์ออกมาเป็นระยะตอนอยู่โหมดออโต้
//
// พิมพ์ทั้งค่าดิบที่ขาอ่านได้ และค่าที่ตีความแล้ว จะได้เห็นว่า SENSOR_BLACK_IS_LOW ตั้งถูกไหม
// วางหุ่นคร่อมเส้นให้กลางทับเส้นพอดี แล้วดูบรรทัด black ต้องได้ 0 0 1 0 0
// ถ้าได้ 1 1 0 1 1 คือกลับด้าน ให้สลับค่า SENSOR_BLACK_IS_LOW บนหัวไฟล์
void reportSensors(const int *raw, const int *black, int blackCount) {
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint < 300) return;
  lastPrint = millis();
  Serial.print("raw   ");
  for (int i = 0; i < 5; i++) { Serial.print(raw[i]); Serial.print(' '); }
  Serial.print("| black ");
  for (int i = 0; i < 5; i++) { Serial.print(black[i]); Serial.print(' '); }
  Serial.print("| count="); Serial.print(blackCount);
  Serial.print(" base=");   Serial.println(baseSpeed);
}

// ตัดสินใจว่าจะทำยังไงเมื่อไม่มีเซนเซอร์ดวงไหนเจอเส้นเลย
//
// ไม่รีบสรุปว่าหลุด เพราะโค้งแคบทำให้เส้นเลยออกนอกแถวเซนเซอร์ไปชั่วขณะได้เป็นปกติ
// ระหว่างรอยืนยันจะเลี้ยวต่อไปทางเดิมด้วยค่า error ล่าสุด ซึ่งคือทิศที่กำลังไล่ตามเส้นอยู่
// โค้งส่วนใหญ่จึงผ่านไปได้โดยไม่มีอะไรเกิดขึ้น หุ่นไม่ต้องหยุดและไม่ต้องกระตุก
void handleLostLine() {
  if (lostSince == 0) lostSince = millis();

  if (millis() - lostSince < LOST_CONFIRM_MS) {
    int output = Kp * lastError;   // เลี้ยวต่อไปทางเดิม เผื่อเส้นแค่เลยออกนอกแถวชั่วคราว
    setSpeed(baseSpeed + output, baseSpeed - output);
    return;
  }

  if (SEARCH_WHEN_LOST) { searchForLine(); return; }

  brakeMotor();
  isAutoMode = false;
  lostSince = 0;
  Serial.println("line lost -> stop");
}

// หาเส้นกลับให้เจอเมื่อหลุดออกไป แทนที่จะถอยตรงอย่างเดียวไปเรื่อยๆ
//
// เดิมพอไม่เจอเส้นเลยก็ถอยตรงท่าเดียว ซึ่งช่วยได้เฉพาะตอนพุ่งเลยเส้นตรงๆ
// ถ้าหลุดตอนเข้าโค้ง เส้นจะอยู่เยื้องไปข้างใดข้างหนึ่ง ถอยตรงยังไงก็ไม่เจอ
//
// ทำเป็นสามจังหวะ ไล่จากสาเหตุที่เจอบ่อยที่สุดไปหาที่เจอน้อยที่สุด
//   1. ถอยกลับก่อน เพราะสาเหตุที่พบบ่อยที่สุดคือเข้าโค้งเร็วไปจนเลยเส้นออกมา
//      ถอยโดยเบี่ยงไปข้างที่เห็นเส้นครั้งสุดท้าย เส้นมักกลับมาอยู่ใต้ตัวพอดี
//   2. ยังไม่เจอก็กวาดหมุนตัว หันไปข้างที่เห็นเส้นครั้งสุดท้ายก่อน แล้วสลับข้างไปมา
//      แต่ละขากวาดกว้างขึ้นกว่าขาก่อน เพราะถ้าวงแคบยังไม่เจอ แปลว่าเส้นอยู่ไกลกว่านั้น
//   3. ครบเวลาที่ให้แล้วยังไม่เจอ ถือว่าไม่มีเส้นให้หาแล้ว หยุดนิ่งดีกว่าไถลไปเรื่อยจนตกโต๊ะ
//
// ข้างที่เห็นเส้นครั้งสุดท้ายดูจาก lastError ค่าลบคือเส้นอยู่ทางซ้าย ค่าบวกคือทางขวา
void searchForLine() {
  if (lostSince == 0) lostSince = millis();
  unsigned long lost = millis() - lostSince;

  if (lost >= SEARCH_GIVE_UP_MS) {
    brakeMotor();
    isAutoMode = false;
    lostSince = 0;
    Serial.println("line not found -> auto off");
    return;
  }

  int turn = baseSpeed * TURN_SPEED_RATIO / 100;
  bool lineWasLeft = (lastError < 0);

  if (lost < BACKUP_MS) {                              // จังหวะที่ 1 ถอยกลับไปหาเส้น
    if (lastError == 0)   setSpeed(-baseSpeed, -baseSpeed);  // หลุดทั้งที่อยู่กลางเส้น ถอยตรงๆ พอ
    else if (lineWasLeft) setSpeed(-turn / 2, -turn);        // เส้นอยู่ซ้าย ถอยให้หัวกวาดไปทางซ้าย
    else                  setSpeed(-turn, -turn / 2);        // เส้นอยู่ขวา ถอยให้หัวกวาดไปทางขวา
    return;
  }

  // จังหวะที่ 2 กวาดหมุนตัวสลับข้าง ขาที่ 0 นาน 1 ช่วง ขาที่ 1 นาน 2 ช่วง ไล่กว้างขึ้นไป
  unsigned long t = lost - BACKUP_MS;
  int leg = 0;
  unsigned long span = SWEEP_STEP_MS;
  while (t >= span) { t -= span; leg++; span = SWEEP_STEP_MS * (leg + 1); }

  bool goLeft = lineWasLeft ? (leg % 2 == 0) : (leg % 2 == 1);
  if (goLeft) setSpeed(-turn, turn);   // หมุนตัวไปทางซ้าย
  else        setSpeed(turn, -turn);   // หมุนตัวไปทางขวา
}

void runPID() {
  const int pins[5] = {pinL2, pinL1, pinC, pinR1, pinR2};
  int raw[5], s[5];
  int blackCount = 0;  // จำนวนเซนเซอร์ที่อยู่บนเส้นดำ
  for (int i = 0; i < 5; i++) {
    raw[i] = digitalRead(pins[i]);
    s[i] = readBlack(pins[i]);   // s[i] == 1 หมายถึงตัวนั้นอยู่บนเส้นดำ
    blackCount += s[i];
  }

  reportSensors(raw, s, blackCount);

  // เจอดำทั้งแถว มีสองความหมาย แยกกันที่ว่าเห็นค้างนานแค่ไหน
  //
  // ทางแยก เส้นตัดกัน หรือโค้งหักศอกที่หุ่นเข้าเฉียง ทำให้ดำทั้งแถวได้เหมือนกัน
  // แต่หุ่นข้ามพ้นในเสี้ยววินาที ส่วนเส้นชัยเป็นแถบกว้างที่หุ่นคร่อมค้างอยู่นาน
  //
  // เดิมไม่ได้แยกสองอย่างนี้ เห็นดำ 4 ดวงแวบเดียวก็ปิดออโต้ทิ้งทันที และปิดแล้วปิดเลย
  // หุ่นจึงหยุดกลางทางแล้วไม่ไปต่อ ทั้งที่ยังไม่ถึงเส้นชัยและไม่ได้เจอ QR อะไรเลย
  //
  // ตอนนี้ระหว่างที่ยังไม่ครบเวลายืนยัน ให้เดินตรงข้ามไปก่อน ซึ่งเป็นสิ่งที่ถูกต้อง
  // เมื่อเจอทางแยก ถ้าข้ามพ้นแล้วดำหายไปเอง ก็ถือว่าไม่ใช่เส้นชัย กลับไปตามเส้นต่อ
  if (blackCount >= FINISH_BLACK_COUNT) {
    if (!STOP_AT_FINISH_LINE) { setSpeed(baseSpeed, baseSpeed); return; }
    if (finishSince == 0) finishSince = millis();
    if (millis() - finishSince < FINISH_CONFIRM_MS) {
      setSpeed(baseSpeed, baseSpeed);  // ยังไม่ชัวร์ว่าเส้นชัย ถือว่าเป็นทางแยก เดินตรงข้ามไป
      return;
    }
    brakeMotor();
    isAutoMode = false;
    finishSince = 0;
    Serial.println("finish line -> auto off");
    return;
  }
  finishSince = 0;  // ดำหายไปก่อนครบเวลา แปลว่าเป็นทางแยก ไม่ใช่เส้นชัย เริ่มนับใหม่

  int error = 0;  // ตัวแปรเก็บค่าเบี่ยงเบนจากศูนย์กลางเส้น
  if      (s[0]) error = -4;  // ซ้ายสุดทับเส้น = หุ่นเบี่ยงไปทางขวามาก
  else if (s[1]) error = -2;  // ซ้ายกลางทับเส้น = เบี่ยงขวาเล็กน้อย
  else if (s[2]) error = 0;   // กลางทับเส้น = อยู่ตรงเส้นพอดี
  else if (s[3]) error = 2;   // ขวากลางทับเส้น = เบี่ยงซ้ายเล็กน้อย
  else if (s[4]) error = 4;   // ขวาสุดทับเส้น = เบี่ยงซ้ายมาก
  else { handleLostLine(); return; }      // ไม่มีดวงไหนเจอเส้นเลย ไปตัดสินใจที่นั่น

  lostSince = 0;  // เจอเส้นแล้ว ล้างตัวจับเวลาการตามหา รอบหน้าที่หลุดจะได้เริ่มนับใหม่

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
