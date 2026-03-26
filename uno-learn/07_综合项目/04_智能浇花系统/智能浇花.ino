/*
 * 综合项目：智能浇花系统
 * 功能：
 *   1. 土壤湿度检测
 *   2. 低于阈值自动浇水（继电器控制水泵）
 *   3. DHT11 监测环境温湿度
 *   4. LCD 显示状态
 *   5. 定时浇水（可选）
 * 
 * 需要安装库：DHT sensor library, LiquidCrystal_I2C
 * 
 * 硬件：
 *   土壤湿度传感器 AO → A0
 *   DHT11 → D2
 *   继电器（水泵）→ D8
 *   LCD I2C → A4, A5
 *   LED指示 → D13
 */

#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define DHT_PIN 2
DHT dht(DHT_PIN, DHT11);
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int SOIL_PIN  = A0;
const int PUMP_PIN  = 8;   // 继电器控制水泵
const int LED_PIN   = 13;

// 湿度阈值
const int DRY_THRESHOLD = 600;   // 低于此值需要浇水
const int WET_THRESHOLD = 300;   // 高于此值停止浇水

// 浇水参数
const unsigned long WATER_DURATION = 5000;   // 每次浇水5秒
const unsigned long CHECK_INTERVAL = 60000;  // 每分钟检测一次

bool isWatering = false;
unsigned long waterStartTime = 0;
unsigned long lastCheckTime  = 0;
int waterCount = 0;

void setup() {
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, HIGH);  // 继电器低电平触发，初始关闭
  
  dht.begin();
  lcd.init();
  lcd.backlight();
  Serial.begin(9600);
  
  lcd.print("智能浇花系统");
  lcd.setCursor(0, 1);
  lcd.print("正在初始化...");
  delay(2000);
  Serial.println("智能浇花系统启动");
}

void loop() {
  unsigned long now = millis();
  
  // 停止浇水检查
  if (isWatering && (now - waterStartTime >= WATER_DURATION)) {
    stopWatering();
  }
  
  // 定时检测
  if (!isWatering && (now - lastCheckTime >= CHECK_INTERVAL)) {
    lastCheckTime = now;
    checkAndWater();
  }
  
  // 更新显示（每5秒）
  static unsigned long lastDisplay = 0;
  if (now - lastDisplay >= 5000) {
    lastDisplay = now;
    updateDisplay();
  }
}

void checkAndWater() {
  int soilValue = analogRead(SOIL_PIN);
  Serial.print("土壤湿度: ");
  Serial.print(soilValue);
  
  if (soilValue > DRY_THRESHOLD) {
    Serial.println(" → 干燥，开始浇水");
    startWatering();
  } else {
    Serial.println(" → 湿润，无需浇水");
  }
}

void startWatering() {
  isWatering = true;
  waterStartTime = millis();
  waterCount++;
  digitalWrite(PUMP_PIN, LOW);   // 继电器吸合，水泵启动
  digitalWrite(LED_PIN, HIGH);
  Serial.print("浇水开始（第");
  Serial.print(waterCount);
  Serial.println("次）");
}

void stopWatering() {
  isWatering = false;
  digitalWrite(PUMP_PIN, HIGH);  // 继电器断开，水泵停止
  digitalWrite(LED_PIN, LOW);
  Serial.println("浇水完成");
}

void updateDisplay() {
  float temp = dht.readTemperature();
  float humi = dht.readHumidity();
  int soil = analogRead(SOIL_PIN);
  int soilPct = map(soil, 1023, 0, 0, 100);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  if (!isnan(temp)) {
    lcd.print("T:");
    lcd.print(temp, 1);
    lcd.print("C H:");
    lcd.print(humi, 0);
    lcd.print("%");
  }
  
  lcd.setCursor(0, 1);
  lcd.print("土壤:");
  lcd.print(soilPct);
  lcd.print("% ");
  lcd.print(isWatering ? "浇水中" : (soilPct < 40 ? "缺水!" : "正常"));
}