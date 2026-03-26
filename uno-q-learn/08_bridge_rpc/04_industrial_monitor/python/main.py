"""
工业多路传感器监控 - Linux MPU 端
Arduino UNO Q (Qualcomm)

Linux MPU 负责：
  - 接收 MCU 原始数据
  - 振动 FFT 频谱分析
  - 统计计算（均值/方差/峰值）
  - 多级告警判断
  - 数据持久化（CSV）
  - REST API 供 Web 仪表盘
"""

import datetime
import threading
import collections
import math
import csv
import os
from arduino.app_utils import App, Bridge
from arduino.app_bricks.web_ui import WebUI

print("工业监控 Python 端启动", flush=True)

web  = WebUI()
lock = threading.Lock()

# ===== 数据存储 =====
HISTORY = 200
state = {
    "current": 0.0, "vibration": 0.0, "temperature": 0.0,
    "pressure": 0.0, "rpm": 0.0, "power": 0.0,
    "dio": 0, "uptime_ms": 0, "updated_at": None,
    "sample_count": 0,
}

history = {k: collections.deque(maxlen=HISTORY)
           for k in ["current","vibration","temperature","pressure","rpm","power","timestamps"]}

# 振动缓冲区（用于 FFT）
VIB_FFT_SIZE = 64
vib_buffer = collections.deque(maxlen=VIB_FFT_SIZE)

# 告警
THRESHOLDS = {
    "current_max":     25.0,   # A
    "vibration_max":   2.0,    # g
    "temperature_max": 80.0,   # °C
    "pressure_max":    8.0,    # bar
    "rpm_max":         3000.0,
}
alerts = collections.deque(maxlen=50)

# CSV 日志
LOG_FILE = "/tmp/industrial_log.csv"
if not os.path.exists(LOG_FILE):
    with open(LOG_FILE, "w", newline="") as f:
        csv.writer(f).writerow(
            ["timestamp","current","vibration","temperature","pressure","rpm","power","dio"])

# ===== 简单 FFT（Cooley-Tukey，纯 Python）=====
def simple_fft(data):
    """计算实数序列的 FFT 幅度谱"""
    n = len(data)
    if n == 0: return []
    # 补零到 2 的幂次
    p = 1
    while p < n: p <<= 1
    x = [complex(v, 0) for v in data] + [0] * (p - n)
    # Cooley-Tukey 迭代 FFT
    j = 0
    for i in range(1, p):
        bit = p >> 1
        while j & bit:
            j ^= bit
            bit >>= 1
        j ^= bit
        if i < j: x[i], x[j] = x[j], x[i]
    length = 2
    while length <= p:
        w = complex(math.cos(-2*math.pi/length), math.sin(-2*math.pi/length))
        for i in range(0, p, length):
            wn = complex(1, 0)
            for k in range(length // 2):
                u, v = x[i+k], x[i+k+length//2] * wn
                x[i+k], x[i+k+length//2] = u+v, u-v
                wn *= w
        length <<= 1
    return [abs(x[i]) / p for i in range(p // 2)]

def update_industrial(current, vibration, temperature, pressure,
                      rpm, power, dio, uptime_ms):
    """MCU 通过 Bridge.call('update_industrial', ...) 调用"""
    now = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

    with lock:
        state.update({
            "current":     float(current),
            "vibration":   float(vibration),
            "temperature": float(temperature),
            "pressure":    float(pressure),
            "rpm":         float(rpm),
            "power":       float(power),
            "dio":         int(dio),
            "uptime_ms":   int(uptime_ms),
            "updated_at":  now,
        })
        state["sample_count"] += 1

        for k in ["current","vibration","temperature","pressure","rpm","power"]:
            history[k].append(float(state[k]))
        history["timestamps"].append(now)

        vib_buffer.append(float(vibration))

        # 告警检测
        _check_alerts(float(current), float(vibration),
                      float(temperature), float(pressure), float(rpm))

    # CSV 日志（锁外写入）
    try:
        with open(LOG_FILE, "a", newline="") as f:
            csv.writer(f).writerow([
                now, round(float(current),3), round(float(vibration),4),
                round(float(temperature),1), round(float(pressure),2),
                round(float(rpm),1), round(float(power),1), int(dio)
            ])
    except Exception:
        pass

def _check_alerts(current, vibration, temperature, pressure, rpm):
    now_str = datetime.datetime.now().strftime("%H:%M:%S")
    checks = [
        (current     > THRESHOLDS["current_max"],     f"电流过高: {current:.1f}A",       "danger"),
        (vibration   > THRESHOLDS["vibration_max"],   f"振动过大: {vibration:.3f}g",      "warning"),
        (temperature > THRESHOLDS["temperature_max"], f"温度过高: {temperature:.1f}°C",   "danger"),
        (pressure    > THRESHOLDS["pressure_max"],    f"压力过高: {pressure:.2f}bar",     "warning"),
        (rpm         > THRESHOLDS["rpm_max"],         f"转速过高: {rpm:.0f}RPM",          "warning"),
    ]
    for triggered, msg, level in checks:
        if triggered:
            alerts.appendleft({"level": level, "msg": msg, "time": now_str})

Bridge.provide("update_industrial", update_industrial)

# ===== REST API =====
def api_state(_req=None):
    with lock:
        s = dict(state)
    # 计算 DIO 位状态
    dio = s.get("dio", 0)
    s["dio_bits"] = [(dio >> i) & 1 for i in range(3)]
    return s

def api_history(_req=None):
    with lock:
        return {k: list(v) for k, v in history.items()}

def api_fft(_req=None):
    with lock:
        buf = list(vib_buffer)
    if len(buf) < 8:
        return {"freqs": [], "magnitudes": []}
    mags = simple_fft(buf)
    sample_rate = 2.0  # 2Hz 采样
    freqs = [i * sample_rate / len(mags) for i in range(len(mags))]
    return {"freqs": freqs, "magnitudes": mags}

def api_alerts(_req=None):
    with lock:
        return {"alerts": list(alerts)}

web.expose_api("GET", "/api/state",   api_state)
web.expose_api("GET", "/api/history", api_history)
web.expose_api("GET", "/api/fft",     api_fft)
web.expose_api("GET", "/api/alerts",  api_alerts)

App.run()