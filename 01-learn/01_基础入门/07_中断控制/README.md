# 中断控制实验

## 实验简介
学习 Arduino 的中断机制，包括外部中断和定时器中断。

## 什么是中断？

中断是一种让 CPU 暂停当前任务、立即处理紧急事件的机制。

```
正常执行: A → B → C → D → E
发生中断:         ↓
                 ISR（中断服务函数）
                 ↓
继续执行:              C → D → E
```

## Arduino UNO 中断资源

| 中断类型 | 引脚 | 触发方式 |
|----------|------|----------|
| 外部中断0 | D2 | LOW/CHANGE/RISING/FALLING |
| 外部中断1 | D3 | LOW/CHANGE/RISING/FALLING |
| 定时器0 | 内部 | 用于 millis()/delay() |
| 定时器1 | 内部 | 16位，可自定义 |
| 定时器2 | 内部 | 8位，用于 tone() |

## 触发方式说明

| 触发方式 | 说明 |
|----------|------|
| LOW | 引脚为低电平时持续触发 |
| CHANGE | 电平发生变化时触发 |
| RISING | 上升沿（LOW→HIGH）触发 |
| FALLING | 下降沿（HIGH→LOW）触发 |

## 实验文件
1. `外部中断.ino` - 按键触发外部中断
2. `定时器中断.ino` - Timer1 定时器中断（需要 TimerOne 库）

## 需要安装的库
- **TimerOne**：`工具 → 管理库 → 搜索 TimerOne`

## 中断编程注意事项

```cpp
// ✅ 正确：中断中修改的变量必须声明为 volatile
volatile bool flag = false;

// ✅ 正确：ISR 要尽量短小
void myISR() {
  flag = true;  // 只设置标志，在 loop() 中处理
}

// ❌ 错误：ISR 中不能使用 delay()
void badISR() {
  delay(100);  // 这会导致问题！
}

// ❌ 错误：ISR 中不能使用 Serial（可能导致死锁）
void badISR2() {
  Serial.println("中断");  // 不安全！
}
```

## 思考题
1. 为什么中断中修改的变量要加 `volatile`？
2. 中断和轮询各有什么优缺点？
3. 如果两个中断同时发生，Arduino 如何处理？