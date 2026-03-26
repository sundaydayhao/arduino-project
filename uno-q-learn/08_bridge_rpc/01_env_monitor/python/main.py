"""
环境监测站 - Linux MPU 端
Arduino UNO Q (Qualcomm)

接收 MCU 通过 Bridge 发送的传感器数据，
提供 REST API，供 Web 仪表盘实时展示。

功能：
  - 接收温湿度/气压/光照/空气质量数据
  - 计算统计值（最大/最小/平均）
  - 历史数据存储（最近 100 条）
  - 告警检测（超阈值推送）
  - REST API 供前端轮询
"""

import datetime
import threading
import collections
from arduino.app_utils import App, Bridge
from arduino.app_bricks.web_ui import WebUI

print("环境监测站 Python 端启动", flush=True)

web  = WebUI()
lock = threading.Lock()

# ===== 数据存储 =====
HISTORY_LEN = 100

state = {
    "temperature": None,
    "humidity":    None,
    "pressure":    None,
    "altitude":    None,
    "light":       None,
    "aqi":         None,
    "updated_at":  None,
    "sample_count": 0,
}

# 历史数据（用于图表）
history = {
    "temperature": collections.deque(maxlen=HISTORY_LEN),
    "humidity":    collections.deque(maxlen=HISTORY_LEN),
    "pressure":    collections.deque(maxlen=HISTORY_LEN),
    "light":       collections.deque(maxlen=HISTORY_LEN),
    "aqi":         collections.deque(maxlen=HISTORY_LEN),
    "timestamps":  collections.deque(maxlen=HISTORY_LEN),
}

# 告警阈值
THRESHOLDS = {
    "temperature_max": 35.0,
    "temperature_min": 0.0,
    "humidity_max":    80.0,
    "aqi_max":         150,
}

alerts = []

# ===== Bridge 回调：接收 MCU 数据 =====
def update_env(temperature, humidity, pressure, altitude, light, aqi):
    """MCU 通过 Bridge.call('update_env', ...) 调用此函数"""
    now = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

    with lock:
        state["temperature"] = float(temperature)
        state["humidity"]    = float(humidity)
        state["pressure"]    = float(pressure)
        state["altitude"]    = float(altitude)
        state["light"]       = float(light)
        state["aqi"]         = int(aqi)
        state["updated_at"]  = now
        state["sample_count"] += 1

        # 追加历史
        history["temperature"].append(float(temperature))
        history["humidity"].append(float(humidity))
        history["pressure"].append(float(pressure))
        history["light"].append(float(light))
        history["aqi"].append(int(aqi))
        history["timestamps"].append(now)

        # 告警检测
        check_alerts(float(temperature), float(humidity), int(aqi))

    print(f"[{now}] T={temperature:.1f}°C H={humidity:.0f}% P={pressure:.1f}hPa "
          f"L={light:.0f}lux AQI={aqi}", flush=True)

def check_alerts(temp, humi, aqi):
    """检测超阈值告警"""
    global alerts
    new_alerts = []
    now = datetime.datetime.now().strftime("%H:%M:%S")

    if temp > THRESHOLDS["temperature_max"]:
        new_alerts.append({"level": "warning", "msg": f"温度过高: {temp:.1f}°C", "time": now})
    if temp < THRESHOLDS["temperature_min"]:
        new_alerts.append({"level": "warning", "msg": f"温度过低: {temp:.1f}°C", "time": now})
    if humi > THRESHOLDS["humidity_max"]:
        new_alerts.append({"level": "warning", "msg": f"湿度过高: {humi:.0f}%", "time": now})
    if aqi > THRESHOLDS["aqi_max"]:
        new_alerts.append({"level": "danger", "msg": f"空气质量差: AQI={aqi}", "time": now})

    if new_alerts:
        alerts = (new_alerts + alerts)[:20]  # 保留最近 20 条告警

# 注册 Bridge 函数
Bridge.provide("update_env", update_env)

# ===== REST API =====
def api_state(_req=None):
    """当前状态 API"""
    with lock:
        return {
            "now_utc":     datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
            "temperature": state["temperature"],
            "humidity":    state["humidity"],
            "pressure":    state["pressure"],
            "altitude":    state["altitude"],
            "light":       state["light"],
            "aqi":         state["aqi"],
            "updated_at":  state["updated_at"],
            "sample_count": state["sample_count"],
            "aqi_level":   _aqi_level(state["aqi"]),
        }

def api_history(_req=None):
    """历史数据 API（用于图表）"""
    with lock:
        return {
            "timestamps":  list(history["timestamps"]),
            "temperature": list(history["temperature"]),
            "humidity":    list(history["humidity"]),
            "pressure":    list(history["pressure"]),
            "light":       list(history["light"]),
            "aqi":         list(history["aqi"]),
        }

def api_alerts(_req=None):
    """告警列表 API"""
    with lock:
        return {"alerts": alerts}

def _aqi_level(aqi):
    if aqi is None: return "unknown"
    if aqi <= 50:   return "优"
    if aqi <= 100:  return "良"
    if aqi <= 150:  return "轻度污染"
    if aqi <= 200:  return "中度污染"
    return "重度污染"

# 注册 API 端点
web.expose_api("GET", "/api/state",   api_state)
web.expose_api("GET", "/api/history", api_history)
web.expose_api("GET", "/api/alerts",  api_alerts)

App.run()