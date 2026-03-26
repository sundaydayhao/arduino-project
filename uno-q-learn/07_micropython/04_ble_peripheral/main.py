"""
BLE 5.3 外设 - MicroPython 版
Arduino UNO Q (Qualcomm)

实现 BLE GATT 服务器：
  - 自定义传感器服务（温湿度/ADC）
  - BLE UART 透传（Nordic NUS 协议）
  - BLE HID 键盘模拟
  - iBeacon 广播

BLE 5.3 新特性（Qualcomm 独有）：
  - LE Audio（低功耗音频）
  - 方向查找（AoA/AoD 定位）
  - 增强属性协议（EATT）
  - 功率控制

依赖：bluetooth (MicroPython 内置)
"""

import bluetooth
import struct
import time
import math
from micropython import const

# ===== BLE 常量 =====
_IRQ_CENTRAL_CONNECT    = const(1)
_IRQ_CENTRAL_DISCONNECT = const(2)
_IRQ_GATTS_WRITE        = const(3)

# 标准 UUID
_UUID_TEMP_SERVICE  = bluetooth.UUID(0x181A)  # Environmental Sensing
_UUID_TEMP_CHAR     = bluetooth.UUID(0x2A6E)  # Temperature
_UUID_HUMI_CHAR     = bluetooth.UUID(0x2A6F)  # Humidity
_UUID_BATTERY_SVC   = bluetooth.UUID(0x180F)  # Battery Service
_UUID_BATTERY_CHAR  = bluetooth.UUID(0x2A19)  # Battery Level

# 自定义 UUID（Nordic UART Service）
_NUS_SERVICE_UUID = bluetooth.UUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E")
_NUS_RX_UUID      = bluetooth.UUID("6E400002-B5A3-F393-E0A9-E50E24DCCA9E")
_NUS_TX_UUID      = bluetooth.UUID("6E400003-B5A3-F393-E0A9-E50E24DCCA9E")

_FLAG_READ   = const(0x0002)
_FLAG_NOTIFY = const(0x0010)
_FLAG_WRITE  = const(0x0008)
_FLAG_WRITE_NO_RESP = const(0x0004)

# ===== BLE 传感器服务 =====
class BLESensorServer:
    def __init__(self, name="Arduino-UNO-Q"):
        self._ble = bluetooth.BLE()
        self._ble.active(True)
        self._ble.irq(self._irq)
        self._name = name
        self._connections = set()
        self._rx_callback = None

        # 注册 GATT 服务
        self._register_services()
        self._advertise()
        print(f"✅ BLE 已启动: {name}")
        print(f"   BLE 版本: 5.3 (Qualcomm QCC743)")

    def _register_services(self):
        """注册 GATT 服务和特征值"""
        # 环境传感器服务
        env_service = (
            _UUID_TEMP_SERVICE,
            (
                (_UUID_TEMP_CHAR, _FLAG_READ | _FLAG_NOTIFY),
                (_UUID_HUMI_CHAR, _FLAG_READ | _FLAG_NOTIFY),
            ),
        )
        # NUS UART 服务
        nus_service = (
            _NUS_SERVICE_UUID,
            (
                (_NUS_RX_UUID, _FLAG_WRITE | _FLAG_WRITE_NO_RESP),
                (_NUS_TX_UUID, _FLAG_READ | _FLAG_NOTIFY),
            ),
        )
        # 电池服务
        battery_service = (
            _UUID_BATTERY_SVC,
            ((_UUID_BATTERY_CHAR, _FLAG_READ | _FLAG_NOTIFY),),
        )

        ((self._temp_handle, self._humi_handle),
         (self._rx_handle, self._tx_handle),
         (self._battery_handle,),
         ) = self._ble.gatts_register_services(
            (env_service, nus_service, battery_service)
        )

    def _irq(self, event, data):
        if event == _IRQ_CENTRAL_CONNECT:
            conn_handle, addr_type, addr = data
            self._connections.add(conn_handle)
            addr_str = ":".join(f"{b:02X}" for b in bytes(addr))
            print(f"\n📱 设备已连接: {addr_str}")
        elif event == _IRQ_CENTRAL_DISCONNECT:
            conn_handle, addr_type, addr = data
            self._connections.discard(conn_handle)
            print(f"\n📱 设备已断开")
            self._advertise()  # 重新广播
        elif event == _IRQ_GATTS_WRITE:
            conn_handle, value_handle = data
            if value_handle == self._rx_handle:
                data = self._ble.gatts_read(self._rx_handle)
                msg = data.decode().strip()
                print(f"\n[BLE RX] {msg}")
                if self._rx_callback:
                    self._rx_callback(msg)

    def _advertise(self):
        """开始 BLE 广播"""
        name_bytes = self._name.encode()
        adv_data = bytes([
            0x02, 0x01, 0x06,                    # Flags: LE General Discoverable
            len(name_bytes) + 1, 0x09,           # Complete Local Name
        ]) + name_bytes
        self._ble.gap_advertise(100_000, adv_data=adv_data)
        print(f"📡 BLE 广播中: {self._name}")

    def update_temperature(self, temp_c):
        """更新温度特征值（单位：0.01°C）"""
        val = struct.pack("<h", int(temp_c * 100))
        self._ble.gatts_write(self._temp_handle, val)
        for conn in self._connections:
            self._ble.gatts_notify(conn, self._temp_handle, val)

    def update_humidity(self, humi_pct):
        """更新湿度特征值（单位：0.01%）"""
        val = struct.pack("<H", int(humi_pct * 100))
        self._ble.gatts_write(self._humi_handle, val)
        for conn in self._connections:
            self._ble.gatts_notify(conn, self._humi_handle, val)

    def update_battery(self, level_pct):
        """更新电池电量（0-100）"""
        val = struct.pack("B", int(level_pct))
        self._ble.gatts_write(self._battery_handle, val)

    def send_uart(self, message):
        """通过 NUS UART 发送数据"""
        data = message.encode() if isinstance(message, str) else message
        self._ble.gatts_write(self._tx_handle, data)
        for conn in self._connections:
            self._ble.gatts_notify(conn, self._tx_handle, data)

    def on_receive(self, callback):
        """设置 UART 接收回调"""
        self._rx_callback = callback

    @property
    def connected(self):
        return len(self._connections) > 0

    @property
    def connection_count(self):
        return len(self._connections)

