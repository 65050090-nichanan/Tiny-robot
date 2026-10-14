// ======================================================
// STM32 Robot: Final Version (With Active Brake & Finish Line)
// พินมอเตอร์: ซ้าย PA0, PA1 | ขวา PA4, PA5
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

bool isAutoMode = false;
int sensorValues[5]; 

void setup() {
  Serial.begin(115200);   
  Serial2.begin(115200);  

  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
  
  pinMode(pinL2, INPUT_PULLUP);
  pinMode(pinL1, INPUT_PULLUP);
  pinMode(pinC,  INPUT_PULLUP);
  pinMode(pinR1, INPUT_PULLUP);
  pinMode(pinR2, INPUT_PULLUP);

  stopMotor();
  Serial.println(">>> System Ready: Finish Line Detection Enabled <<<");
}

void loop() {
  readSensors();

  // --- ส่วนรับคำสั่งจาก Bluetooth ---
  if (Serial2.available() > 0) {
    String command = Serial2.readStringUntil('\n');
    command.trim();         
    command.toLowerCase();  
    
    if (command.indexOf("auto") >= 0) {
      isAutoMode = true;
      stopMotor(); 
      Serial.println(">>>> MODE: AUTO <<<<");
    } 
    else if (command.indexOf("manual") >= 0) {
      isAutoMode = false;
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

  if (isAutoMode) {
    runLineFollower();
  }

  static uint32_t lastDebug = 0;
  if (millis() - lastDebug > 200) {
    printSensorDebug();
    lastDebug = millis();
  }
}

void readSensors() {
  sensorValues[0] = digitalRead(pinL2);
  sensorValues[1] = digitalRead(pinL1);
  sensorValues[2] = digitalRead(pinC);
  sensorValues[3] = digitalRead(pinR1);
  sensorValues[4] = digitalRead(pinR2);
}

void runLineFollower() {
  int L2 = sensorValues[0];
  int L1 = sensorValues[1];
  int C  = sensorValues[2];
  int R1 = sensorValues[3];
  int R2 = sensorValues[4];

  // --- 1. เช็คเส้นชัย (ดำ 4 ดวงขึ้นไป) ---
  int blackCount = (L2==0) + (L1==0) + (C==0) + (R1==0) + (R2==0);
  if (blackCount >= 4) { 
    brakeMotor();      // เบรกกึก
    isAutoMode = false; // ปิดออโต้ทันที
    Serial.println(">>>> FINISH LINE! STOPPING... <<<<");
    return;
  }
  
  // --- 2. กรณีหลุดเส้น (ขาวหมด) ---
  if (L2 == 1 && L1 == 1 && C == 1 && R1 == 1 && R2 == 1) {
    moveBackward();
    return;
  }

  // --- 3. การตัดสินใจทิศทางปกติ ---
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

// --- ฟังก์ชันมอเตอร์ ---

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
  // Active Braking: ล็อคมอเตอร์ด้วยการส่ง HIGH ทั้งคู่
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, HIGH);
  delay(150); // เบรกค้างไว้ 150ms
  stopMotor();
}

void printSensorDebug() {
  Serial.print("Sensors: ");
  for(int i=0; i<5; i++) { Serial.print(sensorValues[i]); Serial.print(" "); }
  Serial.println(isAutoMode ? "| [AUTO]" : "| [MANUAL]");
}