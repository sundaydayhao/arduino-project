/*
 * 实验名称：ESP8266 WiFi 通信
 * 实验目的：通过 WiFi 实现 Arduino 联网，发送传感器数据
 * 
 * 硬件连接（ESP-01 模块）：
 *   ESP-01 VCC  → 3.3V（必须！不能用5V）
 *   ESP-01 GND  → GND
 *   ESP-01 TX   → Arduino D10（SoftwareSerial RX）
 *   ESP-01 RX   → Arduino D11（SoftwareSerial TX，需分压）
 *   ESP-01 CH_PD → 3.3V（使能引脚）
 *   ESP-01 RST  → 3.3V（或悬空）
 * 
 * ⚠️ ESP8266 工作电压 3.3V，电流需求较大（约 300mA）
 *    必须使用 3.3V 稳压电源，不能直接用 Arduino 的 3.3V 引脚！
 * 
 * 功能：
 *   - 连接 WiFi
 *   - 发送 HTTP GET 请求
 *   - 接收服务器响应
 */

#include <SoftwareSerial.h>

SoftwareSerial esp8266(10, 11);  // RX, TX

// WiFi 配置（修改为你的 WiFi 信息）
const String WIFI_SSID = "你的WiFi名称";
const String WIFI_PASS = "你的WiFi密码";

// 服务器配置（示例：ThingSpeak）
const String SERVER = "api.thingspeak.com";
const String API_KEY = "你的API密钥";

void setup() {
  Serial.begin(9600);
  esp8266.begin(9600);
  
  Serial.println("ESP8266 WiFi 实验");
  
  // 初始化 ESP8266
  if (initESP8266()) {
    Serial.println("ESP8266 初始化成功");
    
    // 连接 WiFi
    if (connectWiFi()) {
      Serial.println("WiFi 连接成功！");
      String ip = sendATCommand("AT+CIFSR", 2000);
      Serial.println("IP 地址: " + ip);
    } else {
      Serial.println("WiFi 连接失败！");
    }
  } else {
    Serial.println("ESP8266 初始化失败！");
  }
}

void loop() {
  // 每 30 秒发送一次数据
  static unsigned long lastSend = 0;
  if (millis() - lastSend > 30000) {
    lastSend = millis();
    
    // 模拟传感器数据
    float temperature = 25.5;
    float humidity = 60.0;
    
    // 发送到 ThingSpeak
    sendToThingSpeak(temperature, humidity);
  }
}

bool initESP8266() {
  String response = sendATCommand("AT", 1000);
  if (response.indexOf("OK") == -1) return false;
  
  sendATCommand("AT+CWMODE=1", 1000);  // 设置为 Station 模式
  return true;
}

bool connectWiFi() {
  String cmd = "AT+CWJAP=\"" + WIFI_SSID + "\",\"" + WIFI_PASS + "\"";
  String response = sendATCommand(cmd, 10000);
  return response.indexOf("OK") != -1;
}

void sendToThingSpeak(float temp, float humi) {
  String data = "field1=" + String(temp) + "&field2=" + String(humi);
  int dataLen = data.length();
  
  sendATCommand("AT+CIPSTART=\"TCP\",\"" + SERVER + "\",80", 5000);
  
  String httpRequest = "GET /update?api_key=" + API_KEY + "&" + data + " HTTP/1.1\r\n";
  httpRequest += "Host: " + SERVER + "\r\n";
  httpRequest += "Connection: close\r\n\r\n";
  
  sendATCommand("AT+CIPSEND=" + String(httpRequest.length()), 2000);
  esp8266.print(httpRequest);
  
  String response = sendATCommand("", 5000);
  Serial.println("发送结果: " + response);
  
  sendATCommand("AT+CIPCLOSE", 1000);
}

String sendATCommand(String command, int timeout) {
  String response = "";
  
  if (command.length() > 0) {
    esp8266.println(command);
    Serial.print("发送: ");
    Serial.println(command);
  }
  
  unsigned long startTime = millis();
  while (millis() - startTime < (unsigned long)timeout) {
    while (esp8266.available()) {
      char c = esp8266.read();
      response += c;
    }
  }
  
  if (response.length() > 0) {
    Serial.print("响应: ");
    Serial.println(response);
  }
  
  return response;
}