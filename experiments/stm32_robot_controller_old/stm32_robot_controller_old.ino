// ======================================================
// STM32 Robot: Final Version (With Active Brake & Finish Line)
// พินมอเตอร์: ซ้าย PA0, PA1 | ขวา PA4, PA5
// ======================================================

const int pinL2 = PB9;  // กำหนดขา PB9 เป็นเซนเซอร์ตัวซ้ายสุด (L2)
const int pinL1 = PB8;  // กำหนดขา PB8 เป็นเซนเซอร์ตัวซ้ายกลาง (L1)
const int pinC  = PB7;  // กำหนดขา PB7 เป็นเซนเซอร์ตัวกลาง (C)
const int pinR1 = PB6;  // กำหนดขา PB6 เป็นเซนเซอร์ตัวขวากลาง (R1)
const int pinR2 = PB5;  // กำหนดขา PB5 เป็นเซนเซอร์ตัวขวาสุด (R2)

#define AIN1 PA0  // กำหนดนิยาม AIN1 ใช้ขา PA0 (ควบคุมทิศทางมอเตอร์ซ้าย)
#define AIN2 PA1  // กำหนดนิยาม AIN2 ใช้ขา PA1 (ควบคุมทิศทางมอเตอร์ซ้าย)
#define BIN1 PA4  // กำหนดนิยาม BIN1 ใช้ขา PA4 (ควบคุมทิศทางมอเตอร์ขวา)
#define BIN2 PA5  // กำหนดนิยาม BIN2 ใช้ขา PA5 (ควบคุมทิศทางมอเตอร์ขวา)

bool isAutoMode = false;  // สร้างตัวแปรเก็บสถานะโหมด (true = อัตโนมัติ, false = บังคับมือ)
int sensorValues[5];      // อาร์เรย์เก็บค่าที่อ่านได้จากเซนเซอร์ทั้ง 5 ตัว

void setup() {
  Serial.begin(115200);   // เริ่มการสื่อสาร Serial (USB) ที่ความเร็ว 115200 bps
  Serial2.begin(115200);  // เริ่มการสื่อสาร Serial2 (Bluetooth) ที่ความเร็ว 115200 bps

  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);  // ตั้งค่าขาควบคุมมอเตอร์ซ้ายเป็นขาออก (OUTPUT)
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);  // ตั้งค่าขาควบคุมมอเตอร์ขวาเป็นขาออก (OUTPUT)
  
  pinMode(pinL2, INPUT_PULLUP);  // ตั้งค่าขาเซนเซอร์ L2 เป็น INPUT แบบดึงแรงดันขึ้น (PULLUP)
  pinMode(pinL1, INPUT_PULLUP);  // ตั้งค่าขาเซนเซอร์ L1 เป็น INPUT แบบดึงแรงดันขึ้น (PULLUP)
  pinMode(pinC,  INPUT_PULLUP);  // ตั้งค่าขาเซนเซอร์ C  เป็น INPUT แบบดึงแรงดันขึ้น (PULLUP)
  pinMode(pinR1, INPUT_PULLUP);  // ตั้งค่าขาเซนเซอร์ R1 เป็น INPUT แบบดึงแรงดันขึ้น (PULLUP)
  pinMode(pinR2, INPUT_PULLUP);  // ตั้งค่าขาเซนเซอร์ R2 เป็น INPUT แบบดึงแรงดันขึ้น (PULLUP)

  stopMotor();  // สั่งให้มอเตอร์หยุดทำงานก่อนเริ่มต้น
  Serial.println(">>> System Ready: Finish Line Detection Enabled <<<");  // พิมพ์ข้อความแจ้งเตือนว่าระบบพร้อมทำงาน
}

