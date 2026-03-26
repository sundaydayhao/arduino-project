"""
TFLite 人体检测 - Linux MPU 端
Arduino UNO Q (Qualcomm)

双脑架构：
  MCU (Cortex-M33)  → TFLite 推理，每帧 ~200ms
  Linux MPU (本文件) → 接收结果、日志记录、REST API、告警

MCU 通过 Bridge.call("update_detection", ...) 发送推理结果，
本脚本提供 Web 仪表盘和历史统计。

运行方式：
  Arduino App Lab 部署，或 python main.py
"""

import datetime
import threading
import collections
from arduino.app_utils import App, Bridge
from arduino.app_bricks.web_ui import WebUI

print("人体检测 MPU 端启动", flush=True)

web  = WebUI()
lock = threading.Lock()

HISTORY_LEN = 200

# ===== 状态存储 =====
state = {
    "person_score":     None,
    "no_person_score":  None,
    "person_detected":  False,
    "inference_ms":     None,
    "fps":              None,
    "updated_at":       None,
    "total_frames":     0,
    "person_count":     0,
    "avg_inference_ms": 0.0,
    "detection_rate":   0.0,
}

history = {
    "person_score":    collections.deque(maxlen=HISTORY_LEN),
    "no_person_score": collections.deque(maxlen=HISTORY_LEN),
    "inference_ms":    collections.deque(maxlen=HISTORY_LEN),
    "timestamps":      collections.deque(maxlen=HISTORY_LEN),
}

alerts = []

# ===== Bridge 回调：接收 MCU 推理结果 =====
def update_detection(person_score, no_person_score, inference_ms):
    """
    MCU 在每帧推理后调用：
      Bridge.call("update_detection", personScore, noPersonScore, inferenceMs)
    """
    now = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    ps  = float(person_score)
    nps = float(no_person_score)
    ms  = int(float(inference_ms))
    detected = ps > 0.7

    with lock:
        state["person_score"]    = ps
        state["no_person_score"] = nps
        state["person_detected"] = detected
        state["inference_ms"]    = ms
        state["fps"]             = round(1000.0 / ms, 1) if ms > 0 else 0
        state["updated_at"]      = now
        state["total_frames"]   += 1
        if detected:
            state["person_count"] += 1

        n = state["total_frames"]
        state["avg_inference_ms"] = round(
            (state["avg_inference_ms"] * (n - 1) + ms) / n, 1)
        state["detection_rate"] = round(
            state["person_count"] / n * 100, 1)

        history["person_score"].append(ps)
        history["no_person_score"].append(nps)
        history["inference_ms"].append(ms)
        history["timestamps"].append(now)

        # 告警：连续检测到人体
        if detected:
            _add_alert("info", f"检测到人体 (置信度 {ps*100:.1f}%)", now)

    status = "✅ 检测到人体" if detected else "⬜ 无人"
    print(f"[{now}] {status} | 人体:{ps*100:.1f}% 无人:{nps*100:.1f}% | {ms}ms", flush=True)


def _add_alert(level, msg, time_str):
    """添加告警（最多保留 30 条）"""
    global alerts
    alerts = ([{"level": level, "msg": msg, "time": time_str}] + alerts)[:30]


# 注册 Bridge 函数
Bridge.provide("update_detection", update_detection)


# ===== REST API =====
def api_state(_req=None):
    """当前检测状态"""
    with lock:
        return dict(state)


def api_history(_req=None):
    """历史推理数据（用于图表）"""
    with lock:
        return {
            "timestamps":      list(history["timestamps"]),
            "person_score":    list(history["person_score"]),
            "no_person_score": list(history["no_person_score"]),
            "inference_ms":    list(history["inference_ms"]),
        }


def api_alerts(_req=None):
    """告警列表"""
    with lock:
        return {"alerts": alerts}


web.expose_api("GET", "/api/state",   api_state)
web.expose_api("GET", "/api/history", api_history)
web.expose_api("GET", "/api/alerts",  api_alerts)

App.run()