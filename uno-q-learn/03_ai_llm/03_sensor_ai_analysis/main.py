"""
传感器数据 AI 分析 - Linux MPU 端
Arduino UNO Q (Qualcomm)

双脑架构：
  MCU (Cortex-M33)  → 传感器采集（温湿度/气压/IMU），定期上报
  Linux MPU (本文件) → 调用 LLM 分析传感器数据，生成洞察报告

MPU 积累一段时间的传感器数据后，调用 LLM 进行趋势分析和异常解读。
"""

import os
import datetime
import threading
import collections
import json
import urllib.request
import urllib.error
from arduino.app_utils import App, Bridge
from arduino.app_bricks.web_ui import WebUI

print("传感器 AI 分析 MPU 端启动", flush=True)

web  = WebUI()
lock = threading.Lock()

OPENAI_API_KEY = os.environ.get("OPENAI_API_KEY", "")
OPENAI_MODEL   = os.environ.get("OPENAI_MODEL", "gpt-4o-mini")

# 传感器数据缓冲（最近 60 条）
sensor_buffer = collections.deque(maxlen=60)

state = {
    "temperature": None,
    "humidity":    None,
    "pressure":    None,
    "ax": None, "ay": None, "az": None,
    "updated_at":  None,
    "last_analysis": None,
    "analysis_at":   None,
    "total_samples": 0,
}

ANALYSIS_INTERVAL = 30  # 每 30 条数据分析一次
sample_count = 0


def call_llm_analysis(data_summary):
    """调用 LLM 分析传感器数据"""
    if not OPENAI_API_KEY:
        return "[错误] 未设置 OPENAI_API_KEY"

    prompt = f"""你是一个 IoT 传感器数据分析专家。
以下是 Arduino UNO Q 最近采集的传感器数据摘要：

{data_summary}

请用中文简洁分析：
1. 数据是否正常？
2. 有无异常趋势？
3. 给出 1-2 条建议。
回复不超过 150 字。"""

    payload = json.dumps({
        "model": OPENAI_MODEL,
        "messages": [{"role": "user", "content": prompt}],
        "max_tokens": 200,
        "temperature": 0.3,
    }).encode("utf-8")

    req = urllib.request.Request(
        "https://api.openai.com/v1/chat/completions",
        data=payload,
        headers={
            "Content-Type":  "application/json",
            "Authorization": f"Bearer {OPENAI_API_KEY}",
        },
        method="POST",
    )
    try:
        with urllib.request.urlopen(req, timeout=15) as resp:
            data = json.loads(resp.read().decode("utf-8"))
            return data["choices"][0]["message"]["content"].strip()
    except Exception as e:
        return f"[分析失败] {e}"


def update_sensors(temperature, humidity, pressure, ax, ay, az):
    """
    MCU 定期上报传感器数据：
      Bridge.call("update_sensors", temp, humi, pres, ax, ay, az)
    """
    global sample_count
    now  = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    temp = float(temperature)
    humi = float(humidity)
    pres = float(pressure)
    _ax  = float(ax)
    _ay  = float(ay)
    _az  = float(az)

    with lock:
        state.update({
            "temperature": round(temp, 1),
            "humidity":    round(humi, 1),
            "pressure":    round(pres, 1),
            "ax": round(_ax, 3), "ay": round(_ay, 3), "az": round(_az, 3),
            "updated_at":  now,
        })
        state["total_samples"] += 1
        sensor_buffer.append({
            "t": temp, "h": humi, "p": pres,
            "ax": _ax, "ay": _ay, "az": _az,
            "time": now,
        })
        sample_count += 1

    print(f"[{now}] T:{temp:.1f}C H:{humi:.1f}% P:{pres:.1f}hPa "
          f"IMU:[{_ax:.2f},{_ay:.2f},{_az:.2f}]", flush=True)

    # 定期触发 AI 分析
    if sample_count % ANALYSIS_INTERVAL == 0:
        threading.Thread(target=_run_analysis, daemon=True).start()


def _run_analysis():
    """后台线程执行 LLM 分析"""
    with lock:
        buf = list(sensor_buffer)

    if not buf:
        return

    temps = [d["t"] for d in buf]
    humis = [d["h"] for d in buf]
    pres  = [d["p"] for d in buf]

    summary = (
        f"样本数: {len(buf)}\n"
        f"温度: 均值={sum(temps)/len(temps):.1f}C, "
        f"最高={max(temps):.1f}C, 最低={min(temps):.1f}C\n"
        f"湿度: 均值={sum(humis)/len(humis):.1f}%, "
        f"最高={max(humis):.1f}%, 最低={min(humis):.1f}%\n"
        f"气压: 均值={sum(pres)/len(pres):.1f}hPa\n"
        f"最新时间: {buf[0]['time']}"
    )

    print(f"\n[AI 分析] 开始分析 {len(buf)} 条数据...", flush=True)
    analysis = call_llm_analysis(summary)

    with lock:
        state["last_analysis"] = analysis
        state["analysis_at"]   = datetime.datetime.now(
            datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")

    print(f"[AI 分析] {analysis}\n", flush=True)


Bridge.provide("update_sensors", update_sensors)


def api_state(_req=None):
    with lock:
        return dict(state)


def api_data(_req=None):
    with lock:
        return {"data": list(sensor_buffer)}


web.expose_api("GET", "/api/state", api_state)
web.expose_api("GET", "/api/data",  api_data)

App.run()