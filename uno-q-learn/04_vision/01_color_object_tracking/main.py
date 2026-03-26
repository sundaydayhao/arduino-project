"""
颜色目标追踪 - Linux MPU 端
Arduino UNO Q (Qualcomm)

双脑架构：
  MCU (Cortex-M33)  → 摄像头采集，颜色检测，舵机控制
  Linux MPU (本文件) → 接收追踪数据，记录轨迹，REST API，Web 仪表盘
"""

import datetime
import threading
import collections
from arduino.app_utils import App, Bridge
from arduino.app_bricks.web_ui import WebUI

print("颜色目标追踪 MPU 端启动", flush=True)

web  = WebUI()
lock = threading.Lock()

state = {
    "detected":    False,
    "color":       None,
    "cx":          None,
    "cy":          None,
    "area":        None,
    "pan_angle":   None,
    "tilt_angle":  None,
    "fps":         None,
    "updated_at":  None,
    "total_frames": 0,
    "detect_count": 0,
}

trajectory = collections.deque(maxlen=200)


def update_tracking(detected, color, cx, cy, area, pan_angle, tilt_angle, fps):
    """
    MCU 每帧上报追踪数据：
      Bridge.call("update_tracking", detected, color, cx, cy, area, pan, tilt, fps)
    """
    now = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    det = bool(int(float(detected)))

    with lock:
        state["detected"]   = det
        state["color"]      = str(color).strip()
        state["cx"]         = int(float(cx))
        state["cy"]         = int(float(cy))
        state["area"]       = int(float(area))
        state["pan_angle"]  = round(float(pan_angle), 1)
        state["tilt_angle"] = round(float(tilt_angle), 1)
        state["fps"]        = round(float(fps), 1)
        state["updated_at"] = now
        state["total_frames"] += 1
        if det:
            state["detect_count"] += 1
            trajectory.appendleft({
                "cx": state["cx"], "cy": state["cy"],
                "area": state["area"], "time": now,
            })

    if det:
        print(f"[{now}] 追踪 {color} | 中心:({state['cx']},{state['cy']}) "
              f"面积:{state['area']} 云台:({state['pan_angle']},{state['tilt_angle']}) "
              f"FPS:{state['fps']}", flush=True)


Bridge.provide("update_tracking", update_tracking)


def api_state(_req=None):
    with lock:
        return dict(state)


def api_trajectory(_req=None):
    with lock:
        return {"trajectory": list(trajectory)}


web.expose_api("GET", "/api/state",      api_state)
web.expose_api("GET", "/api/trajectory", api_trajectory)

App.run()