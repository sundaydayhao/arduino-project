/*
 * 综合项目：RFID 门禁系统
 * 功能：
 *   1. RFID 刷卡验证身份
 *   2. 授权卡开门（舵机模拟门锁）
 *   3. LCD 显示状态和时间
 *   4. 蜂鸣器提示音
 *   5. 串口记录出入记录
 *   6. 管理员卡添加/删除授权
 * 
 * 需要安装库：MFRC522, LiquidCrystal_I2C
 * 
 * 硬件：
 *   RC522: SDA→D10, SCK→D13, MOSI→D11, MISO→D12, RST→D9
 *   舵机（门锁）→ D6
 *   LCD I2C → A4, A5
 *   蜂鸣器 → D7
 *   绿灯 → D4，红灯 → D5
 */

#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

MFRC522 rfid(10, 9);
Servo doorServo;
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int BUZZER    = 7;
const int LED_GREEN = 4;
const int LED_RED   = 5;

// 管理员卡 UID（修改为你的管理员卡）
byte adminUID[] = {0xAA, 0xBB, 0xCC, 0xDD};

// 授权卡列表（最多10张）
byte authorizedCards[10][4];
int cardCount = 0;

// 门锁状态
const int LOCK_ANGLE   = 0;    // 锁定角度
const int UNLOCK_ANGLE = 90;   // 解锁角度
const unsigned long OPEN_DURATION = 3000;  // 开门持续3秒

bool addMode = false;  // 添加卡模式

void setup() {
  SPI.begin();
  rfid.PCD_Init();
  doorServo.attach(6);
  doorServo.write(LOCK_ANGLE);
  
  pinMode(BUZZER, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  
  lcd.init();
  lcd.backlight();
  Serial.begin(9600);
  
  // 预设一张授权卡
  byte defaultCard[] = {0x12, 0x34, 0x56, 0x78};
  memcpy(authorizedCards[cardCount++], defaultCard, 4);
  
  lcd.print("RFID 门禁系统");
  lcd.setCursor(0, 1);
  lcd.print("请刷卡...");
  Serial.println("RFID 门禁系统启动");
  Serial.println("管理员卡: 进入添加模式");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }
  
  byte *uid = rfid.uid.uidByte;
  byte size = rfid.uid.size;
  
  // 打印 UID
  String uidStr = getUIDString(uid, size);
  Serial.print("刷卡: ");
  Serial.println(uidStr);
  
  // 检查是否为管理员卡
  if (matchUID(uid, adminUID)) {
    addMode = !addMode;
    lcd.clear();
    lcd.print(addMode ? "添加模式" : "正常模式");
    tone(BUZZER, 1500, 300);
    Serial.println(addMode ? "进入添加模式" : "退出添加模式");
  }
  else if (addMode) {
    // 添加新卡
    if (cardCount < 10) {
      memcpy(authorizedCards[cardCount++], uid, 4);
      lcd.clear();
      lcd.print("卡片已添加!");
      lcd.setCursor(0, 1);
      lcd.print(uidStr);
      tone(BUZZER, 2000, 200);
      delay(100);
      tone(BUZZER, 2000, 200);
      Serial.print("添加卡片: ");
      Serial.println(uidStr);
    }
  }
  else if (isAuthorized(uid)) {
    // 授权通过，开门
    accessGranted(uidStr);
  }
  else {
    // 拒绝访问
    accessDenied(uidStr);
  }
  
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  delay(1000);
}

void accessGranted(String uid) {
  Serial.print("授权通过: ");
  Serial.println(uid);
  
  lcd.clear();
  lcd.print("欢迎！门已开");
  lcd.setCursor(0, 1);
  lcd.print(uid);
  
  digitalWrite(LED_GREEN, HIGH);
  tone(BUZZER, 2000, 200);
  doorServo.write(UNLOCK_ANGLE);
  
  delay(OPEN_DURATION);
  
  doorServo.write(LOCK_ANGLE);
  digitalWrite(LED_GREEN, LOW);
  noTone(BUZZER);
  
  lcd.clear();
  lcd.print("RFID 门禁系统");
  lcd.setCursor(0, 1);
  lcd.print("请刷卡...");
}

void accessDenied(String uid) {
  Serial.print("拒绝访问: ");
  Serial.println(uid);
  
  lcd.clear();
  lcd.print("拒绝访问!");
  lcd.setCursor(0, 1);
  lcd.print(uid);
  
  digitalWrite(LED_RED, HIGH);
  for (int i = 0; i < 3; i++) {
    tone(BUZZER, 500, 200);
    delay(300);
  }
  noTone(BUZZER);
  delay(500);
  digitalWrite(LED_RED, LOW);
  
  lcd.clear();
  lcd.print("RFID 门禁系统");
  lcd.setCursor(0, 1);
  lcd.print("请刷卡...");
}

bool isAuthorized(byte *uid) {
  for (int i = 0; i < cardCount; i++) {
    if (matchUID(uid, authorizedCards[i])) return true;
  }
  return false;
}

bool matchUID(byte *uid1, byte *uid2) {
  for (int i = 0; i < 4; i++) {
    if (uid1[i] != uid2[i]) return false;
  }
  return true;
}

String getUIDString(byte *uid, byte size) {
  String s = "";
  for (byte i = 0; i < size; i++) {
    if (uid[i] < 0x10) s += "0";
    s += String(uid[i], HEX);
    if (i < size - 1) s += ":";
  }
  s.toUpperCase();
  return s;
}