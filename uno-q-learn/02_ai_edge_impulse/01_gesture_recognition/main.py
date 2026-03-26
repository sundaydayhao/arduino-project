"""
Edge Impulse 手势识别 - Linux MPU 端
Arduino UNO Q (Qualcomm)

双脑架构：
  MCU (Cortex-M33)  → IMU 数据采集 + Edge Impulse 推理，~20ms
  Linux MPU (本文件) → 接收手势事件，执行自动化动作，历史统计
"""

import datetime
import threading
import collections
from arduino.app_utils import App, Bridge
from arduino.app_bricks.web_ui import WebUI

print("手势识别 MPU 端启动", flush=True)

web  = WebUI()
lock = threading.Lock()

GESTURE_ACTIONS = {
    "circle":    "切换模式",
    "wave":      "音量控制",
    "punch":     "确认操作",
    "flex":      "取消操作",
    "idle":      "待机",
}

state = {
    "last_gesture":    None,
    "last_confidence": None,
    "updated_at":      None,
    "total_count":     0,
}

history      = collections.deque(maxlen=100)
gesture_counts = {}


def on_gesture(gesture, confidence, inference_ms):
    now  = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    g    = str(gesture).strip()
    conf = float(confidence)
    ms   = int(float(inference_ms))

    with lock:
        state["last_gesture"]    = g
        state["last_confidence"] = conf
        state["updated_at"]      = now
        state["total_count"]    += 1
        gesture_counts[g] = gesture_counts.get(g, 0) + 1
        history.appendleft({
            "gesture":    g,
            "confidence": round(conf * 100, 1),
            "action":     GESTURE_ACTIONS.get(g, "未知"),
            "inference_ms": ms,
            "time":       now,
        })

    action = GESTURE_ACTIONS.get(g, "未知")
    print(f"[{now}] 手势: {g:<10s} ({conf*100:.1f}%) -> {action} | {ms}ms", flush=True)


Bridge.provide("on_gesture", on_gesture)


def api_state(_req=None):
    with lock:
        return {**state, "gesture_counts": dict(gesture_counts)}


def api_history(_req=None):
    with lock:
        return {"history": list(history)}


web.expose_api("GET", "/api/state",   api_state)
web.expose_api("GET", "/api/history", api_history)

App.run()