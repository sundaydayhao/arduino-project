/*
 * 实验名称：RFID RC522 读卡
 * 实验目的：读取 RFID 卡片 UID，实现门禁控制
 * 
 * 需要安装库：MFRC522（by GithubCommunity）
 * 
 * 硬件连接（SPI 接口）：
 *   SDA(SS) → D10
 *   SCK     → D13
 *   MOSI    → D11
 *   MISO    → D12
 *   IRQ     → 不接
 *   GND     → GND
 *   RST     → D9
 *   3.3V    → 3.3V（注意：RC522 工作电压 3.3V！）
 */

#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN  10
#define RST_PIN 9

MFRC522 rfid(SS_PIN, RST_PIN);

// 授权卡片 UID（修改为你的卡片 UID）
byte authorizedUID[][4] = {
  {0xDE, 0xAD, 0xBE, 0xEF},  // 卡片1
  {0x12, 0x34, 0x56, 0x78}   // 卡片2
};
const int NUM_CARDS = 2;

const int LED_GREEN = 7;
const int LED_RED   = 8;
const int BUZZER    = 6;

void setup() {
  SPI.begin();
  rfid.PCD_Init();
  
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  
  Serial.begin(9600);
  Serial.println("RFID RC522 门禁系统");
  Serial.println("请刷卡...");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }
  
  // 打印 UID
  Serial.print("卡片 UID: ");
  String uidStr = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) Serial.print("0");
    Serial.print(rfid.uid.uidByte[i], HEX);
    Serial.print(" ");
    uidStr += String(rfid.uid.uidByte[i], HEX);
  }
  Serial.println();
  
  // 验证权限
  if (isAuthorized(rfid.uid.uidByte, rfid.uid.size)) {
    Serial.println("✓ 授权通过！");
    digitalWrite(LED_GREEN, HIGH);
    tone(BUZZER, 1000, 200);
    delay(1000);
    digitalWrite(LED_GREEN, LOW);
  } else {
    Serial.println("✗ 拒绝访问！");
    digitalWrite(LED_RED, HIGH);
    tone(BUZZER, 300, 500);
    delay(1000);
    digitalWrite(LED_RED, LOW);
  }
  
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

bool isAuthorized(byte *uid, byte size) {
  for (int i = 0; i < NUM_CARDS; i++) {
    bool match = true;
    for (byte j = 0; j < 4 && j < size; j++) {
      if (uid[j] != authorizedUID[i][j]) {
        match = false;
        break;
      }
    }
    if (match) return true;
  }
  return false;
}