void loop() {
  readSensors();  // เรียกใช้ฟังก์ชันอ่านค่าเซนเซอร์ทั้ง 5 ตัว

  // --- 1. เพิ่มส่วนนี้: รับคำสั่งจาก Python (QR Scan ผ่านสาย USB) ---
  if (Serial.available() > 0) {       // ตรวจสอบว่ามีข้อมูลส่งมาจาก Serial (USB) หรือไม่
    char qrCommand = Serial.read();   // อ่านตัวอักษร 1 ตัวที่ส่งมาจาก Python
    
    if (qrCommand == 'D') {           // ถ้าตัวอักษรที่อ่านได้คือ 'D' (สแกนเจอ DOG)
      Serial.println(">>>> QR DETECTED: DOG! STOPPING 5 SEC <<<<");  // พิมพ์ข้อความแจ้งว่าเจอ QR DOG
      brakeMotor();                   // เรียกใช้ฟังก์ชันเบรกมอเตอร์ทันที
      delay(5000);                    // หยุดรอทำงาน 5,000 มิลลิวินาที (5 วินาที)
      // หลังจาก 5 วินาที รถจะกลับไปวิ่งตามเส้นเองอัตโนมัติใน loop ถัดไป
    }
  }

  // --- 2. ส่วนรับคำสั่งจาก Bluetooth (Serial2) เดิมของคุณ ---
  if (Serial2.available() > 0) {                        // ตรวจสอบว่ามีข้อมูลส่งมาจาก Serial2 (Bluetooth) หรือไม่
    String command = Serial2.readStringUntil('\n');     // อ่านข้อความทั้งหมดจนกระทั่งเจอการขึ้นบรรทัดใหม่
    command.trim();                                     // ตัดช่องว่างหน้าและหลังข้อความออก
    command.toLowerCase();                              // แปลงข้อความทั้งหมดเป็นตัวพิมพ์เล็ก
    
    if (command.indexOf("auto") >= 0) {                // ถ้าในข้อความมีคำว่า "auto"
      isAutoMode = true;                                // เปลี่ยนเป็นโหมดอัตโนมัติ
      stopMotor();                                      // หยุดมอเตอร์ชั่วคราวเพื่อความปลอดภัย
      Serial.println(">>>> MODE: AUTO <<<<");          // พิมพ์แจ้งว่าเข้าสู่โหมด AUTO
    } 
    else if (command.indexOf("manual") >= 0) {          // ถ้าในข้อความมีคำว่า "manual"
      isAutoMode = false;                               // เปลี่ยนเป็นโหมดบังคับมือ
      stopMotor();                                      // หยุดมอเตอร์ชั่วคราว
      Serial.println(">>>> MODE: MANUAL <<<<");        // พิมพ์แจ้งว่าเข้าสู่โหมด MANUAL
    }

    if (!isAutoMode) {                                  // ถ้าอยู่ในโหมด MANUAL (ไม่ใช่ AUTO)
      if      (command.indexOf("forward") >= 0)  moveForward();   // ถ้าคำสั่งคือ "forward" ให้เดินหน้า
      else if (command.indexOf("backward") >= 0) moveBackward();  // ถ้าคำสั่งคือ "backward" ให้ถอยหลัง
      else if (command.indexOf("left") >= 0)     turnLeft();      // ถ้าคำสั่งคือ "left" ให้เลี้ยวซ้าย
      else if (command.indexOf("right") >= 0)    turnRight();     // ถ้าคำสั่งคือ "right" ให้เลี้ยวขวา
      else if (command.indexOf("stop") >= 0)     stopMotor();     // ถ้าคำสั่งคือ "stop" ให้หยุดมอเตอร์
    }
  }

  // --- 3. ส่วนวิ่งอัตโนมัติเดิมของคุณ ---
  if (isAutoMode) {        // ถ้าเปิดใช้งานโหมดอัตโนมัติอยู่
    runLineFollower();     // เรียกใช้ฟังก์ชันคำนวณการเดินตามเส้น
  }

  static uint32_t lastDebug = 0;             // ตัวแปรเก็บเวลาครั้งล่าสุดที่พิมพ์ข้อมูล Debug
  if (millis() - lastDebug > 200) {          // ตรวจสอบว่าผ่านไปครบ 200 มิลลิวินาทีหรือยัง
    printSensorDebug();                      // เรียกใช้ฟังก์ชันพิมพ์ค่าเซนเซอร์ออก Serial
    lastDebug = millis();                    // อัปเดตเวลาล่าสุด
  }
}

void readSensors() {
  sensorValues[0] = digitalRead(pinL2);  // อ่านค่าจากเซนเซอร์ L2 เก็บลงอาร์เรย์ตำแหน่งที่ 0
  sensorValues[1] = digitalRead(pinL1);  // อ่านค่าจากเซนเซอร์ L1 เก็บลงอาร์เรย์ตำแหน่งที่ 1
  sensorValues[2] = digitalRead(pinC);   // อ่านค่าจากเซนเซอร์ C  เก็บลงอาร์เรย์ตำแหน่งที่ 2
  sensorValues[3] = digitalRead(pinR1);  // อ่านค่าจากเซนเซอร์ R1 เก็บลงอาร์เรย์ตำแหน่งที่ 3
  sensorValues[4] = digitalRead(pinR2);  // อ่านค่าจากเซนเซอร์ R2 เก็บลงอาร์เรย์ตำแหน่งที่ 4
}

