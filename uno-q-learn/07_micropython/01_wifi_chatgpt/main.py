"""
ChatGPT API 集成 - MicroPython 版
Arduino UNO Q (Qualcomm)

MicroPython 相比 C++ 的优势：
  - 代码更简洁，开发效率高 3-5x
  - 无需编译，直接运行
  - 丰富的 Python 生态
  - 交互式 REPL 调试

运行方式：
  1. 安装 Arduino Lab for MicroPython
  2. 连接开发板，上传 main.py
  3. 或直接在 REPL 中粘贴运行

依赖：network, urequests, ujson (MicroPython 内置)
"""

import network
import urequests
import ujson
import time
import sys

# ===== 配置 =====
WIFI_SSID      = "YOUR_WIFI6_SSID"
WIFI_PASSWORD  = "YOUR_WIFI_PASSWORD"
OPENAI_API_KEY = "sk-YOUR_OPENAI_API_KEY"
OPENAI_MODEL   = "gpt-4o-mini"

SYSTEM_PROMPT = (
    "你是运行在 Arduino UNO Q (Qualcomm MicroPython) 上的 AI 助手。"
    "回答简洁，适合串口终端显示。"
    "如需控制硬件，用 [CMD:指令] 格式，如 [CMD:LED_ON]。"
)

# ===== 连接 Wi-Fi 6 =====
def connect_wifi():
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    if wlan.isconnected():
        print(f"✅ 已连接: {wlan.ifconfig()[0]}")
        return wlan
    print(f"连接 Wi-Fi 6: {WIFI_SSID}", end="")
    wlan.connect(WIFI_SSID, WIFI_PASSWORD)
    for _ in range(20):
        if wlan.isconnected():
            break
        print(".", end="")
        time.sleep(0.5)
    print()
    if wlan.isconnected():
        ip, mask, gw, dns = wlan.ifconfig()
        print(f"✅ Wi-Fi 6 已连接!")
        print(f"   IP: {ip}  网关: {gw}")
        return wlan
    raise RuntimeError("Wi-Fi 连接失败")

# ===== 对话历史 =====
chat_history = []

def add_history(role, content):
    chat_history.append({"role": role, "content": content})
    # 保留最近 8 条
    if len(chat_history) > 8:
        chat_history.pop(0)

# ===== 调用 ChatGPT API =====
def chat(user_message, sensor_data=None):
    """调用 ChatGPT，返回回复文本"""
    if sensor_data:
        user_message += f"\n[传感器] {sensor_data}"

    add_history("user", user_message)

    messages = [{"role": "system", "content": SYSTEM_PROMPT}] + chat_history

    payload = ujson.dumps({
        "model": OPENAI_MODEL,
        "messages": messages,
        "max_tokens": 400,
        "temperature": 0.7,
    })

    headers = {
        "Content-Type": "application/json",
        "Authorization": f"Bearer {OPENAI_API_KEY}",
    }

    t0 = time.ticks_ms()
    resp = urequests.post(
        "https://api.openai.com/v1/chat/completions",
        data=payload,
        headers=headers,
    )
    elapsed = time.ticks_diff(time.ticks_ms(), t0)

    data = resp.json()
    resp.close()

    reply = data["choices"][0]["message"]["content"]
    tokens = data["usage"]["total_tokens"]
    add_history("assistant", reply)

    print(f"[{elapsed}ms, {tokens} tokens]")
    return reply

# ===== 解析硬件控制指令 =====
def execute_commands(reply):
    """解析 GPT 回复中的 [CMD:xxx] 指令"""
    import re
    cmds = re.findall(r'\[CMD:(\w+)\]', reply)
    for cmd in cmds:
        print(f"\n[执行] {cmd}")
        if cmd == "LED_ON":
            # from machine import Pin
            # Pin("LED", Pin.OUT).on()
            print("  → LED 已开")
        elif cmd == "LED_OFF":
            print("  → LED 已关")
        elif cmd == "REBOOT":
            import machine
            machine.reset()

# ===== 读取传感器（示例）=====
def read_sensors():
    """读取传感器数据，返回字符串"""
    import machine
    adc = machine.ADC(0)
    val = adc.read_u16()
    uptime = time.ticks_ms() // 1000
    return f"ADC:{val} 运行:{uptime}s"

# ===== 主程序 =====
def main():
    print("\n" + "="*42)
    print("  Arduino UNO Q - ChatGPT (MicroPython)")
    print("  Python 版，代码更简洁！")
    print("="*42 + "\n")

    connect_wifi()

    print("\n命令:")
    print("  直接输入问题 → GPT 回答")
    print("  's:问题'     → 附加传感器数据")
    print("  'clear'      → 清除历史")
    print("  'sensor'     → 查看传感器\n")

    while True:
        try:
            line = input("👤 你: ").strip()
        except EOFError:
            break
        if not line:
            continue

        if line == "clear":
            chat_history.clear()
            print("✅ 历史已清除\n")
            continue
        if line == "sensor":
            print(read_sensors())
            continue

        attach_sensor = line.startswith("s:")
        question = line[2:] if attach_sensor else line
        sensor = read_sensors() if attach_sensor else None

        print("🤖 GPT 思考中...")
        try:
            reply = chat(question, sensor)
            print(f"\n🤖 GPT:\n{reply}\n")
            execute_commands(reply)
        except Exception as e:
            print(f"❌ 错误: {e}")

main()