// ======================================================
// STM32 Robot: PWM + PID (Safe Version - No Upload Pin Conflict)
// มอเตอร์: ซ้าย PA0, PA1 | ขวา PB0, PB1 (PWM Ready!)
// เซนเซอร์: PB5, PB6, PB7, PB8, PB9 (เดิมเป๊ะ)
// ======================================================

const int pinL2 = PB9;
const int pinL1 = PB8;
const int pinC  = PB7;
const int pinR1 = PB6;
const int pinR2 = PB5;

#define AIN1 PA0  
#define AIN2 PA1
#define BIN1 PB0  // มอเตอร์ขวา (ย้ายมา PB0)
#define BIN2 PB1  // มอเตอร์ขวา (ย้ายมา PB1)

float Kp = 80.0;  
float Kd = 30.0;  
int lastError = 0;
int baseSpeed = 180; 

bool isAutoMode = false;

void setup() {
  Serial.begin(115200);   // UART1 (PA9, PA10) ยังใช้งานดู Log ได้ปกติ
  Serial2.begin(115200);  // UART2 (PA2, PA3) บลูทูธ

  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
  
  pinMode(pinL2, INPUT_PULLUP); pinMode(pinL1, INPUT_PULLUP);
  pinMode(pinC,  INPUT_PULLUP); pinMode(pinR1, INPUT_PULLUP);
  pinMode(pinR2, INPUT_PULLUP);

  stopMotor();
}

void loop() {
  if (Serial2.available() > 0) {
    String command = Serial2.readStringUntil('\n');
    command.trim(); command.toLowerCase();
    
    if (command.indexOf("auto") >= 0) isAutoMode = true;
    else if (command.indexOf("manual") >= 0) { isAutoMode = false; stopMotor(); }

    if (!isAutoMode) {
      if      (command.indexOf("forward") >= 0)  setSpeed(200, 200);
      else if (command.indexOf("backward") >= 0) setSpeed(-200, -200);
      else if (command.indexOf("left") >= 0)     setSpeed(-180, 180);
      else if (command.indexOf("right") >= 0)    setSpeed(180, -180);
      else if (command.indexOf("stop") >= 0)     stopMotor();
    }
  }

  if (isAutoMode) runPID();
}

void runPID() {
  int s[5] = {digitalRead(pinL2), digitalRead(pinL1), digitalRead(pinC), digitalRead(pinR1), digitalRead(pinR2)};
  
  int blackCount = 0;
  for(int i=0; i<5; i++) if(s[i] == 0) blackCount++;
  if (blackCount >= 4) { brakeMotor(); isAutoMode = false; return; }

  int error = 0;
  if      (s[0] == 0) error = -4;
  else if (s[1] == 0) error = -2;
  else if (s[2] == 0) error = 0;
  else if (s[3] == 0) error = 2;
  else if (s[4] == 0) error = 4;
  else if (blackCount == 0) { setSpeed(-100, -100); return; }

  int output = (Kp * error) + (Kd * (error - lastError));
  lastError = error;

  setSpeed(baseSpeed + output, baseSpeed - output);
}

void setSpeed(int left, int right) {
  left = constrain(left, -255, 255);
  right = constrain(right, -255, 255);

  if (left >= 0) { analogWrite(AIN1, 0); analogWrite(AIN2, left); }
  else { analogWrite(AIN1, abs(left)); analogWrite(AIN2, 0); }

  if (right >= 0) { analogWrite(BIN1, 0); analogWrite(BIN2, right); }
  else { analogWrite(BIN1, abs(right)); analogWrite(BIN2, 0); }
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