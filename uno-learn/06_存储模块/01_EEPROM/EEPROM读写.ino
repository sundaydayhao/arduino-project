/*
 * 实验名称：EEPROM 读写
 * 实验目的：学习使用 Arduino 内置 EEPROM 存储数据
 * 
 * 知识点：
 *   - EEPROM：掉电不丢失，写入寿命约 10 万次
 *   - Arduino UNO：1KB EEPROM（地址 0~1023）
 *   - EEPROM 库（Arduino 内置）
 */

#include <EEPROM.h>

void setup() {
  Serial.begin(9600);
  Serial.println("EEPROM 读写实验");
  Serial.println("命令: write <addr> <val> | read <addr> | clear | dump");
}

void loop() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    if (cmd.startsWith("write ")) {
      // 格式: write 0 123
      int spaceIdx = cmd.indexOf(' ', 6);
      int addr = cmd.substring(6, spaceIdx).toInt();
      int val  = cmd.substring(spaceIdx + 1).toInt();
      
      if (addr >= 0 && addr < EEPROM.length()) {
        EEPROM.write(addr, (byte)val);
        Serial.print("写入 [");
        Serial.print(addr);
        Serial.print("] = ");
        Serial.println(val);
      } else {
        Serial.println("地址超出范围！");
      }
    }
    else if (cmd.startsWith("read ")) {
      int addr = cmd.substring(5).toInt();
      byte val = EEPROM.read(addr);
      Serial.print("读取 [");
      Serial.print(addr);
      Serial.print("] = ");
      Serial.println(val);
    }
    else if (cmd == "clear") {
      Serial.println("清除 EEPROM...");
      for (int i = 0; i < EEPROM.length(); i++) {
        EEPROM.write(i, 0xFF);
      }
      Serial.println("完成！");
    }
    else if (cmd == "dump") {
      Serial.println("EEPROM 内容（前64字节）:");
      for (int i = 0; i < 64; i++) {
        if (i % 16 == 0) {
          Serial.print("\n[");
          if (i < 10) Serial.print("0");
          Serial.print(i);
          Serial.print("] ");
        }
        byte val = EEPROM.read(i);
        if (val < 0x10) Serial.print("0");
        Serial.print(val, HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
    else if (cmd == "size") {
      Serial.print("EEPROM 大小: ");
      Serial.print(EEPROM.length());
      Serial.println(" 字节");
    }
  }
}