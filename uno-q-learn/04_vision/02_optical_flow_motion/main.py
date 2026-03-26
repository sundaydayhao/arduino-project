"""
光流运动检测 - Linux MPU 端
Arduino UNO Q (Qualcomm)

双脑架构：
  MCU (Cortex-M33)  → 摄像头采集，Lucas-Kanade 光流计算，运动方向估计
  Linux MPU (本文件) → 接收运动事件，告警推送，轨迹统计，REST API，Web 仪表盘

应用场景：入侵检测、人流统计、手势控制、无人机避障
"""

import datetime
import threading
import collections
import math
from arduino.app_utils import App, Bridge
from arduino.app_bricks.web_ui import WebUI

print("光流运动检测 MPU 端启动", flush=True)

web  = WebUI()
lock = threading.Lock()

# ===== 状态 =====
state = {
    "speed":          0.0,
    "direction":      0.0,
    "direction_name": None,
    "coverage":       0.0,
    "avg_vx":         0.0,
    "avg_vy":         0.0,
    "fps":            0.0,
    "is_alarm":       False,
    "updated_at":     None,
    "total_events":   0,
    "alarm_count":    0,
}

# 运动事件历史
event_history = collections.deque(maxlen=200)

# 方向统计（8 个方向）
direction_counts = {
    "右": 0, "右下": 0, "下": 0, "左下": 0,
    "左": 0, "左上": 0, "上": 0, "右上": 0,
}

# 速度时序（用于绘图）
speed_series = collections.deque(maxlen=100)


def _direction_name(deg):
    deg = float(deg) % 360
    if deg < 22.5 or deg >= 337.5:  return "右"
    if deg < 67.5:   return "右下"
    if deg < 112.5:  return "下"
    if deg < 157.5:  return "左下"
    if deg < 202.5:  return "左"
    if deg < 247.5:  return "左上"
    if deg < 292.5:  return "上"
    return "右上"


def on_motion(speed, direction, coverage, is_alarm, avg_vx, avg_vy, fps):
    """
    MCU 检测到运动时调用：
      Bridge.call("on_motion", speed, direction, coverage,
                  isAlarm, avgVx, avgVy, fps)
    """
    now   = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    spd   = float(speed)
    deg   = float(direction)
    cov   = float(coverage)
    alarm = bool(int(float(is_alarm)))
    vx    = float(avg_vx)
    vy    = float(avg_vy)
    _fps  = float(fps)
    dname = _direction_name(deg)

    with lock:
        state["speed"]          = round(spd, 2)
        state["direction"]      = round(deg, 1)
        state["direction_name"] = dname
        state["coverage"]       = round(cov, 3)
        state["avg_vx"]         = round(vx, 2)
        state["avg_vy"]         = round(vy, 2)
        state["fps"]            = round(_fps, 1)
        state["is_alarm"]       = alarm
        state["updated_at"]     = now
        state["total_events"]  += 1
        if alarm:
            state["alarm_count"] += 1

        direction_counts[dname] = direction_counts.get(dname, 0) + 1

        event_history.appendleft({
            "speed":     round(spd, 2),
            "direction": round(deg, 1),
            "dir_name":  dname,
            "coverage":  round(cov * 100, 1),
            "is_alarm":  alarm,
            "vx":        round(vx, 2),
            "vy":        round(vy, 2),
            "time":      now,
        })

        speed_series.appendleft({"speed": round(spd, 2), "time": now})

    if alarm:
        print(f"[{now}] *** 告警! 速度:{spd:.1f}px/帧 方向:{dname}({deg:.0f}°) "
              f"覆盖:{cov*100:.0f}% FPS:{_fps:.1f} ***", flush=True)
    else:
        print(f"[{now}] 运动 速度:{spd:.1f}px/帧 方向:{dname}({deg:.0f}°) "
              f"覆盖:{cov*100:.0f}% FPS:{_fps:.1f}", flush=True)


Bridge.provide("on_motion", on_motion)


# ===== REST API =====
def api_state(_req=None):
    with lock:
        return dict(state)


def api_events(_req=None):
    with lock:
        return {"events": list(event_history)}


def api_stats(_req=None):
    with lock:
        total = state["total_events"]
        alarm = state["alarm_count"]
        speeds = [d["speed"] for d in speed_series]
        return {
            "total_events":  total,
            "alarm_count":   alarm,
            "alarm_rate":    round(alarm / total * 100, 1) if total > 0 else 0,
            "avg_speed":     round(sum(speeds) / len(speeds), 2) if speeds else 0,
            "max_speed":     round(max(speeds), 2) if speeds else 0,
            "direction_dist": dict(direction_counts),
        }


def api_speed_series(_req=None):
    with lock:
        return {"series": list(speed_series)}


web.expose_api("GET", "/api/state",        api_state)
web.expose_api("GET", "/api/events",       api_events)
web.expose_api("GET", "/api/stats",        api_stats)
web.expose_api("GET", "/api/speed_series", api_speed_series)

App.run()