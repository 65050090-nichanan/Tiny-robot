// ======================================================
// STM32 Robot: PWM + PD (Safe Version - No Upload Pin Conflict)
// มอเตอร์: ซ้าย PA0, PA1 | ขวา PB0, PB1 (PWM Ready!)
// เซนเซอร์: PB5, PB6, PB7, PB8, PB9 (เดิมเป๊ะ)
//
// ฐานของไฟล์นี้คือเวอร์ชัน Safe ที่ยืนยันแล้วว่าเดินหน้า-ถอยหลังถูกทาง
// ส่วนที่เพิ่มเข้ามามีแค่ 4 อย่าง และไม่มีอันไหนแตะวิธีขับมอเตอร์เลย
//   1. Serial3 รับ 'S' จาก ESP32-CAM แล้วหยุด 5 วินาที
//   2. คำสั่ง SPEED:<ค่า> จากสไลเดอร์บนหน้าเว็บ
//   3. MIN_PWM พื้นล่างของค่า PWM ตอนนี้ตั้ง 0 คือส่งค่าออกไปตรงๆ
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

// Kp คูณกับ error ซึ่งมีค่าสูงสุด 4 ผลลัพธ์จึงต้องไม่เกินพิสัยที่มอเตอร์รับได้
// ที่ Kp = 80 ผลลัพธ์พุ่งไปถึง 320 เกินช่วง 0-255 ไปเท่าตัว ล้อนอกวิ่งสุด ล้อในหมุนกลับทาง
// กลายเป็นหักศอกทุกครั้งที่เข้าโค้ง แทนที่จะค่อยๆ เลี้ยวตาม
// ที่ Kp = 45 ผลลัพธ์สูงสุดราว 180 ซึ่งอยู่ในพิสัยจริงของมอเตอร์ เลี้ยวนุ่มกว่ามาก
float Kp = 45.0;  // ค่าอัตราส่วนขยาย Proportional (ปรับความไวตามระยะห่างจากเส้น)
float Kd = 35.0;  // ค่าอัตราส่วนขยาย Derivative (ช่วยลดอาการส่ายและชะลอการเลี้ยวแรงเกินไป)
int lastError = 0;  // ตัวแปรเก็บค่าความผิดพลาด (Error) ของรอบที่แล้ว
int baseSpeed = 180;  // ความเร็วพื้นฐานของมอเตอร์ (ค่า PWM ช่วง 0 - 255) ปรับได้จากสไลเดอร์บนเว็บ

// --- ค่าที่เกี่ยวกับแรงบิด ยกมาจาก experiments/stm32_robot_controller_previous ที่เคยวิ่งได้จริง ---
// พื้นล่างของค่า PWM ตั้ง 0 = ไม่มีพื้น สั่งเท่าไหร่ล้อได้เท่านั้น
//
// มอเตอร์ชุดนี้ต่ำกว่าราว 135 จะไม่หมุน แค่ครางอยู่กับที่ การดันค่าขึ้นให้พ้นจุดนั้นอัตโนมัติ
// ทำให้หุ่นออกตัวได้ทุกครั้งก็จริง แต่แลกมาด้วยการที่ทุกค่าตั้งแต่ 1 ถึง 134 ออกมาแรงเท่ากันหมด
// ล็อกจึงบอกว่าสั่ง 50 ทั้งที่มอเตอร์ได้รับ 135 และไม่มีทางรู้เลยว่าตรงไหนถูกดันขึ้นบ้าง
//
// ตอนนี้เลือกให้ตัวเลขตรงกับความจริงมากกว่า ยอมให้ค่าต่ำๆ แล้วล้อไม่หมุน
// ถ้าอยากได้พฤติกรรมเดิมกลับมา ตั้งค่านี้เป็น 135
const int MIN_PWM = 0;
// ชดเชยมอเตอร์สองข้างแรงไม่เท่ากัน ค่าบวกคือหักกำลังฝั่งซ้ายออก ทำให้หุ่นเอนไปทางซ้าย
// ค่าลบคือหักกำลังฝั่งขวาแทน ทำให้หุ่นเอนไปทางขวา ตั้ง 0 ถ้าไม่ต้องชดเชย
//
// ปรับสดจากสไลเดอร์บนหน้าเว็บได้ด้วยคำสั่ง TRIM: ไม่ต้องอัปโหลดใหม่ทุกครั้งที่ลองค่าใหม่
// ซึ่งสำคัญมาก เพราะค่าที่พอดีขึ้นกับมอเตอร์ ล้อ และน้ำหนักที่บรรทุก ต้องลองกันหลายรอบ
int leftMotorOffset = 25;
const int MAX_TRIM = 80;  // ขอบเขตที่ยอมให้ปรับ กันตั้งจนหุ่นหมุนอยู่กับที่

