#include <PS4Controller.h>

const int MOTOR_LEFT_PIN1 = 12;
const int MOTOR_LEFT_PIN2 = 13;
const int MOTOR_RIGHT_PIN1 = 14;
const int MOTOR_RIGHT_PIN2 = 15;
const int LED_PIN = 4;
const int LED_CHANNEL = 7;
const int LED_BRIGHTNESS = 100;

const char* PS4_MAC = "0a:62:28:18:1b:50";
const int DEADZONE = 15;
const int MAX_SPEED = 255;
const int DEBOUNCE_MS = 300;

bool ledOn = false;

void setMotor(int pin1, int pin2, int pwm) {
  if (pwm == 0) {
    analogWrite(pin1, 0);
    analogWrite(pin2, 0);
  } else if (pwm > 0) {
    analogWrite(pin1, 0);
    analogWrite(pin2, pwm);
  } else {
    analogWrite(pin1, -pwm);
    analogWrite(pin2, 0);
  }
}

void stopAllMotors() {
  setMotor(MOTOR_LEFT_PIN1, MOTOR_LEFT_PIN2, 0);
  setMotor(MOTOR_RIGHT_PIN1, MOTOR_RIGHT_PIN2, 0);
}

void toggleLED() {
  ledOn = !ledOn;
  ledcWrite(LED_CHANNEL, ledOn ? LED_BRIGHTNESS : 0);
}

int mapStickToSpeed(int val) {
  return abs(val) < DEADZONE ? 0 : map(val, -128, 127, -MAX_SPEED, MAX_SPEED);
}

void onConnect() {
  Serial.println("Connected");
  PS4.setLed(124, 252, 0);
  PS4.sendToController();
}

void onDisconnect() {
  Serial.println("Disconnected");
  stopAllMotors();
  ledOn = false;
  ledcWrite(LED_CHANNEL, 0);
}

void setup() {
  Serial.begin(115200);
  
  pinMode(MOTOR_LEFT_PIN1, OUTPUT);
  pinMode(MOTOR_LEFT_PIN2, OUTPUT);
  pinMode(MOTOR_RIGHT_PIN1, OUTPUT);
  pinMode(MOTOR_RIGHT_PIN2, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  
  stopAllMotors();
  
  ledcSetup(LED_CHANNEL, 5000, 8);
  ledcAttachPin(LED_PIN, LED_CHANNEL);
  ledcWrite(LED_CHANNEL, 0);
  
  PS4.begin(PS4_MAC);
  PS4.attachOnConnect(onConnect);
  PS4.attachOnDisconnect(onDisconnect);
}

void loop() {
  if (PS4.isConnected()) {
    setMotor(MOTOR_LEFT_PIN1, MOTOR_LEFT_PIN2, mapStickToSpeed(PS4.LStickY()));
    setMotor(MOTOR_RIGHT_PIN1, MOTOR_RIGHT_PIN2, mapStickToSpeed(PS4.RStickY()));
    
    if (PS4.Cross()) {
      static unsigned long lastPress = 0;
      if (millis() - lastPress > DEBOUNCE_MS) {
        toggleLED();
        lastPress = millis();
      }
    }
  }
  delay(10);
}