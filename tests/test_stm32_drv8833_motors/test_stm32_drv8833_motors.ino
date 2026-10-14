// Motor A (ซ้าย)
#define AIN1 PA0
#define AIN2 PA1

// Motor B (ขวา)
#define BIN1 PA2
#define BIN2 PA3

void setup() {
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
}

void loop() {

  //  เดินหน้า
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);
  delay(3000);

  stopMotor();
  delay(1000);

  //  ถอยหลัง
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);
  delay(3000);

  stopMotor();
  delay(1000);

  //  เลี้ยวซ้าย
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);
  delay(2000);

  stopMotor();
  delay(1000);

  //  เลี้ยวขวา
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);
  delay(2000);

  stopMotor();
  delay(2000);
}

// ฟังก์ชันหยุด
void stopMotor() {
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, LOW);
}