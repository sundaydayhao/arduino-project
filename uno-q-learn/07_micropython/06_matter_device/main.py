"""
Matter 智能家居设备 - MicroPython 版
Arduino UNO Q (Qualcomm)

Qualcomm QCC743 原生支持 Matter 1.x，
MicroPython 版让配置更简单！

Matter 设备类型：
  - 温湿度传感器 / 开关插座 / 调光灯 / 门锁

配网后支持：
  ✅ Apple HomeKit / Google Home / Amazon Alexa / Home Assistant

依赖：matter (Qualcomm MicroPython 内置)
"""

import matter
import time
import math
from machine import Pin, PWM

# ===== 硬件配置 =====
LED_PIN    = Pin("LED", Pin.OUT)
DIMMER_PIN = PWM(Pin(9), freq=1000)
BUTTON_PIN = Pin(2, Pin.IN, Pin.PULL_UP)

def read_temperature():
    return 25.0 + math.sin(time.ticks_ms() / 10000) * 3.0

def read_humidity():
    return 60.0 + math.cos(time.ticks_ms() / 8000) * 5.0

# ===== Matter 设备 =====
class SmartHomeDevice:
    def __init__(self):
        matter.begin(
            device_name="Arduino UNO Q",
            vendor_name="Qualcomm",
            vendor_id=0xFFF1,
            product_id=0x8001,
        )
        self.temp_ep   = matter.TemperatureSensor(endpoint=1)
        self.humi_ep   = matter.HumiditySensor(endpoint=2)
        self.light_ep  = matter.OnOffLight(endpoint=3)
        self.dimmer_ep = matter.DimmableLight(endpoint=4)
        self.outlet_ep = matter.OnOffPlugInUnit(endpoint=5)

        self.light_ep.on_change(self._on_light)
        self.dimmer_ep.on_change(self._on_dimmer)
        self.outlet_ep.on_change(self._on_outlet)
        matter.on_commissioned(self._on_commissioned)

        print("✅ Matter 设备已初始化")
        print("   EP1:温度  EP2:湿度  EP3:开关灯  EP4:调光灯  EP5:插座")

    def _on_light(self, state):
        print(f"[Matter] 灯光: {'开' if state else '关'}")
        LED_PIN.value(1 if state else 0)

    def _on_dimmer(self, level):
        print(f"[Matter] 亮度: {level * 100 // 254}%")
        DIMMER_PIN.duty_u16(level * 257)

    def _on_outlet(self, state):
        print(f"[Matter] 插座: {'开' if state else '关'}")

    def _on_commissioned(self):
        print("\n✅ Matter 配网成功! 可用 Apple Home / Google Home 控制")
        for _ in range(5):
            LED_PIN.on();  time.sleep(0.1)
            LED_PIN.off(); time.sleep(0.1)

    def update_sensors(self):
        temp = read_temperature()
        humi = read_humidity()
        self.temp_ep.set_temperature(temp)
        self.humi_ep.set_humidity(humi)
        return temp, humi

    def show_pairing_info(self):
        if matter.is_commissioned():
            print("✅ 已配网")
        else:
            print("\n⚠️ 未配网，配网步骤:")
            print("  1. 打开 Apple Home / Google Home")
            print("  2. 添加配件 → 扫描二维码")
            print(f"  配网码: {matter.manual_pairing_code()}")
            print(f"  二维码: {matter.qr_code_payload()}")


# ===== 门锁设备（独立示例）=====
class SmartLock:
    """Matter 门锁设备"""
    def __init__(self):
        matter.begin(device_name="Smart Lock", vendor_id=0xFFF1, product_id=0x8002)
        self.lock_ep = matter.DoorLock(endpoint=1)
        self.lock_ep.on_change(self._on_lock_change)
        self._locked = True
        print("✅ Matter 门锁已初始化")

    def _on_lock_change(self, locked):
        self._locked = locked
        state = "🔒 已锁定" if locked else "🔓 已解锁"
        print(f"[Matter 门锁] {state}")
        # 控制电磁锁
        # lock_relay.value(1 if locked else 0)

    def lock(self):
        self.lock_ep.set_locked(True)

    def unlock(self):
        self.lock_ep.set_locked(False)


# ===== 窗帘控制器（独立示例）=====
class SmartCurtain:
    """Matter 窗帘控制器"""
    def __init__(self):
        matter.begin(device_name="Smart Curtain", vendor_id=0xFFF1, product_id=0x8003)
        self.cover_ep = matter.WindowCovering(endpoint=1)
        self.cover_ep.on_change(self._on_position_change)
        print("✅ Matter 窗帘控制器已初始化")

    def _on_position_change(self, position):
        """position: 0=全关, 100=全开"""
        print(f"[Matter 窗帘] 位置: {position}%")
        # 控制步进电机到指定位置
        # stepper.move_to_position(position)


# ===== 主程序 =====
def main():
    print("\n" + "="*44)
    print("  Arduino UNO Q - Matter 智能家居 (MicroPython)")
    print("  Qualcomm QCC743 原生 Matter 1.x 支持")
    print("="*44 + "\n")

    device = SmartHomeDevice()
    device.show_pairing_info()

    print("\n命令: 'pair' | 'status' | 'light on/off' | 'dim <0-100>'")

    last_sensor_update = 0
    last_btn = True

    while True:
        matter.loop()

        # 每 30 秒更新传感器
        now = time.ticks_ms()
        if time.ticks_diff(now, last_sensor_update) > 30000:
            last_sensor_update = now
            temp, humi = device.update_sensors()
            print(f"[传感器] 温度={temp:.1f}°C 湿度={humi:.0f}%")

        # 物理按钮切换灯光
        btn = BUTTON_PIN.value()
        if btn == 0 and last_btn == 1:
            new_state = not device.light_ep.get_on_off()
            device.light_ep.set_on_off(new_state)
            print(f"[按钮] 灯光: {'开' if new_state else '关'}")
            time.sleep(0.05)
        last_btn = btn

        # 串口命令
        import sys
        import select
        if select.select([sys.stdin], [], [], 0)[0]:
            cmd = sys.stdin.readline().strip()
            if cmd == "pair":
                device.show_pairing_info()
            elif cmd == "status":
                temp, humi = device.update_sensors()
                print(f"温度={temp:.1f}°C 湿度={humi:.0f}%")
                print(f"配网状态: {'已配网' if matter.is_commissioned() else '未配网'}")
            elif cmd == "light on":
                device.light_ep.set_on_off(True)
            elif cmd == "light off":
                device.light_ep.set_on_off(False)
            elif cmd.startswith("dim "):
                pct = int(cmd.split()[1])
                level = pct * 254 // 100
                device.dimmer_ep.set_level(level)
                print(f"亮度设置为 {pct}%")

        time.sleep(0.01)

main()