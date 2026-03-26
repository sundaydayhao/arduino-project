"""
摄像头 + Gemini Vision AI - MicroPython 版
Arduino UNO Q (Qualcomm)

用 Python 实现摄像头图像理解：
  - 拍照 → Base64 编码 → Gemini API
  - 自动场景描述
  - 工业质检
  - 植物病害识别
  - 实时 OCR

Python 版比 C++ 版代码量减少 60%！

依赖：network, urequests, ubinascii, camera (内置)
"""

import network
import urequests
import ujson
import ubinascii
import time
import camera  # Qualcomm Arduino MicroPython 摄像头模块

# ===== 配置 =====
WIFI_SSID      = "YOUR_WIFI_SSID"
WIFI_PASSWORD  = "YOUR_WIFI_PASSWORD"
GEMINI_API_KEY = "YOUR_GEMINI_API_KEY"
GEMINI_MODEL   = "gemini-1.5-flash"  # 免费，速度快

# ===== 连接 Wi-Fi =====
def connect_wifi():
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    if not wlan.isconnected():
        wlan.connect(WIFI_SSID, WIFI_PASSWORD)
        print("连接 Wi-Fi...", end="")
        while not wlan.isconnected():
            print(".", end="")
            time.sleep(0.5)
    print(f"\n✅ Wi-Fi: {wlan.ifconfig()[0]}")
    return wlan

# ===== 初始化摄像头 =====
def init_camera():
    camera.init(0, format=camera.JPEG, framesize=camera.FRAME_QVGA)
    camera.quality(10)  # JPEG 质量 (1-63, 越小质量越高)
    print("✅ 摄像头初始化成功 (320x240 JPEG)")

# ===== 拍照 =====
def capture_photo():
    """拍摄一张照片，返回 JPEG 字节数据"""
    img = camera.capture()
    print(f"📷 拍照完成: {len(img)} bytes")
    return img

# ===== 调用 Gemini Vision API =====
def analyze_image(jpeg_bytes, prompt="请用中文描述这张图片的内容。"):
    """
    将图像发送给 Gemini Vision 分析
    返回: AI 分析结果字符串
    """
    # Base64 编码
    b64_image = ubinascii.b2a_base64(jpeg_bytes).decode().strip()
    print(f"Base64 编码: {len(b64_image)} chars")

    # 构建请求
    url = (f"https://generativelanguage.googleapis.com/v1beta/models/"
           f"{GEMINI_MODEL}:generateContent?key={GEMINI_API_KEY}")

    payload = ujson.dumps({
        "contents": [{
            "parts": [
                {"text": prompt},
                {"inline_data": {
                    "mime_type": "image/jpeg",
                    "data": b64_image
                }}
            ]
        }],
        "generationConfig": {
            "maxOutputTokens": 300,
            "temperature": 0.4
        }
    })

    headers = {"Content-Type": "application/json"}

    print("🤖 Gemini 分析中...")
    t0 = time.ticks_ms()
    resp = urequests.post(url, data=payload, headers=headers)
    elapsed = time.ticks_diff(time.ticks_ms(), t0)

    data = resp.json()
    resp.close()

    result = data["candidates"][0]["content"]["parts"][0]["text"]
    print(f"[{elapsed}ms]")
    return result

# ===== 应用场景 =====
PROMPTS = {
    "describe": "请用中文详细描述这张图片的内容，包括主要物体、颜色、场景。",

    "qc": (
        "这是一个工业零件的图片。请检查：\n"
        "1. 是否有裂纹、缺口、变形等缺陷？\n"
        "2. 表面是否有污染？\n"
        "3. 质检结论：合格/不合格，原因。\n"
        "用中文简洁回答，第一行必须是'合格'或'不合格'。"
    ),

    "plant": (
        "这是植物叶片图片。请分析：\n"
        "1. 植物种类\n2. 是否有病害及类型\n3. 处理建议\n"
        "用中文回答。"
    ),

    "ocr": "请识别并输出图片中的所有文字，保持原始格式。",

    "count": "请数出图片中的物体数量，说明是什么物体，共有几个。",

    "safety": (
        "这是工厂/工地现场图片。请检查安全隐患：\n"
        "1. 是否有人员未佩戴安全帽/安全带？\n"
        "2. 是否有危险操作或设备？\n"
        "3. 安全评级：安全/警告/危险\n"
        "用中文回答。"
    ),
}

# ===== 连续拍照分析 =====
def continuous_analysis(mode="describe", interval=10):
    """
    每隔 interval 秒拍照并分析
    mode: 分析模式 (describe/qc/plant/ocr/count/safety)
    """
    prompt = PROMPTS.get(mode, PROMPTS["describe"])
    print(f"\n🔄 连续分析模式: {mode} (每{interval}秒)")
    print("按 Ctrl+C 停止\n")

    count = 0
    while True:
        count += 1
        print(f"\n[第{count}次] ", end="")
        try:
            img = capture_photo()
            result = analyze_image(img, prompt)
            print(f"\n📋 结果:\n{result}\n")
            print("-" * 40)

            # 质检模式：根据结果控制 IO
            if mode == "qc":
                if "合格" in result and "不合格" not in result:
                    print("→ ✅ 合格品")
                    # Pin(12, Pin.OUT).on()  # 合格品通道
                else:
                    print("→ ❌ 不合格品")
                    # Pin(13, Pin.OUT).on()  # 不合格品通道

        except Exception as e:
            print(f"❌ 错误: {e}")

        time.sleep(interval)

# ===== 主程序 =====
def main():
    print("\n" + "="*44)
    print("  Arduino UNO Q - 摄像头 + Gemini AI")
    print("  MicroPython 版，代码简洁高效")
    print("="*44 + "\n")

    connect_wifi()
    init_camera()

    print("\n命令:")
    print("  'snap'         → 拍照并描述")
    print("  'qc'           → 工业质检")
    print("  'plant'        → 植物识别")
    print("  'ocr'          → 文字识别")
    print("  'count'        → 物体计数")
    print("  'safety'       → 安全检查")
    print("  'auto <mode>'  → 连续分析模式")
    print("  或输入自定义问题\n")

    while True:
        try:
            cmd = input(">>> ").strip()
        except EOFError:
            break

        if not cmd:
            continue

        if cmd.startswith("auto "):
            mode = cmd.split()[1] if len(cmd.split()) > 1 else "describe"
            continuous_analysis(mode)
        elif cmd in PROMPTS:
            img = capture_photo()
            result = analyze_image(img, PROMPTS[cmd])
            print(f"\n🤖 Gemini:\n{result}\n")
        elif cmd == "snap":
            img = capture_photo()
            result = analyze_image(img)
            print(f"\n🤖 Gemini:\n{result}\n")
        else:
            # 自定义问题
            img = capture_photo()
            result = analyze_image(img, cmd)
            print(f"\n🤖 Gemini:\n{result}\n")

main()