const int MIN_SPEED = MIN_PWM;  // ขอบล่างของสไลเดอร์ ผูกกับ MIN_PWM ให้ทั้งสองที่ไม่ขัดกัน
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
// ไฟเซนเซอร์ติดครบทั้ง 5 ดวงแปลว่าอยู่บนพื้นขาวหมด ไม่มีดวงไหนทับเส้นเลย
// กรณีนี้ต้องขยับตัวเองต่อไปจนกว่าจะมีดวงใดดวงหนึ่งกลับไปเจอเส้น ไม่ใช่หยุดอยู่เฉยๆ
// ตั้ง false ได้ถ้าอยากให้หยุดทันทีที่หลุด
//
// อย่าสับสนกับกรณีไฟดับครบ 5 ดวง นั่นคือดำทั้งแถว = ถึงเส้นชัย อันนั้นถึงจะหยุด
const bool SEARCH_WHEN_LOST = true;

// ต้องไม่เจอเส้นค้างไว้นานเท่านี้ก่อน ถึงจะถือว่าหลุดจริง
//
// ทางโค้งมีจังหวะที่เส้นหลุดออกนอกแถวเซนเซอร์ไปชั่วขณะ เป็นเรื่องปกติของโค้งแคบ
// ถ้าตัดสินทันทีที่ไม่เจอ หุ่นจะหยุดทุกโค้ง ระหว่างรอยืนยันจึงเลี้ยวต่อไปทางเดิม
// ซึ่งเป็นทิศที่พาให้กลับไปเจอเส้นพอดีเมื่อเส้นแค่เลยออกนอกแถวชั่วคราว
const unsigned long LOST_CONFIRM_MS = 150;

// ---------- การตามหาเส้น ใช้เมื่อ SEARCH_WHEN_LOST เป็น true ----------
const unsigned long BACKUP_MS = 400;           // ถอยกลับก่อนนานเท่านี้ เผื่อแค่เลยโค้งมานิดเดียว
const unsigned long SWEEP_STEP_MS = 220;       // ขากวาดแรกนานเท่านี้ ขาถัดๆ ไปจะกว้างขึ้นเรื่อยๆ
const unsigned long SEARCH_GIVE_UP_MS = 6000;  // หาไม่เจอภายในเวลานี้ ถือว่าหลุดจริง หยุดแล้วปิดออโต้

// ความเร็วตอนกวาดหาเส้น คิดเป็นเปอร์เซ็นต์ของ baseSpeed
//
// ต้องช้ากว่าตอนเลี้ยวปกติมาก เพราะการกวาดคือการหมุนตัวอยู่กับที่เพื่อ "มอง" หาเส้น
// ถ้าหมุนเร็ว เส้นจะผ่านใต้แถวเซนเซอร์ไปเร็วเกินกว่าจะจับได้ แล้วก็เลยไปไกลกว่าที่ตั้งใจ
// อาการที่เห็นคือหุ่นเหวี่ยงตัวแรงเกินไปและหาเส้นไม่เจอทั้งที่กวาดผ่านมันไปแล้ว
const int SWEEP_SPEED_RATIO = 55;

