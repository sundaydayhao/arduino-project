"""
ChatGPT API 对话助手 - Linux MPU 端
Arduino UNO Q (Qualcomm)

双脑架构：
  MCU (Cortex-M33)  → 按键/语音触发，发送对话请求，显示回复
  Linux MPU (本文件) → 调用 OpenAI API，管理对话历史，返回回复

Linux MPU 拥有完整网络栈，适合调用云端 LLM API。
MCU 负责人机交互（按键、OLED 显示、TTS 播报）。
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

print("ChatGPT 对话助手 MPU 端启动", flush=True)

web  = WebUI()
lock = threading.Lock()

# ===== OpenAI 配置 =====
OPENAI_API_KEY = os.environ.get("OPENAI_API_KEY", "")
OPENAI_MODEL   = os.environ.get("OPENAI_MODEL", "gpt-4o-mini")
SYSTEM_PROMPT  = "你是 Arduino UNO Q 开发板上的智能助手，简洁友好地回答问题。"
MAX_HISTORY    = 10  # 保留最近 N 轮对话

# 对话历史（含 system prompt）
conversation = [{"role": "system", "content": SYSTEM_PROMPT}]
chat_log     = collections.deque(maxlen=50)

state = {
    "last_question": None,
    "last_answer":   None,
    "updated_at":    None,
    "total_queries": 0,
    "model":         OPENAI_MODEL,
}


def call_openai(messages):
    """调用 OpenAI Chat Completions API"""
    if not OPENAI_API_KEY:
        return "[错误] 未设置 OPENAI_API_KEY 环境变量"

    payload = json.dumps({
        "model":    OPENAI_MODEL,
        "messages": messages,
        "max_tokens": 200,
        "temperature": 0.7,
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
    except urllib.error.HTTPError as e:
        return f"[API 错误] HTTP {e.code}: {e.read().decode()}"
    except Exception as e:
        return f"[错误] {e}"


def chat(question):
    """
    MCU 发送对话请求：
      Bridge.call("chat", question)
    返回值通过 Bridge 回传给 MCU。
    """
    now = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    q   = str(question).strip()
    print(f"[{now}] 问: {q}", flush=True)

    with lock:
        conversation.append({"role": "user", "content": q})
        # 保持历史长度（system + MAX_HISTORY*2 条）
        if len(conversation) > 1 + MAX_HISTORY * 2:
            conversation[1:3] = []  # 删除最旧一轮

    answer = call_openai(list(conversation))

    with lock:
        conversation.append({"role": "assistant", "content": answer})
        state["last_question"] = q
        state["last_answer"]   = answer
        state["updated_at"]    = now
        state["total_queries"] += 1
        chat_log.appendleft({"q": q, "a": answer, "time": now})

    print(f"[{now}] 答: {answer[:80]}...", flush=True)
    return answer


Bridge.provide("chat", chat)


def api_state(_req=None):
    with lock:
        return dict(state)


def api_history(_req=None):
    with lock:
        return {"history": list(chat_log)}


web.expose_api("GET", "/api/state",   api_state)
web.expose_api("GET", "/api/history", api_history)

App.run()