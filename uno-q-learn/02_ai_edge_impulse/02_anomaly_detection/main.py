"""
Edge Impulse 异常检测 - Linux MPU 端
Arduino UNO Q (Qualcomm)

双脑架构：
  MCU (Cortex-M33)  → 传感器采集 + EI 异常评分，实时检测
  Linux MPU (本文件) → 接收异常事件，告警推送，趋势分析，REST API
"""

import datetime
import threading
import collections
from arduino.app_utils import App, Bridge
from arduino.app_bricks.web_ui import WebUI

print("异常检测 MPU 端启动", flush=True)

web  = WebUI()
lock = threading.Lock()

ANOMALY_THRESHOLD = 0.6  # 与 MCU 端保持一致

state = {
    "anomaly_score":   None,
    "is_anomaly":      False,
    "updated_at":      None,
    "total_samples":   0,
    "anomaly_count":   0,
    "anomaly_rate":    0.0,
}

history = collections.deque(maxlen=200)


def update_anomaly(anomaly_score, is_anomaly, inference_ms):
    """
    MCU 每次推理后调用：
      Bridge.call("update_anomaly", anomalyScore, isAnomaly, inferenceMs)
    """
    now   = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    score = float(anomaly_score)
    anom  = bool(int(float(is_anomaly)))
    ms    = int(float(inference_ms))

    with lock:
        state["anomaly_score"] = score
        state["is_anomaly"]    = anom
        state["updated_at"]    = now
        state["total_samples"] += 1
        if anom:
            state["anomaly_count"] += 1
        total = state["total_samples"]
        state["anomaly_rate"] = round(state["anomaly_count"] / total * 100, 2)

        history.appendleft({
            "score":      round(score, 4),
            "is_anomaly": anom,
            "ms":         ms,
            "time":       now,
        })

    if anom:
        print(f"[{now}] *** 异常! 评分: {score:.4f} | {ms}ms ***", flush=True)
    else:
        print(f"[{now}] 正常  评分: {score:.4f} | {ms}ms", flush=True)


Bridge.provide("update_anomaly", update_anomaly)


def api_state(_req=None):
    with lock:
        return dict(state)


def api_history(_req=None):
    with lock:
        return {"history": list(history)}


web.expose_api("GET", "/api/state",   api_state)
web.expose_api("GET", "/api/history", api_history)

App.run()