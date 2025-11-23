#include <ps4.h>
#include <ps4_int.h>
#include "esp_camera.h"
#include <PS4Controller.h>

/* 电机和LED引脚定义 */
extern int DRV_A = 12;  // 左履带控制引脚1
extern int DRV_B = 13;  // 左履带控制引脚2
extern int DIR_A = 15;  // 右履带控制引脚1
extern int DIR_B = 14;  // 右履带控制引脚2
extern int ledPin = 4;  // LED引脚（使用内置LED）
extern int ledVal = 100; // LED亮度，范围0-255
extern bool ledOn = false; // LED状态

// PS4手柄MAC地址 - 需要替换为你的PS4手柄MAC地址
const char* PS4_MAC = "0a:62:28:18:1b:50";

// 摇杆控制相关变量
int leftSpeed = 0;   // 左履带速度
int rightSpeed = 0;  // 右履带速度
const int DEADZONE = 10; // 摇杆死区值
const int MAX_SPEED = 255; // 最大速度值

void initLed() {
	ledcSetup(7, 5000, 8); // 5000 Hz PWM, 8-bit分辨率，范围0-255
	ledcAttachPin(ledPin, 7);
}

void setupPS4Controller() {
	// 初始化PS4控制器
	PS4.begin(PS4_MAC);
	Serial.println("PS4 Connecting...");

	// 设置事件回调
	PS4.attachOnConnect(onPS4Connect);
	PS4.attachOnDisconnect(onPS4Disconnect);
}

// PS4控制器连接事件
void onPS4Connect() {
	Serial.println("PS4 Connected");
	PS4.setLed(124, 252, 0); // 设置控制器灯为绿色
	PS4.sendToController();
}

// PS4控制器断开连接事件
void onPS4Disconnect() {
	Serial.println("PS4 Disconnected");
	// 停止所有电机
	leftSpeed = 0;
	rightSpeed = 0;
	controlMotors();
}

// 将PS4摇杆值转换为电机速度
int mapJoystickToSpeed(int value) {
	// 处理死区
	if (abs(value) < DEADZONE) {
		return 0;
	}

	// 映射摇杆值(-128到127)到电机速度(0到MAX_SPEED)
	if (value > 0) {
		return map(value, DEADZONE, 127, 0, MAX_SPEED);
	}
	else {
		return map(value, -128, -DEADZONE, -MAX_SPEED, 0);
	}
}

// 控制电机
void controlMotors() {
	// 左履带控制
	if (leftSpeed > 0) {
		analogWrite(DRV_A, 0);
		analogWrite(DRV_B, leftSpeed);
	}
	else if (leftSpeed < 0) {
		analogWrite(DRV_A, -leftSpeed);
		analogWrite(DRV_B, 0);
	}
	else {
		analogWrite(DRV_A, 0);
		analogWrite(DRV_B, 0);
	}

	// 右履带控制
	if (rightSpeed > 0) {
		analogWrite(DIR_A, 0);
		analogWrite(DIR_B, rightSpeed);
	}
	else if (rightSpeed < 0) {
		analogWrite(DIR_A, -rightSpeed);
		analogWrite(DIR_B, 0);
	}
	else {
		analogWrite(DIR_A, 0);
		analogWrite(DIR_B, 0);
	}
}

// 控制LED灯
void toggleLED() {
	ledOn = !ledOn;
	if (ledOn) {
		ledcWrite(7, ledVal);
		Serial.println("LED On");
	}
	else {
		ledcWrite(7, 0);
		Serial.println("LED Off");
	}
}

void setup() {
	Serial.begin(115200);
	Serial.println();
	Serial.println("*Crazy Tank - PS4*");

	// 设置电机控制引脚为输出
	pinMode(DRV_A, OUTPUT);
	pinMode(DRV_B, OUTPUT);
	pinMode(DIR_A, OUTPUT);
	pinMode(DIR_B, OUTPUT);
	pinMode(ledPin, OUTPUT);

	// 初始状态 - 关闭电机和LED
	analogWrite(DRV_A, 0);
	analogWrite(DRV_B, 0);
	analogWrite(DIR_A, 0);
	analogWrite(DIR_B, 0);
	digitalWrite(ledPin, LOW);

	// 初始化LED
	initLed();

	// 设置PS4控制器
	setupPS4Controller();
}

void loop() {
	// 检查PS4控制器是否连接
	if (PS4.isConnected()) {
		// 获取左右摇杆的Y轴值并控制履带
		leftSpeed = mapJoystickToSpeed(PS4.LStickY());
		rightSpeed = mapJoystickToSpeed(PS4.RStickY());

		// 应用电机控制
		controlMotors();

		// 检测X按键按下，控制LED
		if (PS4.Cross()) {
			static unsigned long lastPress = 0;
			unsigned long now = millis();

			// 简单的消抖处理
			if (now - lastPress > 300) {
				toggleLED();
				lastPress = now;
			}
		}
	}

	// 小延迟以减少CPU负载
	delay(1);
}