void runLineFollower() {
  int L2 = sensorValues[0];  // ดึงค่าเซนเซอร์ L2 จากอาร์เรย์มาใส่ตัวแปร
  int L1 = sensorValues[1];  // ดึงค่าเซนเซอร์ L1 จากอาร์เรย์มาใส่ตัวแปร
  int C  = sensorValues[2];  // ดึงค่าเซนเซอร์ C  จากอาร์เรย์มาใส่ตัวแปร
  int R1 = sensorValues[3];  // ดึงค่าเซนเซอร์ R1 จากอาร์เรย์มาใส่ตัวแปร
  int R2 = sensorValues[4];  // ดึงค่าเซนเซอร์ R2 จากอาร์เรย์มาใส่ตัวแปร

  // --- 1. เช็คเส้นชัย (ดำ 4 ดวงขึ้นไป) ---
  int blackCount = (L2==0) + (L1==0) + (C==0) + (R1==0) + (R2==0);  // นับจำนวนเซนเซอร์ที่ตรวจเจอสีดำ (ค่าเป็น 0)
  if (blackCount >= 4) {   // ถ้าเจอสีดำตั้งแต่ 4 ตัวขึ้นไป (เข้าเส้นชัย)
    brakeMotor();          // เรียกใช้ฟังก์ชันเบรกมอเตอร์ทันที
    isAutoMode = false;    // ปิดโหมดอัตโนมัติ
    Serial.println(">>>> FINISH LINE! STOPPING... <<<<");  // พิมพ์แจ้งว่าเจอเส้นชัยแล้ว
    return;                // ออกจากฟังก์ชันทันที
  }
  
  // --- 2. กรณีหลุดเส้น (ขาวหมด) ---
  if (L2 == 1 && L1 == 1 && C == 1 && R1 == 1 && R2 == 1) {  // ถ้าเซนเซอร์ทุกตัวเจอสีขาวทั้งหมด (หลุดเส้น)
    moveBackward();  // ให้ถอยหลังกลับไปหาเส้น
    return;          // ออกจากฟังก์ชัน
  }

  // --- 3. การตัดสินใจทิศทางปกติ ---
  if (C == 0) {        // ถ้าเซนเซอร์ตรงกลางเจอสีดำ
    moveForward();     // ให้เดินหน้า
  } 
  else if (L1 == 0 || L2 == 0) {  // ถ้าเซนเซอร์ทางซ้ายตัวใดตัวหนึ่งเจอสีดำ
    turnLeft();        // ให้เลี้ยวซ้าย
  } 
  else if (R1 == 0 || R2 == 0) {  // ถ้าเซนเซอร์ทางขวาตัวใดตัวหนึ่งเจอสีดำ
    turnRight();       // ให้เลี้ยวขวา
  }
}

// --- ฟังก์ชันมอเตอร์ ---

void moveForward() {
  digitalWrite(AIN1, LOW);  digitalWrite(AIN2, HIGH);  // สั่งมอเตอร์ซ้ายหมุนไปข้างหน้า
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);   // สั่งมอเตอร์ขวาหมุนไปข้างหน้า
}

void moveBackward() {
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW);   // สั่งมอเตอร์ซ้ายหมุนถอยหลัง
  digitalWrite(BIN1, LOW);  digitalWrite(BIN2, HIGH);  // สั่งมอเตอร์ขวาหมุนถอยหลัง
}

void turnLeft() {
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW);   // สั่งมอเตอร์ซ้ายหมุนถอยหลัง
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);   // สั่งมอเตอร์ขวาหมุนไปข้างหน้า (หมุนกลับทิศทางกันเพื่อเลี้ยวซ้าย)
}

void turnRight() {
  digitalWrite(AIN1, LOW);  digitalWrite(AIN2, HIGH);  // สั่งมอเตอร์ซ้ายหมุนไปข้างหน้า
  digitalWrite(BIN1, LOW);  digitalWrite(BIN2, HIGH);  // สั่งมอเตอร์ขวาหมุนถอยหลัง (หมุนกลับทิศทางกันเพื่อเลี้ยวขวา)
}

void stopMotor() {
  digitalWrite(AIN1, LOW);  digitalWrite(AIN2, LOW);   // ตัดไฟมอเตอร์ซ้าย (ปล่อยไหล stop)
  digitalWrite(BIN1, LOW);  digitalWrite(BIN2, LOW);   // ตัดไฟมอเตอร์ขวา (ปล่อยไหล stop)
}

void brakeMotor() {
  // Active Braking: ล็อคมอเตอร์ด้วยการส่ง HIGH ทั้งคู่
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, HIGH);  // ช็อตขั้วมอเตอร์ซ้ายเพื่อเบรกแบบแอคทีฟ (หยุดทันที)
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, HIGH);  // ช็อตขั้วมอเตอร์ขวาเพื่อเบรกแบบแอคทีฟ (หยุดทันที)
  delay(150); // เบรกค้างไว้ 150 มิลลิวินาที
  stopMotor(); // คืนค่าเป็นตัดไฟปกติเพื่อไม่ให้มอเตอร์และไดรฟ์ร้อนเกินไป
}

void printSensorDebug() {
  Serial.print("Sensors: ");  // พิมพ์ข้อความนำหน้า "Sensors: "
  for(int i=0; i<5; i++) { Serial.print(sensorValues[i]); Serial.print(" "); }  // วนลูปพิมพ์ค่าเซนเซอร์ทั้ง 5 ตัวทีละตัว
  Serial.println(isAutoMode ? "| [AUTO]" : "| [MANUAL]");  // พิมพ์แสดงโหมดปัจจุบันว่าอยู่ AUTO หรือ MANUAL
}
