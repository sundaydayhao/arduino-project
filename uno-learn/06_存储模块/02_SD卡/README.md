# SD 卡存储实验

## 简介
SD 卡模块让 Arduino 可以读写 SD 卡，适合数据记录、配置存储等应用。

## 硬件连接
```
SD 模块    Arduino UNO
CS   ───── D4
MOSI ───── D11
MISO ───── D12
SCK  ───── D13
VCC  ───── 5V
GND  ───── GND
```

## SD 卡格式要求
- 格式：FAT16 或 FAT32
- 容量：建议 32GB 以下
- 文件名：8.3 格式（最多8字符名+3字符扩展名）

## 实验文件
1. `SD卡读写.ino` - 文件读写、数据记录

## 常用函数
```cpp
SD.begin(csPin);           // 初始化
SD.open("file.txt");       // 打开文件（只读）
SD.open("file.txt", FILE_WRITE); // 打开文件（写入/追加）
file.print(data);          // 写入数据
file.read();               // 读取一个字节
file.available();          // 是否有数据可读
file.close();              // 关闭文件
SD.exists("file.txt");     // 文件是否存在
SD.remove("file.txt");     // 删除文件