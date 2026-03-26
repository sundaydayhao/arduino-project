/*
 * 实验名称：土壤湿度传感器
 * 实验目的：检测土壤湿度，实现自动浇水提醒
 * 
 * 硬件连接：
 *   VCC → 5V（或3.3V）
 *   GND → GND
 *   DO  → D2（数字输出）
 *   AO  → A0（模拟输出）
 * 
 * 注意：长期通电会导致探针氧化腐蚀，
 *       建议只在测量时通电（用数字引脚控制 VCC）
 * 
 * 知识点：
 *   - 电阻式土壤湿度传感器原理
 *   - 延长传感器寿命的方法
 */

const int SOIL_DO  = 2;   // 数字输出
const int SOIL_AO  = A0;  // 模拟输出
const int LED_PIN  = 13;  // 缺水指示灯
const int PUMP_PIN = 8;   // 水泵控制（继电器）

// 湿度阈值（根据实际土壤和传感器校准）
const int DRY_THRESHOLD  = 700;  // 高于此值认为干燥
const int WET_THRESHOLD  = 300;  // 低于此值认为湿润

void setup() {
  pinMode(SOIL_DO, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW);
  
  Serial.begin(9600);
  Serial.println("土壤湿度传感器实验");
  Serial.println("湿度值 | 状态 | 操作");
  Serial.println("-------|------|------");
}

void loop() {
  int soilValue = analogRead(SOIL_AO);
  int doValue   = digitalRead(SOIL_DO);
  
  // 将 ADC 值转换为湿度百分比（值越大越干燥）
  int moisturePercent = map(soilValue, 1023, 0, 0, 100);
  moisturePercent = constrain(moisturePercent, 0, 100);
  
  // 判断湿度状态
  String status;
  if (soilValue > DRY_THRESHOLD) {
    status = "干燥";
    digitalWrite(LED_PIN, HIGH);   // 缺水指示灯亮
    // 可以在这里控制水泵
    // digitalWrite(PUMP_PIN, HIGH);
  } else if (soilValue < WET_THRESHOLD) {
    status = "湿润";
    digitalWrite(LED_PIN, LOW);
    digitalWrite(PUMP_PIN, LOW);
  } else {
    status = "适中";
    digitalWrite(LED_PIN, LOW);
    digitalWrite(PUMP_PIN, LOW);
  }
  
  Serial.print(soilValue);
  Serial.print(" (");
  Serial.print(moisturePercent);
  Serial.print("%) | ");
  Serial.print(status);
  Serial.print(" | DO=");
  Serial.println(doValue);
  
  delay(1000);
}