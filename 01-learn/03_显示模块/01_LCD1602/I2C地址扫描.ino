/*
 * 工具程序：I2C 地址扫描
 * 用途：扫描连接到 I2C 总线的所有设备地址
 * 
 * 硬件连接：
 *   SDA → A4
 *   SCL → A5
 * 
 * 使用方法：
 *   上传后打开串口监视器，查看扫描结果
 *   LCD1602 通常在 0x27 或 0x3F
 */

#include <Wire.h>

void setup() {
  Wire.begin();
  Serial.begin(9600);
  
  while (!Serial);
  
  Serial.println("I2C 地址扫描器");
  Serial.println("扫描中...");
  
  int deviceCount = 0;
  
  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("发现设备，地址: 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(" !");
      deviceCount++;
    } else if (error == 4) {
      Serial.print("地址 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(" 发生未知错误");
    }
  }
  
  if (deviceCount == 0) {
    Serial.println("未发现 I2C 设备！");
    Serial.println("请检查接线是否正确");
  } else {
    Serial.print("共发现 ");
    Serial.print(deviceCount);
    Serial.println(" 个设备");
  }
}

void loop() {
  // 每 5 秒重新扫描
  delay(5000);
  Serial.println("\n重新扫描...");
  
  int deviceCount = 0;
  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      deviceCount++;
    }
  }
  Serial.print("共 ");
  Serial.print(deviceCount);
  Serial.println(" 个设备");
}