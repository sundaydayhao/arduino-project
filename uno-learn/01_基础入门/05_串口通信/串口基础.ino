/*
 * 实验名称：串口通信基础
 * 实验目的：学习 Arduino 与电脑之间的串口通信
 * 
 * 硬件连接：
 *   仅需 USB 连接 Arduino 到电脑
 * 
 * 知识点：
 *   - UART 串口通信协议
 *   - Serial.begin() 初始化
 *   - Serial.print() / println() 发送数据
 *   - Serial.available() / read() 接收数据
 */

int counter = 0;

void setup() {
  // 初始化串口，波特率 9600
  Serial.begin(9600);
  
  // 等待串口连接（对 Leonardo/Micro 有用）
  while (!Serial) {
    ;
  }
  
  Serial.println("=== Arduino 串口通信实验 ===");
  Serial.println("在串口监视器输入命令：");
  Serial.println("  'h' - 打印帮助信息");
  Serial.println("  'r' - 重置计数器");
  Serial.println("  其他字符 - 回显");
}

void loop() {
  // 每秒发送一次计数
  Serial.print("计数: ");
  Serial.println(counter++);
  
  // 检查是否有串口数据
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    
    switch (cmd) {
      case 'h':
        printHelp();
        break;
      case 'r':
        counter = 0;
        Serial.println("计数器已重置！");
        break;
      case '\n':
      case '\r':
        // 忽略换行符
        break;
      default:
        Serial.print("收到字符: '");
        Serial.print(cmd);
        Serial.print("' (ASCII: ");
        Serial.print((int)cmd);
        Serial.println(")");
    }
  }
  
  delay(1000);
}

void printHelp() {
  Serial.println("--- 帮助信息 ---");
  Serial.println("h: 显示帮助");
  Serial.println("r: 重置计数器");
  Serial.println("其他: 回显字符");
  Serial.println("----------------");
}