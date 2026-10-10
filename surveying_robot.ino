#include <WiFi.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

const char* ssid = "device";
const char* wifiPassword = "12345678";
const char* otaHostname = "ESP32-Robot";
const char* otaPassword = "ESP32OTA123";

Adafruit_MPU6050 mpu;

const int FL_IN1 = 25; const int FL_IN2 = 26;
const int FR_IN1 = 27; const int FR_IN2 = 14;
const int BL_IN1 = 32; const int BL_IN2 = 33;
const int BR_IN1 = 18; const int BR_IN2 = 19;

const int FL_EN = 13; const int FR_EN = 23;
const int BL_EN = 4;  const int BR_EN = 21; // GPIO 22 free for MPU6050 SCL

const unsigned long FORWARD_TIME = 5000;
const unsigned long STOP_TIME = 1000;
const unsigned long REVERSE_TIME = 5000;
const unsigned long IMU_INTERVAL = 500;
unsigned long lastImuRead = 0;

enum MotorState { FORWARD, STOP_BETWEEN, REVERSE, FINISHED };
MotorState state = FORWARD;
unsigned long stateStart = 0;
bool otaStarted = false;

void stopAllMotors() {
  digitalWrite(FL_EN, LOW); digitalWrite(FR_EN, LOW);
  digitalWrite(BL_EN, LOW); digitalWrite(BR_EN, LOW);
  digitalWrite(FL_IN1, LOW); digitalWrite(FL_IN2, LOW);
  digitalWrite(FR_IN1, LOW); digitalWrite(FR_IN2, LOW);
  digitalWrite(BL_IN1, LOW); digitalWrite(BL_IN2, LOW);
  digitalWrite(BR_IN1, LOW); digitalWrite(BR_IN2, LOW);
}

void moveForward() {
  stopAllMotors();
  digitalWrite(FL_IN1, HIGH); digitalWrite(FL_IN2, LOW);
  digitalWrite(FR_IN1, HIGH); digitalWrite(FR_IN2, LOW);
  digitalWrite(BL_IN1, HIGH); digitalWrite(BL_IN2, LOW);
  digitalWrite(BR_IN1, HIGH); digitalWrite(BR_IN2, LOW);
  digitalWrite(FL_EN, HIGH); digitalWrite(FR_EN, HIGH);
  digitalWrite(BL_EN, HIGH); digitalWrite(BR_EN, HIGH);
}

void moveReverse() {
  stopAllMotors();
  digitalWrite(FL_IN1, LOW); digitalWrite(FL_IN2, HIGH);
  digitalWrite(FR_IN1, LOW); digitalWrite(FR_IN2, HIGH);
  digitalWrite(BL_IN1, LOW); digitalWrite(BL_IN2, HIGH);
  digitalWrite(BR_IN1, LOW); digitalWrite(BR_IN2, HIGH);
  digitalWrite(FL_EN, HIGH); digitalWrite(FR_EN, HIGH);
  digitalWrite(BL_EN, HIGH); digitalWrite(BR_EN, HIGH);
}

void startOTA() {
  ArduinoOTA.setHostname(otaHostname);
  ArduinoOTA.setPassword(otaPassword);
  ArduinoOTA.onStart([]() { stopAllMotors(); Serial.println("{\"event\":\"OTA_START\"}"); });
  ArduinoOTA.begin();
  otaStarted = true;
}

void readAndPrintIMU() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  Serial.print("{\"imu\":{\"accel_x\":");
  Serial.print(a.acceleration.x);
  Serial.print(",\"accel_y\":");
  Serial.print(a.acceleration.y);
  Serial.print(",\"accel_z\":");
  Serial.print(a.acceleration.z);
  Serial.print(",\"gyro_x\":");
  Serial.print(g.gyro.x);
  Serial.print(",\"gyro_y\":");
  Serial.print(g.gyro.y);
  Serial.print(",\"gyro_z\":");
  Serial.print(g.gyro.z);
  Serial.println("}}");
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  if (!mpu.begin()) {
    Serial.println("{\"status\":\"ERROR\",\"message\":\"MPU6050 not found! Check wiring.\"}");
    while (1) { delay(10); }
  }
  Serial.println("{\"status\":\"INFO\",\"message\":\"MPU6050 Initialized Successfully\"}");

  pinMode(FL_IN1, OUTPUT); pinMode(FL_IN2, OUTPUT);
  pinMode(FR_IN1, OUTPUT); pinMode(FR_IN2, OUTPUT);
  pinMode(BL_IN1, OUTPUT); pinMode(BL_IN2, OUTPUT);
  pinMode(BR_IN1, OUTPUT); pinMode(BR_IN2, OUTPUT);

  pinMode(FL_EN, OUTPUT); pinMode(FR_EN, OUTPUT);
  pinMode(BL_EN, OUTPUT); pinMode(BR_EN, OUTPUT);

  stopAllMotors();
  delay(200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, wifiPassword);

  moveForward();
  state = FORWARD;
  stateStart = millis();

  Serial.println("{\"status\":\"STARTED\", \"state\":\"FORWARD\", \"duration_sec\":5}");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED && !otaStarted) {
    startOTA();
  }
  if (otaStarted) {
    ArduinoOTA.handle();
  }

  unsigned long currentMillis = millis();
  if (currentMillis - lastImuRead >= IMU_INTERVAL) {
    lastImuRead = currentMillis;
    readAndPrintIMU();
  }

  unsigned long elapsed = currentMillis - stateStart;
  switch (state) {
    case FORWARD:
      if (elapsed >= FORWARD_TIME) {
        stopAllMotors();
        state = STOP_BETWEEN;
        stateStart = millis();
        Serial.println("{\"status\":\"RUNNING\", \"state\":\"STOP_BETWEEN\", \"duration_sec\":1}");
      }
      break;
    case STOP_BETWEEN:
      if (elapsed >= STOP_TIME) {
        moveReverse();
        state = REVERSE;
        stateStart = millis();
        Serial.println("{\"status\":\"RUNNING\", \"state\":\"REVERSE\", \"duration_sec\":5}");
      }
      break;
    case REVERSE:
      if (elapsed >= REVERSE_TIME) {
        stopAllMotors();
        state = FINISHED;
        Serial.println("{\"status\":\"FINISHED\", \"state\":\"STOPPED\"}");
      }
      break;
    case FINISHED:
      stopAllMotors();
      break;
  }
  delay(10);
}
