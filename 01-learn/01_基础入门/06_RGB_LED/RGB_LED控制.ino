/*
 * 实验名称：RGB LED 控制
 * 实验目的：学习混色原理，用 PWM 控制 RGB LED 显示各种颜色
 * 
 * 硬件连接（共阴极 RGB LED）：
 *   R 引脚 → 220Ω → Arduino D9
 *   G 引脚 → 220Ω → Arduino D10
 *   B 引脚 → 220Ω → Arduino D11
 *   公共阴极 → GND
 * 
 * 注意：共阳极 RGB LED 需要将逻辑反转（255-value）
 * 
 * 知识点：
 *   - RGB 三原色混色原理
 *   - PWM 控制颜色
 *   - HSV 颜色空间转换
 */

const int RED_PIN   = 9;
const int GREEN_PIN = 10;
const int BLUE_PIN  = 11;

// 是否为共阳极（共阳极需要反转逻辑）
const bool COMMON_ANODE = false;

void setup() {
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  Serial.begin(9600);
  Serial.println("RGB LED 控制实验");
}

void loop() {
  // 显示基本颜色
  Serial.println("红色");
  setColor(255, 0, 0);
  delay(1000);
  
  Serial.println("绿色");
  setColor(0, 255, 0);
  delay(1000);
  
  Serial.println("蓝色");
  setColor(0, 0, 255);
  delay(1000);
  
  Serial.println("黄色");
  setColor(255, 255, 0);
  delay(1000);
  
  Serial.println("青色");
  setColor(0, 255, 255);
  delay(1000);
  
  Serial.println("品红");
  setColor(255, 0, 255);
  delay(1000);
  
  Serial.println("白色");
  setColor(255, 255, 255);
  delay(1000);
  
  Serial.println("关闭");
  setColor(0, 0, 0);
  delay(500);
  
  // 彩虹渐变效果（HSV 色轮）
  Serial.println("彩虹渐变...");
  for (int hue = 0; hue < 360; hue++) {
    int r, g, b;
    hsvToRgb(hue, 255, 255, r, g, b);
    setColor(r, g, b);
    delay(10);
  }
}

// 设置 RGB 颜色
void setColor(int red, int green, int blue) {
  if (COMMON_ANODE) {
    red   = 255 - red;
    green = 255 - green;
    blue  = 255 - blue;
  }
  analogWrite(RED_PIN, red);
  analogWrite(GREEN_PIN, green);
  analogWrite(BLUE_PIN, blue);
}

// HSV 转 RGB（H: 0~359, S: 0~255, V: 0~255）
void hsvToRgb(int h, int s, int v, int &r, int &g, int &b) {
  if (s == 0) {
    r = g = b = v;
    return;
  }
  
  int region = h / 60;
  int remainder = (h - (region * 60)) * 4;
  
  int p = (v * (255 - s)) >> 8;
  int q = (v * (255 - ((s * remainder) >> 8))) >> 8;
  int t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;
  
  switch (region) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
  }
}