// ---------- เงื่อนไขการหยุดที่เส้นชัย ----------
// ตั้ง false ถ้าไม่อยากให้หุ่นหยุดเองเลย ไม่ว่าจะเจอแถบดำกว้างแค่ไหน
const bool STOP_AT_FINISH_LINE = true;
// ต้องเห็นดำกี่ดวงถึงจะเข้าข่ายเส้นชัย ใช้ 5 คือต้องดำทั้งแถว
// เดิมใช้ 4 ซึ่งเกิดขึ้นได้ตอนข้ามทางแยกหรือเข้าโค้งหักศอก ไม่ใช่แค่ตอนถึงเส้นชัย
const int FINISH_BLACK_COUNT = 5;
// ต้องเห็นค้างไว้นานเท่านี้ถึงจะนับว่าใช่
//
// เดิมตั้งไว้ 300 ซึ่งยาวเกินไป แถบเส้นชัยกว้างไม่กี่เซนติเมตร ที่ความเร็ว 180 หุ่นข้ามพ้น
// ในเวลาราว 100-200 มิลลิวินาที จึงวิ่งผ่านเส้นชัยไปเฉยๆ โดยไม่เคยครบเวลายืนยัน
//
// 40 ยังกรองสัญญาณรบกวนได้สบาย เพราะ loop วิ่งหลายพันรอบต่อวินาที สัญญาณกระพริบ
// จากสายสั่นอยู่ได้ไม่ถึงเสี้ยวของเวลานี้ แต่แถบดำจริงอยู่ได้นานกว่านี้แน่นอน
const unsigned long FINISH_CONFIRM_MS = 40;

bool isAutoMode = false;      // ตัวแปรเก็บสถานะโหมด (true = อัตโนมัติด้วย PD, false = บังคับมือ)
unsigned long stopUntil = 0;  // หยุดนิ่งไปจนถึงเวลานี้ (millis) ใช้ตอนเจอ QR — 0 คือไม่ได้ถูกสั่งหยุด
unsigned long finishSince = 0;  // เริ่มเห็นดำเกือบครบแถวตั้งแต่เมื่อไหร่ — 0 คือตอนนี้ไม่เห็น
unsigned long lostSince = 0;    // เริ่มหลุดออกจากเส้นตั้งแต่เมื่อไหร่ — 0 คือยังอยู่บนเส้น
bool searchStartLeft = false;   // รอบนี้จะเริ่มกวาดไปทางซ้ายหรือขวา ตั้งตอนเริ่มหาแต่ละครั้ง

// ---------- ค่าที่ส่งกลับไปโชว์บนหน้าเว็บ ----------
// เก็บสิ่งที่สั่งออกไปจริงๆ ไม่ใช่สิ่งที่ตั้งใจจะสั่ง ค่าพวกนี้ผ่าน constrain และ trim มาแล้ว
// จึงเป็นตัวเลขเดียวกับที่ไปถึงมอเตอร์ ซึ่งเป็นสิ่งที่ต้องรู้ตอนจูน
int cmdLeft = 0;      // PWM ที่สั่งล้อซ้ายครั้งล่าสุด ติดลบคือหมุนถอยหลัง
int cmdRight = 0;     // PWM ที่สั่งล้อขวาครั้งล่าสุด
int steerOutput = 0;  // ค่าการเลี้ยวล่าสุดจาก PD ติดลบคือเลี้ยวซ้าย บวกคือเลี้ยวขวา
int steerError = 0;   // ค่าเบี่ยงจากเส้นล่าสุด -4 ถึง 4 คือวัตถุดิบที่ PD เอาไปคำนวณ

