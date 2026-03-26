"""
Matter 智能家居 - Linux MPU 端
Arduino UNO Q (Qualcomm)

双脑架构：
  MCU (Cortex-M33)  → Matter 协议栈，设备配对，状态控制
  Linux MPU (本文件) → 接收设备状态，自动化规则，REST API，Web 控制面板

Linux MPU 运行自动化逻辑（定时、联动、场景），
MCU 负责 Matter 协议通信和硬件控制。
"""

import datetime
import threading
import time
from arduino.app_utils import App, Bridge
from arduino.app_bricks.web_ui import WebUI

print("Matter 智能家居 MPU 端启动", flush=True)

web  = WebUI()
lock = threading.Lock()

# ===== 设备状态 =====
devices = {
    "light":       {"on": False, "brightness": 100, "color_temp": 4000},
    "fan":         {"on": False, "speed": 0},
    "thermostat":  {"mode": "off", "target_temp": 22.0, "current_temp": None},
    "sensor":      {"temperature": None, "humidity": None, "occupancy": False},
}

state = {
    "commissioned": False,
    "fabric_id":    None,
    "node_id":      None,
    "updated_at":   None,
    "automation_enabled": True,
}

automation_log = []


# ===== Bridge 回调 =====
def update_device_state(device, key, value):
    """
    MCU 设备状态变化时调用：
      Bridge.call("update_device_state", "light", "on", 1)
    """
    now = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    dev = str(device).strip()
    k   = str(key).strip()

    with lock:
        if dev in devices and k in devices[dev]:
            # 类型转换
            orig = devices[dev][k]
            if isinstance(orig, bool):
                devices[dev][k] = bool(int(float(value)))
            elif isinstance(orig, int):
                devices[dev][k] = int(float(value))
            elif isinstance(orig, float):
                devices[dev][k] = round(float(value), 2)
            else:
                devices[dev][k] = str(value)
        state["updated_at"] = now

    print(f"[{now}] 设备更新: {dev}.{k} = {value}", flush=True)
    _check_automation(dev, k, value)


def on_commissioned(fabric_id, node_id):
    """Matter 设备配对成功"""
    now = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    with lock:
        state["commissioned"] = True
        state["fabric_id"]    = str(fabric_id)
        state["node_id"]      = str(node_id)
        state["updated_at"]   = now
    print(f"[{now}] Matter 配对成功! Fabric:{fabric_id} Node:{node_id}", flush=True)


Bridge.provide("update_device_state", update_device_state)
Bridge.provide("on_commissioned",     on_commissioned)


# ===== 自动化规则 =====
def _check_automation(device, key, value):
    """检查并执行自动化规则"""
    with lock:
        if not state["automation_enabled"]:
            return
        temp = devices["sensor"]["temperature"]
        occ  = devices["sensor"]["occupancy"]

    # 规则1：有人时自动开灯
    if device == "sensor" and key == "occupancy":
        if bool(int(float(value))):
            _trigger_automation("有人检测", "light", "on", True)
        else:
            _trigger_automation("无人检测", "light", "on", False)

    # 规则2：温度过高时开风扇
    if device == "sensor" and key == "temperature" and temp is not None:
        if temp > 28:
            _trigger_automation(f"温度过高({temp}C)", "fan", "on", True)
        elif temp < 25:
            _trigger_automation(f"温度适宜({temp}C)", "fan", "on", False)


def _trigger_automation(rule, device, key, value):
    now = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    print(f"[{now}] 自动化: {rule} -> {device}.{key}={value}", flush=True)
    automation_log.append({"rule": rule, "device": device,
                           "key": key, "value": value, "time": now})
    if len(automation_log) > 50:
        automation_log.pop(0)
    # 通知 MCU 执行动作
    Bridge.call("set_device", device, key, int(value) if isinstance(value, bool) else value)


# ===== REST API =====
def api_state(_req=None):
    with lock:
        return {**state, "devices": dict(devices)}


def api_automation(_req=None):
    return {"log": list(automation_log[-20:])}


def api_set_device(req):
    """POST /api/set_device {"device":"light","key":"on","value":1}"""
    device = req.get("device", "")
    key    = req.get("key", "")
    value  = req.get("value")
    if device and key and value is not None:
        Bridge.call("set_device", device, key, value)
        return {"ok": True}
    return {"ok": False, "error": "missing params"}


web.expose_api("GET",  "/api/state",      api_state)
web.expose_api("GET",  "/api/automation", api_automation)
web.expose_api("POST", "/api/set_device", api_set_device)

App.run()