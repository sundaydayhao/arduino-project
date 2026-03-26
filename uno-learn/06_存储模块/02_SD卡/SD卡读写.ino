/*
 * 实验名称：SD 卡读写
 * 实验目的：学习使用 SD 卡模块存储数据
 * 
 * 需要安装库：SD（Arduino 内置）
 * 
 * 硬件连接（SPI 接口）：
 *   CS   → D4（或D10）
 *   MOSI → D11
 *   MISO → D12
 *   SCK  → D13
 *   VCC  → 5V（模块内置3.3V稳压）
 *   GND  → GND
 */

#include <SPI.h>
#include <SD.h>

const int CS_PIN = 4;

void setup() {
  Serial.begin(9600);
  Serial.println("SD 卡读写实验");
  
  if (!SD.begin(CS_PIN)) {
    Serial.println("SD 卡初始化失败！检查接线和卡片");
    return;
  }
  Serial.println("SD 卡初始化成功！");
  
  // 写入文件
  writeFile("data.txt", "Hello, Arduino SD Card!\n");
  writeFile("data.txt", "第二行数据\n");
  
  // 读取文件
  readFile("data.txt");
  
  // 列出文件
  listFiles();
}

void loop() {
  // 每秒记录一次传感器数据
  static unsigned long lastLog = 0;
  if (millis() - lastLog > 1000) {
    lastLog = millis();
    
    // 模拟传感器数据
    float temp = 25.0 + random(-50, 50) / 10.0;
    
    // 追加写入 CSV 文件
    File dataFile = SD.open("log.csv", FILE_WRITE);
    if (dataFile) {
      dataFile.print(millis() / 1000);
      dataFile.print(",");
      dataFile.println(temp);
      dataFile.close();
    }
  }
}

void writeFile(const char* filename, const char* content) {
  File file = SD.open(filename, FILE_WRITE);
  if (file) {
    file.print(content);
    file.close();
    Serial.print("写入文件: ");
    Serial.println(filename);
  } else {
    Serial.print("无法打开文件: ");
    Serial.println(filename);
  }
}

void readFile(const char* filename) {
  File file = SD.open(filename);
  if (file) {
    Serial.print("读取文件 ");
    Serial.print(filename);
    Serial.println(":");
    while (file.available()) {
      Serial.write(file.read());
    }
    file.close();
  } else {
    Serial.println("文件不存在！");
  }
}

void listFiles() {
  Serial.println("\nSD 卡文件列表:");
  File root = SD.open("/");
  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;
    Serial.print("  ");
    Serial.print(entry.name());
    Serial.print("\t");
    Serial.println(entry.size());
    entry.close();
  }
  root.close();
}