// หุ่นกำลังทำอะไรอยู่ ณ วินาทีนี้ เขียนทับที่จุดตัดสินใจแต่ละจุด
//
// ตัวเลข PWM บอกว่าล้อหมุนแรงแค่ไหน แต่ไม่บอกว่าทำไม 200/80 เกิดได้ทั้งตอนเลี้ยวตามเส้น
// และตอนกวาดหาเส้นที่หลุดไป ซึ่งคนละเรื่องกันสิ้นเชิงตอนมานั่งอ่านล็อกย้อนหลัง
const char *action = "IDLE";
const unsigned long TELEMETRY_MS = 200;  // ส่งค่าสถานะกลับทุกกี่มิลลิวินาที

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

  sendTelemetry();  // รายงานสถานะกลับไปให้หน้าเว็บ
}

// ส่งค่าสถานะกลับไปให้ ESP32 รีโมทเอาไปโชว์บนหน้าเว็บ
//
// ส่งเป็นบรรทัดเดียวคั่นด้วยจุลภาค สั้นและแยกส่วนง่าย ฝั่งรีโมทแค่ส่งต่อไม่ต้องตีความ
//   T:<โหมด>,<กำลังทำอะไร>,<ความเร็วฐาน>,<PWM ซ้าย>,<PWM ขวา>,<ค่าเลี้ยว>,<error>,<Kp>,<Kd>,<trim>,
//     <L2>,<L1>,<C>,<R1>,<R2>,<จำนวนที่ทับเส้น>
//
// ส่ง Kp กับ Kd ไปด้วยทั้งที่เป็นค่าคงที่ เพราะไฟล์ล็อกจะได้อธิบายตัวเองได้
// เปิดผลของเมื่อวานขึ้นมาแล้วรู้ทันทีว่าตอนนั้นจูนไว้เท่าไหร่ ไม่ต้องเดาจากความจำ
// และ error คือวัตถุดิบที่ PD ใช้ พอมีครบก็ตรวจได้ว่า ค่าเลี้ยว = Kp*error + Kd*(error ที่เปลี่ยนไป) จริงไหม
//
// ตัวเลขที่ส่งคือค่าที่สั่งมอเตอร์จริง ผ่าน trim มาแล้ว ไม่ใช่ค่าที่ตั้งใจจะสั่ง
// ตอนจูนต้องดูตัวนี้ เพราะสิ่งที่คำนวณได้กับสิ่งที่มอเตอร์ได้รับมักไม่ใช่ตัวเดียวกัน
void sendTelemetry() {
  static unsigned long last = 0;
  if (millis() - last < TELEMETRY_MS) return;
  last = millis();

  Serial2.print("T:");
  Serial2.print(isAutoMode ? 'A' : 'M');  Serial2.print(',');
  Serial2.print(action);                  Serial2.print(',');
  Serial2.print(baseSpeed);               Serial2.print(',');
  Serial2.print(cmdLeft);                 Serial2.print(',');
  Serial2.print(cmdRight);                Serial2.print(',');
  Serial2.print(steerOutput);             Serial2.print(',');
  Serial2.print(steerError);              Serial2.print(',');
  Serial2.print(Kp, 1);                   Serial2.print(',');
  Serial2.print(Kd, 1);                   Serial2.print(',');
  Serial2.print(leftMotorOffset);         Serial2.print(',');

  // อ่านเซนเซอร์สดตรงนี้ ไม่ใช้ค่าที่ runPID เก็บไว้
  //
  // runPID ทำงานเฉพาะโหมดออโต้ ถ้าอ้างค่าจากตรงนั้นแถวในโหมดบังคับมือจะค้างอยู่ที่ค่าเก่า
  // ทั้งที่เซนเซอร์เห็นอะไรอยู่จริงๆ อ่านใหม่ 5 ขาทุก 200 มิลลิวินาทีถูกกว่าการอธิบายแถวที่โกหก
  const int pins[5] = {pinL2, pinL1, pinC, pinR1, pinR2};
  int onLine = 0;
  for (int i = 0; i < 5; i++) {
    int black = readBlack(pins[i]);
    onLine += black;
    Serial2.print(black);   // เรียงซ้ายไปขวาตามตำแหน่งจริงบนตัวหุ่น
    Serial2.print(',');     // ช่องละดวง ไม่ใช่ 00100 ติดกัน เพราะ Excel จะตัดศูนย์หน้าทิ้งจนเหลือ 100
  }
  Serial2.println(onLine);
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
  action = "QR_STOP";
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
    // สั่งเท่าไหร่ก็เอาไปเท่านั้น ไม่ดันขึ้นให้ ค่าต่ำๆ ล้ออาจไม่หมุนซึ่งเป็นเรื่องปกติของมอเตอร์
    baseSpeed = constrain(command.substring(6).toInt(), MIN_SPEED, MAX_SPEED);
    Serial.print("speed = "); Serial.println(baseSpeed);
    return;
  }

  // ปรับความเอนซ้ายขวาได้ตลอด ใช้จูนตอนหุ่นวิ่งอยู่โดยไม่ต้องหยุดหรืออัปโหลดใหม่
  if (command.startsWith("trim:")) {
    leftMotorOffset = constrain(command.substring(5).toInt(), -MAX_TRIM, MAX_TRIM);
    Serial.print("trim = "); Serial.println(leftMotorOffset);
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
    action = "STOP";
    return;
  }

  if (isAutoMode) return;      // โหมด AUTO ไม่รับคำสั่งบังคับทิศทาง ปล่อยให้ PD ทำงานไป
  if (stopUntil != 0) return;  // กำลังหยุดรอ QR อยู่ อย่าเพิ่งขยับ

  int turn = baseSpeed * TURN_SPEED_RATIO / 100;  // ตอนหมุนตัวใช้ความเร็วน้อยกว่าตอนวิ่งตรงเล็กน้อย
  if      (command.indexOf("forward") >= 0)  { setSpeed(baseSpeed, baseSpeed);   action = "FORWARD"; }
  else if (command.indexOf("backward") >= 0) { setSpeed(-baseSpeed, -baseSpeed); action = "BACKWARD"; }
  else if (command.indexOf("left") >= 0)     { setSpeed(-turn, turn);            action = "TURN_LEFT"; }
  else if (command.indexOf("right") >= 0)    { setSpeed(turn, -turn);            action = "TURN_RIGHT"; }
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
  if (lostSince == 0) {
    lostSince = millis();

    // เลือกข้างที่จะเริ่มกวาดตอนเริ่มหาแต่ละครั้ง ไม่ใช่คำนวณใหม่ทุกรอบของ loop
    //
    // ถ้ารู้ว่าเส้นอยู่ข้างไหนก็เริ่มจากข้างนั้น แต่ถ้าหลุดตอนวิ่งตรงจะไม่มีเบาะแสเลย
    // (lastError เป็น 0) กรณีนั้นถ้าเลือกข้างเดิมทุกครั้ง หุ่นจะกวาดไปทางเดียวตลอด
    // แล้วยิ่งออกห่างจากเส้นไปเรื่อยๆ จึงสลับข้างจากรอบก่อนแทน
    if (lastError < 0)      searchStartLeft = true;
    else if (lastError > 0) searchStartLeft = false;
    else                    searchStartLeft = !searchStartLeft;
  }

  if (millis() - lostSince < LOST_CONFIRM_MS) {
    action = "BEND";
    int output = Kp * lastError;   // เลี้ยวต่อไปทางเดิม เผื่อเส้นแค่เลยออกนอกแถวชั่วคราว
    setSpeed(baseSpeed + output, baseSpeed - output);
    return;
  }

  if (SEARCH_WHEN_LOST) { action = "SEARCH"; searchForLine(); return; }  // ขยับตัวเองต่อจนกว่าจะเจอเส้น

  brakeMotor();
  isAutoMode = false;
  lostSince = 0;
  action = "LOST";
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
  int sweep = baseSpeed * SWEEP_SPEED_RATIO / 100;

  if (lost < BACKUP_MS) {                                    // จังหวะที่ 1 ถอยกลับไปหาเส้น
    if (lastError == 0)        setSpeed(-baseSpeed, -baseSpeed);  // หลุดทั้งที่อยู่กลางเส้น ถอยตรงๆ พอ
    else if (searchStartLeft)  setSpeed(-turn / 2, -turn);        // เส้นอยู่ซ้าย ถอยให้หัวกวาดไปทางซ้าย
    else                       setSpeed(-turn, -turn / 2);        // เส้นอยู่ขวา ถอยให้หัวกวาดไปทางขวา
    return;
  }

  // จังหวะที่ 2 กวาดหมุนตัวสลับข้าง ขาที่ 0 นาน 1 ช่วง ขาที่ 1 นาน 2 ช่วง ไล่กว้างขึ้นไป
  unsigned long t = lost - BACKUP_MS;
  int leg = 0;
  unsigned long span = SWEEP_STEP_MS;
  while (t >= span) { t -= span; leg++; span = SWEEP_STEP_MS * (leg + 1); }

  bool goLeft = (leg % 2 == 0) ? searchStartLeft : !searchStartLeft;
  if (goLeft) setSpeed(-sweep, sweep);   // หมุนตัวไปทางซ้ายช้าๆ
  else        setSpeed(sweep, -sweep);   // หมุนตัวไปทางขวาช้าๆ
}

