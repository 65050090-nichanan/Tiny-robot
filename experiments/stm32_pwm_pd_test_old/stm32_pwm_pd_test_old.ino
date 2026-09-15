// ======================================================
// STM32 Robot: Remote Speed + PID + Sensor Fix
// มอเตอร์: ซ้าย PA0, PA1 | ขวา PB0, PB1 (PWM)
// ======================================================

const int pinL2 = PB9;
const int pinL1 = PB8;
const int pinC  = PB7;
const int pinR1 = PB6;
const int pinR2 = PB5;

#define AIN1 PA0  
#define AIN2 PA1
#define BIN1 PB0  
#define BIN2 PB1

int remoteSpeed = 180;  
float Kp = 50.0;  // ปรับเพิ่มเพื่อให้เลี้ยวแรงขึ้น
float Kd = 25.0;  
int lastError = 0;
bool isAutoMode = false;
int s[5];

void setup() {
  Serial.begin(115200);   
  Serial2.begin(115200);  

  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
  
  // ใช้ INPUT_PULLUP ตามโค้ดเก่าของคุณเป๊ะๆ
  pinMode(pinL2, INPUT_PULLUP); pinMode(pinL1, INPUT_PULLUP);
  pinMode(pinC,  INPUT_PULLUP); pinMode(pinR1, INPUT_PULLUP);
  pinMode(pinR2, INPUT_PULLUP);

  stopMotor();
}

void loop() {
  // อ่านเซนเซอร์เข้า Array (ดำ=0, ขาว=1 ตามโค้ดเก่า)
  s[0] = digitalRead(pinL2);
  s[1] = digitalRead(pinL1);
  s[2] = digitalRead(pinC);
  s[3] = digitalRead(pinR1);
  s[4] = digitalRead(pinR2);

  if (Serial2.available() > 0) {
    String cmd = Serial2.readStringUntil('\n');
    cmd.trim(); cmd.toLowerCase();
    
    if (cmd.indexOf("auto") >= 0) {
      isAutoMode = true;
      lastError = 0;
    } 
    else if (cmd.indexOf("manual") >= 0) {
      isAutoMode = false;
      stopMotor();
    }

    // รับค่า Speed จากแอป
    if (cmd.indexOf("speed") >= 0 || cmd.indexOf("value") >= 0) {
      String val = "";
      for (int i=0; i<cmd.length(); i++) if (isDigit(cmd[i])) val += cmd[i];
      if (val.length() > 0) remoteSpeed = constrain(val.toInt(), 100, 255);
    }

    if (!isAutoMode) {
      if (cmd.indexOf("forward") >= 0)  setMotorPWM(remoteSpeed, remoteSpeed);
      else if (cmd.indexOf("backward") >= 0) setMotorPWM(-remoteSpeed, -remoteSpeed);
      else if (cmd.indexOf("left") >= 0)     setMotorPWM(-remoteSpeed, remoteSpeed);
      else if (cmd.indexOf("right") >= 0)    setMotorPWM(remoteSpeed, -remoteSpeed);
      else if (cmd.indexOf("stop") >= 0)     stopMotor();
    }
  }

  if (isAutoMode) runPID();
}

void runPID() {
  // 1. เช็คเส้นชัย (ดำ=0) ตาม Logic โค้ดเก่า
  int blackCount = (s[0]==0) + (s[1]==0) + (s[2]==0) + (s[3]==0) + (s[4]==0);
  if (blackCount >= 4) { brakeMotor(); isAutoMode = false; return; }

  // 2. กรณีหลุดเส้น (ขาวหมด=1)
  if (blackCount == 0) {
    setMotorPWM(-120, -120); // ถอยหลังหาเส้น
    return;
  }

  // 3. คำนวณ Error (แปลง ดำ=0 ให้เป็นค่าในการคำนวณ)
  // เราจะใช้ (1 - s[i]) เพื่อให้ ดำกลายเป็น 1 และ ขาวกลายเป็น 0 ในการคำนวณ
  int error = ((1-s[0]) * -2) + ((1-s[1]) * -1) + ((1-s[2]) * 0) + ((1-s[3]) * 1) + ((1-s[4]) * 2);

  // 4. PD Control
  int turn = (int)(Kp * error + Kd * (error - lastError));
  lastError = error;

  // 5. สั่งล้อหมุน (remoteSpeed เป็นความเร็วพื้นฐาน)
  setMotorPWM(remoteSpeed + turn, remoteSpeed - turn);
}

void setMotorPWM(int L, int R) {
  int minP = 150; // กัน Deadzone
  if (L > 0) L = map(L, 0, 255, minP, 255); else if (L < 0) L = map(L, -255, 0, -255, -minP);
  if (R > 0) R = map(R, 0, 255, minP, 255); else if (R < 0) R = map(R, -255, 0, -255, -minP);

  L = constrain(L, -255, 255); R = constrain(R, -255, 255);

  // ล้อซ้าย PA0, PA1
  if (L >= 0) { analogWrite(AIN1, 0); analogWrite(AIN2, L); }
  else { analogWrite(AIN1, abs(L)); analogWrite(AIN2, 0); }

  // ล้อขวา PB0, PB1
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
  delay(150); stopMotor();
}