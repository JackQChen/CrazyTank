#include <PS4Controller.h>

// 硬件配置
int DRV_A = 12;
int DRV_B = 13;
int DIR_A = 15;
int DIR_B = 14;
int ledPin = 4;
int ledVal = 100;
bool ledOn = false;

// 控制参数
const char* PS4_MAC = "0a:62:28:18:1b:50";
const int DEADZONE = 10;
const int MAX_SPEED = 255;
const int DEBOUNCE = 300;

// 电机控制
void setMotor(int speedPin, int dirPin, int speed) {
  if (speed > 0) {
    analogWrite(speedPin, speed);
    analogWrite(dirPin, 0);
  } else if (speed < 0) {
    analogWrite(speedPin, 0);
    analogWrite(dirPin, -speed);
  } else {
    analogWrite(speedPin, 0);
    analogWrite(dirPin, 0);
  }
}

void stopAll() {
  setMotor(DRV_A, DRV_B, 0);
  setMotor(DIR_A, DIR_B, 0);
}

// LED控制
void initLED() {
  ledcSetup(7, 5000, 8);
  ledcAttachPin(ledPin, 7);
}

void toggleLED() {
  ledOn = !ledOn;
  ledcWrite(7, ledOn ? ledVal : 0);
}

// PS4事件
void onConnect() {
  Serial.println("Connected");
  PS4.setLed(124, 252, 0);
  PS4.sendToController();
}

void onDisconnect() {
  Serial.println("Disconnected");
  stopAll();
}

// 摇杆映射
int mapStick(int val) {
  return abs(val) < DEADZONE ? 0 : map(val, -128, 127, -MAX_SPEED, MAX_SPEED);
}

void setup() {
  Serial.begin(115200);
  
  pinMode(DRV_A, OUTPUT);
  pinMode(DRV_B, OUTPUT);
  pinMode(DIR_A, OUTPUT);
  pinMode(DIR_B, OUTPUT);
  pinMode(ledPin, OUTPUT);
  
  stopAll();
  initLED();
  
  PS4.begin(PS4_MAC);
  PS4.attachOnConnect(onConnect);
  PS4.attachOnDisconnect(onDisconnect);
}

void loop() {
  if (PS4.isConnected()) {
    setMotor(DRV_A, DRV_B, mapStick(PS4.LStickY()));
    setMotor(DIR_A, DIR_B, mapStick(PS4.RStickY()));
    
    if (PS4.Cross()) {
      static unsigned long last = 0;
      if (millis() - last > DEBOUNCE) {
        toggleLED();
        last = millis();
      }
    }
  }
  delay(10);
}