void runPID() {
  if (baseSpeed == 0) { stopMotor(); action = "STOP"; return; }  // สไลเดอร์อยู่ที่ 0 = สั่งจอด

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
    action = "FINISH";
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

  action = (error == 0) ? "STRAIGHT" : (error < 0 ? "TURN_LEFT" : "TURN_RIGHT");
  steerError = error;

  int output = (Kp * error) + (Kd * (error - lastError));  // คำนวณค่าควบคุม PD: (Kp * Error) + (Kd * ผลต่างของ Error)
  steerOutput = output;  // เก็บไว้ส่งกลับไปโชว์
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

  // ค่าบวกหักกำลังฝั่งซ้าย ค่าลบหักกำลังฝั่งขวา หักออกเท่ากันทั้งเดินหน้าและถอยหลัง
  if (leftMotorOffset > 0) {
    if (left > 0) left -= leftMotorOffset;
    else if (left < 0) left += leftMotorOffset;
  } else if (leftMotorOffset < 0) {
    if (right > 0) right += leftMotorOffset;
    else if (right < 0) right -= leftMotorOffset;
  }

  left = applyMinPwm(left);
  right = applyMinPwm(right);

  // ===== ส่วนนี้เหมือนโค้ดเดิมที่ยืนยันแล้วว่าเดินถูกทาง ห้ามสลับคู่ขา =====
  if (left >= 0) { analogWrite(AIN1, 0); analogWrite(AIN2, left); }  // ถ้าค่าเป็นบวก สั่งมอเตอร์ซ้ายหมุนเดินหน้าด้วยความเร็วแบบ PWM
  else { analogWrite(AIN1, abs(left)); analogWrite(AIN2, 0); }       // ถ้าค่าเป็นลบ สั่งมอเตอร์ซ้ายหมุนถอยหลังด้วยความเร็วแบบ PWM (ใช้ค่าสัมบูรณ์)

  if (right >= 0) { analogWrite(BIN1, 0); analogWrite(BIN2, right); }  // ถ้าค่าเป็นบวก สั่งมอเตอร์ขวาหมุนเดินหน้าด้วยความเร็วแบบ PWM
  else { analogWrite(BIN1, abs(right)); analogWrite(BIN2, 0); }        // ถ้าค่าเป็นลบ สั่งมอเตอร์ขวาหมุนถอยหลังด้วยความเร็วแบบ PWM (ใช้ค่าสัมบูรณ์)

  cmdLeft = left;    // จำไว้ส่งกลับไปโชว์บนหน้าเว็บ
  cmdRight = right;
}

void stopMotor() {
  cmdLeft = cmdRight = 0;
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
