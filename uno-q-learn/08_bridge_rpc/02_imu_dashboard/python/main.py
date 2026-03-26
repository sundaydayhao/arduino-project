"""
IMU 运动仪表盘 - Linux MPU 端
Arduino UNO Q (Qualcomm)

接收 MCU 的 IMU 数据，提供 REST API，
前端用 Three.js 渲染实时 3D 姿态可视化。
"""

import datetime
import threading
import collections
from arduino.app_utils import App, Bridge
from arduino.app_bricks.web_ui import WebUI

print("IMU 仪表盘 Python 端启动", flush=True)

web  = WebUI()
lock = threading.Lock()

state = {
    "roll": 0.0, "pitch": 0.0, "yaw": 0.0,
    "ax": 0.0, "ay": 0.0, "az": 0.0,
    "gx": 0.0, "gy": 0.0, "gz": 0.0,
    "temperature": 0.0,
    "accel_mag": 0.0,
    "updated_at": None,
    "sample_count": 0,
}

# 历史数据（用于图表）
HISTORY = 200
history = {
    "roll":      collections.deque(maxlen=HISTORY),
    "pitch":     collections.deque(maxlen=HISTORY),
    "yaw":       collections.deque(maxlen=HISTORY),
    "accel_mag": collections.deque(maxlen=HISTORY),
    "timestamps": collections.deque(maxlen=HISTORY),
}

# 运动检测
MOTION_THRESHOLD = 10.5  # m/s² (静止约 9.8)
motion_events = collections.deque(maxlen=50)

def update_imu(roll, pitch, yaw, ax, ay, az, gx, gy, gz, temperature, accel_mag):
    """MCU 通过 Bridge.call('update_imu', ...) 调用"""
    now = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

    with lock:
        state.update({
            "roll": float(roll), "pitch": float(pitch), "yaw": float(yaw),
            "ax": float(ax), "ay": float(ay), "az": float(az),
            "gx": float(gx), "gy": float(gy), "gz": float(gz),
            "temperature": float(temperature),
            "accel_mag": float(accel_mag),
            "updated_at": now,
        })
        state["sample_count"] += 1

        history["roll"].append(float(roll))
        history["pitch"].append(float(pitch))
        history["yaw"].append(float(yaw))
        history["accel_mag"].append(float(accel_mag))
        history["timestamps"].append(now)

        # 运动事件检测
        if float(accel_mag) > MOTION_THRESHOLD:
            motion_events.append({
                "time": datetime.datetime.now().strftime("%H:%M:%S"),
                "magnitude": round(float(accel_mag), 2),
                "roll": round(float(roll), 1),
                "pitch": round(float(pitch), 1),
            })

Bridge.provide("update_imu", update_imu)

def api_state(_req=None):
    with lock:
        return dict(state)

def api_history(_req=None):
    with lock:
        return {
            "roll":      list(history["roll"]),
            "pitch":     list(history["pitch"]),
            "yaw":       list(history["yaw"]),
            "accel_mag": list(history["accel_mag"]),
            "timestamps": list(history["timestamps"]),
        }

def api_motion(_req=None):
    with lock:
        return {"events": list(motion_events)}

web.expose_api("GET", "/api/state",   api_state)
web.expose_api("GET", "/api/history", api_history)
web.expose_api("GET", "/api/motion",  api_motion)

App.run()