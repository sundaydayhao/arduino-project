"""
AI 推理结果仪表盘 - Linux MPU 端
Arduino UNO Q (Qualcomm)

接收 MCU TFLite 推理结果，
提供 REST API，展示实时 AI 推理可视化。
"""

import datetime
import threading
import collections
from arduino.app_utils import App, Bridge
from arduino.app_bricks.web_ui import WebUI

print("AI 推理仪表盘 Python 端启动", flush=True)

web  = WebUI()
lock = threading.Lock()

GESTURES = ["idle", "swipe_up", "swipe_down",
            "swipe_left", "swipe_right", "circle", "shake"]

state = {
    "gesture":        "idle",
    "confidence":     0.0,
    "scores":         [0.0] * 7,
    "inference_ms":   0,
    "total_count":    0,
    "avg_ms":         0.0,
    "updated_at":     None,
}

# 推理历史
HISTORY = 100
history = {
    "gestures":    collections.deque(maxlen=HISTORY),
    "confidence":  collections.deque(maxlen=HISTORY),
    "latency":     collections.deque(maxlen=HISTORY),
    "timestamps":  collections.deque(maxlen=HISTORY),
}

# 手势统计
gesture_counts = {g: 0 for g in GESTURES}

# 最近检测到的手势（非 idle）
recent_detections = collections.deque(maxlen=20)

def update_inference(gesture, confidence,
                     s0, s1, s2, s3, s4, s5, s6,
                     inference_ms, total_count, avg_ms):
    """MCU 通过 Bridge.call('update_inference', ...) 调用"""
    now = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

    with lock:
        state["gesture"]      = str(gesture)
        state["confidence"]   = float(confidence)
        state["scores"]       = [float(s0), float(s1), float(s2), float(s3),
                                  float(s4), float(s5), float(s6)]
        state["inference_ms"] = int(inference_ms)
        state["total_count"]  = int(total_count)
        state["avg_ms"]       = float(avg_ms)
        state["updated_at"]   = now

        history["gestures"].append(str(gesture))
        history["confidence"].append(float(confidence))
        history["latency"].append(int(inference_ms))
        history["timestamps"].append(now)

        gesture_counts[str(gesture)] = gesture_counts.get(str(gesture), 0) + 1

        # 记录非 idle 手势
        if str(gesture) != "idle" and float(confidence) > 0.75:
            recent_detections.append({
                "gesture":    str(gesture),
                "confidence": round(float(confidence) * 100, 1),
                "time":       datetime.datetime.now().strftime("%H:%M:%S"),
            })

Bridge.provide("update_inference", update_inference)

def api_state(_req=None):
    with lock:
        return {
            **state,
            "gesture_labels": GESTURES,
        }

def api_history(_req=None):
    with lock:
        return {
            "gestures":   list(history["gestures"]),
            "confidence": list(history["confidence"]),
            "latency":    list(history["latency"]),
        }

def api_stats(_req=None):
    with lock:
        total = sum(gesture_counts.values()) or 1
        return {
            "counts": gesture_counts,
            "percentages": {g: round(c * 100 / total, 1)
                            for g, c in gesture_counts.items()},
            "recent": list(recent_detections),
        }

web.expose_api("GET", "/api/state",   api_state)
web.expose_api("GET", "/api/history", api_history)
web.expose_api("GET", "/api/stats",   api_stats)

App.run()