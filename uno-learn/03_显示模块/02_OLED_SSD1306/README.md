# OLED SSD1306 显示屏实验

## 模块简介
SSD1306 是一款 0.96 英寸 OLED 显示屏，分辨率 128×64，自发光，对比度高，功耗低。

## 规格参数
| 参数 | 值 |
|------|-----|
| 尺寸 | 0.96 英寸 |
| 分辨率 | 128 × 64 像素 |
| 颜色 | 单色（白色/蓝色/黄蓝双色） |
| 接口 | I2C 或 SPI |
| 工作电压 | 3.3V ~ 5V |
| I2C 地址 | 0x3C 或 0x3D |

## 硬件连接（I2C）

```
OLED           Arduino UNO
VCC ─────────── 3.3V 或 5V
GND ─────────── GND
SDA ─────────── A4
SCL ─────────── A5
```

## 需要安装的库
1. **Adafruit SSD1306**
2. **Adafruit GFX Library**

## 坐标系统

```
(0,0) ──────────────────→ X (127)
  │
  │    128 × 64 像素
  │
  ↓
Y (63)
```

## 常用函数

```cpp
display.clearDisplay();              // 清屏
display.display();                   // 刷新显示（必须调用！）
display.setTextSize(n);              // 文字大小（1~8）
display.setTextColor(SSD1306_WHITE); // 文字颜色
display.setCursor(x, y);             // 设置光标
display.print("text");               // 显示文字
display.drawPixel(x, y, color);      // 画点
display.drawLine(x0,y0,x1,y1,color);// 画线
display.drawRect(x,y,w,h,color);     // 画矩形
display.fillRect(x,y,w,h,color);     // 填充矩形
display.drawCircle(x,y,r,color);     // 画圆
display.fillCircle(x,y,r,color);     // 填充圆
display.drawBitmap(x,y,bmp,w,h,color); // 显示位图
```

## 实验文件
1. `OLED基础显示.ino` - 文字、图形、动画演示

## 思考题
1. OLED 和 LCD 有什么区别？各有什么优缺点？
2. 如何在 OLED 上显示图片？（提示：转换为位图数组）
3. 如何实现 OLED 屏保功能（防止烧屏）？