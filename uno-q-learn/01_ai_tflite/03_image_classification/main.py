"""
TFLite MobileNet 图像分类 - Linux MPU 端
Arduino UNO Q (Qualcomm)

双脑架构：
  MCU (Cortex-M33)  → MobileNetV2 推理，Top-5 分类，~200ms/帧
  Linux MPU (本文件) → 接收分类结果、统计分析、REST API、Web 仪表盘

MCU 通过 Bridge.call("update_classification", ...) 发送 Top-1 结果。
"""

import datetime
import threading
import collections
from arduino.app_utils import App, Bridge
from arduino.app_bricks.web_ui import WebUI

print("图像分类 MPU 端启动", flush=True)

web  = WebUI()
lock = threading.Lock()

HISTORY_LEN = 100

state = {
    "top_label":       None,
    "top_score":       None,
    "class_id":        None,
    "inference_ms":    None,
    "fps":             None,
    "updated_at":      None,
    "total_frames":    0,
    "avg_inference_ms": 0.0,
}

# 分类历史
history = collections.deque(maxlen=HISTORY_LEN)

# 类别出现频次统计
class_counts = {}


def update_classification(label, score, class_id, inference_ms):
    """
    MCU 每帧推理后调用：
      Bridge.call("update_classification", topLabel, topScore, classId, inferenceMs)
    """
    now  = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    lbl  = str(label).strip()
    sc   = float(score)
    cid  = int(float(class_id))
    ms   = int(float(inference_ms))

    with lock:
        state["top_label"]    = lbl
        state["top_score"]    = sc
        state["class_id"]     = cid
        state["inference_ms"] = ms
        state["fps"]          = round(1000.0 / ms, 1) if ms > 0 else 0
        state["updated_at"]   = now
        state["total_frames"] += 1

        n = state["total_frames"]
        state["avg_inference_ms"] = round(
            (state["avg_inference_ms"] * (n - 1) + ms) / n, 1)

        class_counts[lbl] = class_counts.get(lbl, 0) + 1

        history.appendleft({
            "label":       lbl,
            "score":       round(sc * 100, 1),
            "class_id":    cid,
            "inference_ms": ms,
            "time":        now,
        })

    print(f"[{now}] [{cid:4d}] {lbl:<30s} {sc*100:.1f}% | {ms}ms", flush=True)


Bridge.provide("update_classification", update_classification)


def api_state(_req=None):
    with lock:
        return dict(state)


def api_history(_req=None):
    with lock:
        return {"history": list(history)}


def api_top_classes(_req=None):
    """出现频次最高的 Top-20 类别"""
    with lock:
        total = state["total_frames"]
        top = sorted(class_counts.items(), key=lambda x: x[1], reverse=True)[:20]
        return {
            "total_frames": total,
            "top_classes": [
                {"label": lbl, "count": cnt,
                 "percent": round(cnt / total * 100, 1) if total > 0 else 0}
                for lbl, cnt in top
            ],
        }


web.expose_api("GET", "/api/state",       api_state)
web.expose_api("GET", "/api/history",     api_history)
web.expose_api("GET", "/api/top_classes", api_top_classes)

App.run()