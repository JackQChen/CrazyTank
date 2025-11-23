#include <ps4.h>
#include <ps4_int.h>
#include "esp_camera.h"
#include <WiFi.h>
#include <PS4Controller.h>
#include <ESP32Servo.h>

/* 摄像头引脚定义 - AI_THINKER模型 */
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

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

// AP设置
const char* AP_SSID = "TANK-AP";

// 摇杆控制相关变量
int leftSpeed = 0;   // 左履带速度
int rightSpeed = 0;  // 右履带速度
const int DEADZONE = 10; // 摇杆死区值
const int MAX_SPEED = 255; // 最大速度值

void startCameraServer();

void initLed() {
	ledcSetup(7, 5000, 8); // 5000 Hz PWM, 8-bit分辨率，范围0-255
	ledcAttachPin(ledPin, 7);
}

void setupCamera() {
	camera_config_t config;
	config.ledc_channel = LEDC_CHANNEL_0;
	config.ledc_timer = LEDC_TIMER_0;
	config.pin_d0 = Y2_GPIO_NUM;
	config.pin_d1 = Y3_GPIO_NUM;
	config.pin_d2 = Y4_GPIO_NUM;
	config.pin_d3 = Y5_GPIO_NUM;
	config.pin_d4 = Y6_GPIO_NUM;
	config.pin_d5 = Y7_GPIO_NUM;
	config.pin_d6 = Y8_GPIO_NUM;
	config.pin_d7 = Y9_GPIO_NUM;
	config.pin_xclk = XCLK_GPIO_NUM;
	config.pin_pclk = PCLK_GPIO_NUM;
	config.pin_vsync = VSYNC_GPIO_NUM;
	config.pin_href = HREF_GPIO_NUM;
	config.pin_sscb_sda = SIOD_GPIO_NUM;
	config.pin_sscb_scl = SIOC_GPIO_NUM;
	config.pin_pwdn = PWDN_GPIO_NUM;
	config.pin_reset = RESET_GPIO_NUM;
	config.xclk_freq_hz = 20000000;
	config.pixel_format = PIXFORMAT_JPEG;

	// 根据是否有PSRAM初始化相应的配置
	if (psramFound()) {
		config.frame_size = FRAMESIZE_UXGA;
		config.jpeg_quality = 10;
		config.fb_count = 2;
	}
	else {
		config.frame_size = FRAMESIZE_SVGA;
		config.jpeg_quality = 10;
		config.fb_count = 1;
	}

	// 初始化摄像头
	esp_err_t err = esp_camera_init(&config);
	if (err != ESP_OK) {
		Serial.printf("Camera init failed, Error code: 0x%x\n", err);
		return;
	}

	// 降低分辨率以提高初始帧率
	sensor_t* s = esp_camera_sensor_get();
	s->set_framesize(s, FRAMESIZE_QVGA);
}

void setupWiFiAP() {
	WiFi.mode(WIFI_AP);
	WiFi.softAP(AP_SSID);
	IPAddress myIP = WiFi.softAPIP();
	Serial.println("WiFi AP Started");
	Serial.print("AP IP: ");
	Serial.println(myIP);
	Serial.print("AP Name: ");
	Serial.println(AP_SSID);
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
	Serial.setDebugOutput(true);
	Serial.println();
	Serial.println("*ESP32 Camera Tank - PS4*");
	Serial.println("--------------------------------------------------------");

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

	//// 设置PS4控制器
	//setupPS4Controller();

	// 初始化摄像头
	setupCamera();

	// 设置WiFi AP模式
	setupWiFiAP();

	// 启动摄像头服务器
	startCameraServer();
	Serial.print("Camera Ready! Use 'http://");
	Serial.print(WiFi.softAPIP());
	Serial.println("' to view the video");
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
	delay(10);
}