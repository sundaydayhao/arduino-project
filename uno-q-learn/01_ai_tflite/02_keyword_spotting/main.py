"""
TFLite 语音关键词识别 - Linux MPU 端
Arduino UNO Q (Qualcomm)

双脑架构：
  MCU (Cortex-M33)  → PDM 麦克风采集 + DS-CNN 推理，~10ms/次
  Linux MPU (本文件) → 接收关键词事件、执行系统动作、历史日志

MCU 检测到关键词后通过 Bridge.call("on_keyword", ...) 通知 MPU，
MPU 可执行更复杂的动作（播放音乐、控制智能家居、发送通知等）。
"""

import datetime
import threading
import collections
import subprocess
from arduino.app_utils import App, Bridge
from arduino.app_bricks.web_ui import WebUI

print("语音关键词识别 MPU 端启动", flush=True)

web  = WebUI()
lock = threading.Lock()

HISTORY_LEN = 100

# ===== 状态存储 =====
state = {
    "last_keyword":    None,
    "last_confidence": None,
    "updated_at":      None,
    "total_detections": 0,
}

# 关键词历史记录
keyword_history = collections.deque(maxlen=HISTORY_LEN)

# 关键词统计计数
keyword_counts = {}

# ===== 关键词动作映射（MPU 端执行更复杂的动作）=====
KEYWORD_ACTIONS = {
    "yes":   "确认操作",
    "no":    "取消操作",
    "up":    "音量增大",
    "down":  "音量减小",
    "left":  "上一曲",
    "right": "下一曲",
    "on":    "开启设备",
    "off":   "关闭设备",
    "stop":  "停止播放",
    "go":    "开始播放",
}


# ===== Bridge 回调：接收 MCU 关键词检测结果 =====
def on_keyword(keyword, confidence):
    """
    MCU 检测到关键词后调用：
      Bridge.call("on_keyword", keyword, confidence)
    """
    now  = datetime.datetime.now().strftime("%Y-%m-%dT%H:%M:%SZ")
    kw   = str(keyword).strip()
    conf = float(confidence)

    with lock:
        state["last_keyword"]    = kw
        state["last_confidence"] = conf
        state["updated_at"]      = now
        state["total_detections"] += 1

        # 更新统计
        keyword_counts[kw] = keyword_counts.get(kw, 0) + 1

        # 追加历史
        keyword_history.appendleft({
            "keyword":    kw,
            "confidence": round(conf * 100, 1),
            "action":     KEYWORD_ACTIONS.get(kw, "未知动作"),
            "time":       now,
        })

    action = KEYWORD_ACTIONS.get(kw, "未知动作")
    print(f"[{now}] 🎤 关键词: '{kw}' ({conf*100:.1f}%) → {action}", flush=True)

    # 执行 MPU 端动作
    _execute_action(kw, conf)


def _execute_action(keyword, confidence):
    """在 Linux MPU 上执行对应动作"""
    if confidence < 0.8:
        return  # 置信度不足，忽略

    # 示例：调用系统命令（可替换为实际动作）
    if keyword == "up":
        # subprocess.run(["amixer", "set", "Master", "10%+"], capture_output=True)
        print("  → 执行: 音量 +10%", flush=True)
    elif keyword == "down":
        # subprocess.run(["amixer", "set", "Master", "10%-"], capture_output=True)
        print("  → 执行: 音量 -10%", flush=True)
    elif keyword == "stop":
        # subprocess.run(["playerctl", "stop"], capture_output=True)
        print("  → 执行: 停止播放", flush=True)
    elif keyword == "go":
        # subprocess.run(["playerctl", "play"], capture_output=True)
        print("  → 执行: 开始播放", flush=True)


# 注册 Bridge 函数
Bridge.provide("on_keyword", on_keyword)


# ===== REST API =====
def api_state(_req=None):
    """当前识别状态"""
    with lock:
        return {
            **state,
            "keyword_counts": dict(keyword_counts),
        }


def api_history(_req=None):
    """关键词历史记录"""
    with lock:
        return {"history": list(keyword_history)}


def api_stats(_req=None):
    """关键词统计"""
    with lock:
        total = state["total_detections"]
        stats = [
            {
                "keyword": kw,
                "count":   cnt,
                "percent": round(cnt / total * 100, 1) if total > 0 else 0,
                "action":  KEYWORD_ACTIONS.get(kw, "未知"),
            }
            for kw, cnt in sorted(keyword_counts.items(),
                                  key=lambda x: x[1], reverse=True)
        ]
        return {"total": total, "stats": stats}


web.expose_api("GET", "/api/state",   api_state)
web.expose_api("GET", "/api/history", api_history)
web.expose_api("GET", "/api/stats",   api_stats)

App.run()