"""
Gemini Vision API 图像理解 - Linux MPU 端
Arduino UNO Q (Qualcomm)

双脑架构：
  MCU (Cortex-M33)  → 摄像头拍照，Base64 编码，发送给 MPU
  Linux MPU (本文件) → 调用 Gemini Vision API，返回图像描述

Linux MPU 处理图像 Base64 编码和 API 调用，
MCU 负责摄像头控制和结果显示。
"""

import os
import base64
import datetime
import threading
import collections
import json
import urllib.request
import urllib.error
from arduino.app_utils import App, Bridge
from arduino.app_bricks.web_ui import WebUI

print("Gemini Vision MPU 端启动", flush=True)

web  = WebUI()
lock = threading.Lock()

GEMINI_API_KEY = os.environ.get("GEMINI_API_KEY", "")
GEMINI_MODEL   = "gemini-1.5-flash"
GEMINI_URL     = (
    f"https://generativelanguage.googleapis.com/v1beta/models/"
    f"{GEMINI_MODEL}:generateContent?key={GEMINI_API_KEY}"
)

state = {
    "last_description": None,
    "updated_at":       None,
    "total_queries":    0,
}
history = collections.deque(maxlen=20)


def call_gemini_vision(image_b64, prompt="请用中文描述这张图片的内容"):
    """调用 Gemini Vision API"""
    if not GEMINI_API_KEY:
        return "[错误] 未设置 GEMINI_API_KEY 环境变量"

    payload = json.dumps({
        "contents": [{
            "parts": [
                {"text": prompt},
                {"inline_data": {"mime_type": "image/jpeg", "data": image_b64}},
            ]
        }],
        "generationConfig": {"maxOutputTokens": 256, "temperature": 0.4},
    }).encode("utf-8")

    req = urllib.request.Request(
        GEMINI_URL,
        data=payload,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    try:
        with urllib.request.urlopen(req, timeout=20) as resp:
            data = json.loads(resp.read().decode("utf-8"))
            return data["candidates"][0]["content"]["parts"][0]["text"].strip()
    except urllib.error.HTTPError as e:
        return f"[API 错误] HTTP {e.code}"
    except Exception as e:
        return f"[错误] {e}"


def analyze_image(image_b64, prompt="请用中文描述这张图片的内容"):
    """
    MCU 发送图像分析请求：
      Bridge.call("analyze_image", imageB64, prompt)
    """
    now  = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    b64  = str(image_b64).strip()
    prmt = str(prompt).strip()

    print(f"[{now}] 收到图像 ({len(b64)} bytes B64), 提示: {prmt[:40]}", flush=True)

    description = call_gemini_vision(b64, prmt)

    with lock:
        state["last_description"] = description
        state["updated_at"]       = now
        state["total_queries"]   += 1
        history.appendleft({
            "description": description,
            "prompt":      prmt,
            "time":        now,
        })

    print(f"[{now}] 描述: {description[:80]}", flush=True)
    return description


Bridge.provide("analyze_image", analyze_image)


def api_state(_req=None):
    with lock:
        return dict(state)


def api_history(_req=None):
    with lock:
        return {"history": list(history)}


web.expose_api("GET", "/api/state",   api_state)
web.expose_api("GET", "/api/history", api_history)

App.run()