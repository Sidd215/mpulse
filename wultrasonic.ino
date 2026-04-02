#define REMOTEXY_MODE__ESP32CORE_WIFI_POINT

#include <WiFi.h>
#include <RemoteXY.h>

// WiFi settings
#define REMOTEXY_WIFI_SSID "ESP32_CAR"
#define REMOTEXY_WIFI_PASSWORD "12345678"
#define REMOTEXY_SERVER_PORT 6377

#pragma pack(push, 1)

uint8_t RemoteXY_CONF[] = 
{ 
  255,3,0,0,0,48,0,19,0,0,0,69,83,80,51,50,95,67,65,82,
  0,31,1,106,200,1,1,2,0,2,51,12,44,22,0,2,26,31,31,79,
  78,0,79,70,70,0,5,12,65,60,60,32,2,26,31 
};

struct {
  uint8_t switch_01;
  int8_t joystick_01_x;
  int8_t joystick_01_y;
} RemoteXY;

#pragma pack(pop)

// Motor pins
#define IN1 19 
#define IN2 17 
#define IN3 22 
#define IN4 23 
#define ENA 16 
#define ENB 21 

// IR sensors
#define IR_LEFT 34
#define IR_RIGHT 35

// Ultrasonic Sensor Pins
#define TRIG_PIN 5
#define ECHO_PIN 18

float duration_us, distance_cm;

int currentLeft = 0;
int currentRight = 0;

void setupMotors() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
}

// Smooth ramp
int smooth(int current, int target, int step = 10) {
  if (current < target) return min(current + step, target);
  if (current > target) return max(current - step, target);
  return current;
}

void setMotor(int left, int right) {

  // LEFT motor
  if (left >= 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    left = -left;
  }

  // RIGHT motor
  if (right >= 0) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  } else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    right = -right;
  }

  left = constrain(left, 0, 255);
  right = constrain(right, 0, 255);

  analogWrite(ENA, left);
  analogWrite(ENB, right);
}

// 🔹 Ultrasonic function (UNCHANGED)
float checkDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration_us = pulseIn(ECHO_PIN, HIGH);

  distance_cm = 0.017 * duration_us;

  return distance_cm;
}

// 🔹 Stop motors
void stopMotors() {
  setMotor(0, 0);
}

void setup() {
  Serial.begin(115200);

  WiFi.setSleep(false);

  RemoteXY_Init();
  setupMotors();

  pinMode(IR_LEFT, INPUT);
  pinMode(IR_RIGHT, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.println("System Ready");
}

void loop() {
  RemoteXY_Handler();

  // 🔴 ULTRASONIC CHECK (ADDED)
  float dist = checkDistance();

  if (dist > 0 && dist < 20) {
    Serial.println("Obstacle detected! Stopping...");
    stopMotors();
    return;
  }

  int x = RemoteXY.joystick_01_x;
  int y = RemoteXY.joystick_01_y;

  if (abs(x) < 5) x = 0;
  if (abs(y) < 5) y = 0;

  int speedX = map(x, -100, 100, -255, 255);
  int speedY = map(y, -100, 100, -255, 255);

  // ================= MANUAL MODE =================
  if (RemoteXY.switch_01 == 0) {

    int targetLeft  = speedY + speedX;
    int targetRight = speedY - speedX;

    currentLeft  = smooth(currentLeft, targetLeft);
    currentRight = smooth(currentRight, targetRight);

    setMotor(currentLeft, currentRight);
  } 

  // ================= AUTO MODE =================
  else {

    int leftIR  = digitalRead(IR_LEFT);
    int rightIR = digitalRead(IR_RIGHT);

    Serial.print("L: ");
    Serial.print(leftIR);
    Serial.print("  R: ");
    Serial.println(rightIR);

    int baseSpeed = 50;
    int turnSpeed = 50;

    if (leftIR == LOW && rightIR == LOW) {
      currentLeft  = smooth(currentLeft, baseSpeed);
      currentRight = smooth(currentRight, baseSpeed);
    }

    else if (leftIR == HIGH && rightIR == HIGH) {
      currentLeft  = smooth(currentLeft, 0);
      currentRight = smooth(currentRight, 0);
    }

    else if (leftIR == HIGH && rightIR == LOW) {
      currentLeft  = smooth(currentLeft, turnSpeed);
      currentRight = smooth(currentRight, -turnSpeed);
    }

    else if (leftIR == LOW && rightIR == HIGH) {
      currentLeft  = smooth(currentLeft, -turnSpeed);
      currentRight = smooth(currentRight, turnSpeed);
    }

    setMotor(currentLeft, currentRight);
  }

  delay(5);
}
