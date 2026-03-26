/*
 * 实验名称：串口控制 LED
 * 实验目的：通过串口命令控制 LED，学习串口字符串解析
 * 
 * 硬件连接：
 *   LED1 → D8，LED2 → D9，LED3 → D10
 *   所有 LED 通过 220Ω 电阻接 GND
 * 
 * 命令格式：
 *   ON 1    - 点亮 LED1
 *   OFF 2   - 熄灭 LED2
 *   ALL ON  - 全部点亮
 *   ALL OFF - 全部熄灭
 *   STATUS  - 查看状态
 */

const int LED_PINS[] = {8, 9, 10};
const int NUM_LEDS = 3;
bool ledStates[NUM_LEDS] = {false, false, false};

String inputString = "";
bool stringComplete = false;

void setup() {
  for (int i = 0; i < NUM_LEDS; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }
  
  Serial.begin(9600);
  inputString.reserve(50);
  
  Serial.println("=== 串口控制 LED ===");
  Serial.println("命令: ON 1/2/3 | OFF 1/2/3 | ALL ON | ALL OFF | STATUS");
}

void loop() {
  if (stringComplete) {
    inputString.trim();
    inputString.toUpperCase();
    processCommand(inputString);
    inputString = "";
    stringComplete = false;
  }
}

// 串口接收中断
void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n' || inChar == '\r') {
      if (inputString.length() > 0) {
        stringComplete = true;
      }
    } else {
      inputString += inChar;
    }
  }
}

void processCommand(String cmd) {
  Serial.print("执行命令: ");
  Serial.println(cmd);
  
  if (cmd == "ALL ON") {
    for (int i = 0; i < NUM_LEDS; i++) {
      setLED(i, true);
    }
    Serial.println("所有 LED 已点亮");
  }
  else if (cmd == "ALL OFF") {
    for (int i = 0; i < NUM_LEDS; i++) {
      setLED(i, false);
    }
    Serial.println("所有 LED 已熄灭");
  }
  else if (cmd == "STATUS") {
    printStatus();
  }
  else if (cmd.startsWith("ON ")) {
    int ledNum = cmd.substring(3).toInt() - 1;
    if (ledNum >= 0 && ledNum < NUM_LEDS) {
      setLED(ledNum, true);
      Serial.print("LED");
      Serial.print(ledNum + 1);
      Serial.println(" 已点亮");
    } else {
      Serial.println("错误：LED 编号无效（1~3）");
    }
  }
  else if (cmd.startsWith("OFF ")) {
    int ledNum = cmd.substring(4).toInt() - 1;
    if (ledNum >= 0 && ledNum < NUM_LEDS) {
      setLED(ledNum, false);
      Serial.print("LED");
      Serial.print(ledNum + 1);
      Serial.println(" 已熄灭");
    } else {
      Serial.println("错误：LED 编号无效（1~3）");
    }
  }
  else {
    Serial.println("未知命令，输入 STATUS 查看帮助");
  }
}

void setLED(int index, bool state) {
  ledStates[index] = state;
  digitalWrite(LED_PINS[index], state ? HIGH : LOW);
}

void printStatus() {
  Serial.println("--- LED 状态 ---");
  for (int i = 0; i < NUM_LEDS; i++) {
    Serial.print("LED");
    Serial.print(i + 1);
    Serial.print(" (D");
    Serial.print(LED_PINS[i]);
    Serial.print("): ");
    Serial.println(ledStates[i] ? "亮" : "灭");
  }
}