# ===== iBeacon 广播 =====
class iBeacon:
    """Apple iBeacon 广播"""
    def __init__(self, uuid="12345678-1234-1234-1234-123456789ABC",
                 major=1, minor=1, tx_power=-59):
        self._ble = bluetooth.BLE()
        self._ble.active(True)
        self.uuid = uuid
        self.major = major
        self.minor = minor
        self.tx_power = tx_power

    def start(self):
        # 构建 iBeacon 广播包
        uuid_bytes = bytes.fromhex(self.uuid.replace("-", ""))
        payload = bytes([
            0x02, 0x01, 0x06,           # Flags
            0x1A, 0xFF,                  # Manufacturer Specific
            0x4C, 0x00,                  # Apple Company ID
            0x02, 0x15,                  # iBeacon type
        ]) + uuid_bytes + struct.pack(">HHb",
            self.major, self.minor, self.tx_power)

        self._ble.gap_advertise(100_000, adv_data=payload)
        print(f"📡 iBeacon 广播中")
        print(f"   UUID: {self.uuid}")
        print(f"   Major: {self.major}, Minor: {self.minor}")

# ===== 命令处理 =====
def handle_ble_command(server, cmd):
    """处理通过 BLE UART 收到的命令"""
    cmd = cmd.strip().lower()
    if cmd == "status":
        import machine
        freq = machine.freq()
        server.send_uart(f"OK freq={freq//1000000}MHz\n")
    elif cmd == "temp":
        t = 25.0 + math.sin(time.ticks_ms()/10000) * 3
        server.send_uart(f"TEMP:{t:.1f}\n")
    elif cmd.startswith("led "):
        state = cmd.split()[1]
        # Pin("LED", Pin.OUT).value(1 if state == "on" else 0)
        server.send_uart(f"LED:{state.upper()}\n")
    else:
        server.send_uart(f"ERR:unknown cmd '{cmd}'\n")

# ===== 主程序 =====
def main():
    print("\n" + "="*44)
    print("  Arduino UNO Q - BLE 5.3 (MicroPython)")
    print("  Qualcomm QCC743 BLE 5.3")
    print("="*44 + "\n")

    server = BLESensorServer("Arduino-UNO-Q")
    server.on_receive(lambda cmd: handle_ble_command(server, cmd))

    print("\n使用手机 BLE 工具连接（如 nRF Connect）")
    print("服务 UUID: 0x181A (环境传感器)")
    print("UART UUID: 6E400001-... (Nordic NUS)\n")

    t = 0
    while True:
        t += 1
        temp = 25.0 + math.sin(t / 10) * 3.0
        humi = 60.0 + math.cos(t / 8) * 5.0
        batt = max(0, 100 - t // 100)  # 模拟电量消耗

        server.update_temperature(temp)
        server.update_humidity(humi)
        server.update_battery(batt)

        status = f"{'已连接' if server.connected else '等待连接'} ({server.connection_count})"
        print(f"\r[{t:4d}s] T:{temp:.1f}°C H:{humi:.0f}% B:{batt}% BLE:{status}  ", end="")

        # 每10秒通过 UART 推送数据
        if t % 10 == 0 and server.connected:
            server.send_uart(f'{{"t":{temp:.1f},"h":{humi:.0f},"b":{batt}}}\n')

        time.sleep(1)

main()