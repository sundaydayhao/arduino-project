# RFID RC522 实验

## 模块简介
RC522 是一款基于 SPI 接口的 RFID 读写模块，工作频率 13.56MHz，兼容 Mifare 卡。

## 硬件连接
```
RC522    Arduino UNO
SDA  ─── D10（SS）
SCK  ─── D13
MOSI ─── D11
MISO ─── D12
RST  ─── D9
3.3V ─── 3.3V（注意！不能接5V）
GND  ─── GND
```

## 需要安装的库
- **MFRC522**：`工具 → 管理库 → 搜索 MFRC522`

## 实验文件
1. `RFID读卡.ino` - 读取卡片UID，门禁控制

## 思考题
1. 如何读写 Mifare 卡的数据块？
2. 如何实现考勤打卡系统？
3. RFID 和 NFC 有什么区别？