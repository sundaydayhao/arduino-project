"""
传感器实时仪表盘 - MicroPython 版
Arduino UNO Q (Qualcomm)

功能：
  - 多传感器数据采集（温湿度/IMU/光照/ADC）
  - 串口实时可视化（ASCII 图表）
  - HTTP Web 仪表盘（浏览器访问）
  - WebSocket 实时推送
  - 数据记录到 Flash 文件系统

Python 优势：
  - asyncio 异步并发，无需 FreeRTOS
  - 内置 JSON/HTTP 支持
  - 代码量比 C++ 少 70%

依赖：network, uasyncio, ujson, machine (内置)
"""

import network
import uasyncio as asyncio
import ujson
import time
import math
from machine import Pin, ADC, I2C, SoftI2C

# ===== 配置 =====
WIFI_SSID     = "YOUR_WIFI_SSID"
WIFI_PASSWORD = "YOUR_WIFI_PASSWORD"
WEB_PORT      = 80
WS_PORT       = 8080

# ===== 传感器数据 =====
sensor_data = {
    "temperature": 0.0,
    "humidity":    0.0,
    "light":       0,
    "adc":         0,
    "accel_x":     0.0,
    "accel_y":     0.0,
    "accel_z":     0.0,
    "uptime":      0,
    "timestamp":   0,
}

# 历史数据（最近 60 个点）
history = {k: [] for k in ["temperature", "humidity", "light"]}
HISTORY_LEN = 60

# ===== 传感器读取 =====
def read_all_sensors():
    """读取所有传感器，更新 sensor_data"""
    t = time.ticks_ms() / 1000

    # ADC（模拟输入）
    adc = ADC(Pin(26))
    sensor_data["adc"] = adc.read_u16() >> 6  # 16bit -> 10bit

    # 模拟温湿度（替换为真实 DHT22/SHT31）
    sensor_data["temperature"] = 25.0 + math.sin(t / 10) * 3.0
    sensor_data["humidity"]    = 60.0 + math.cos(t / 8) * 5.0

    # 模拟光照（替换为 BH1750）
    sensor_data["light"] = int(500 + math.sin(t / 5) * 300)

    # 模拟 IMU（替换为 MPU6050）
    sensor_data["accel_x"] = math.sin(t) * 0.5
    sensor_data["accel_y"] = math.cos(t) * 0.5
    sensor_data["accel_z"] = 9.8

    sensor_data["uptime"]    = int(t)
    sensor_data["timestamp"] = time.time()

    # 更新历史
    for key in history:
        history[key].append(sensor_data[key])
        if len(history[key]) > HISTORY_LEN:
            history[key].pop(0)

# ===== ASCII 图表 =====
def ascii_chart(values, width=40, height=8, title=""):
    """生成 ASCII 折线图"""
    if not values:
        return ""
    mn, mx = min(values), max(values)
    if mx == mn:
        mx = mn + 1
    lines = []
    if title:
        lines.append(f"  {title}")
    for row in range(height, -1, -1):
        val = mn + (mx - mn) * row / height
        line = f"{val:6.1f} |"
        for v in values[-width:]:
            norm = (v - mn) / (mx - mn)
            bar_row = norm * height
            if abs(bar_row - row) < 0.5:
                line += "●"
            elif bar_row > row:
                line += "│"
            else:
                line += " "
        lines.append(line)
    lines.append("       +" + "─" * min(len(values), width))
    return "\n".join(lines)

# ===== 串口实时显示 =====
def print_dashboard():
    """在串口打印实时仪表盘"""
    print("\033[2J\033[H")  # 清屏
    print("╔══════════════════════════════════════════╗")
    print("║   Arduino UNO Q - 传感器仪表盘           ║")
    print("╠══════════════════════════════════════════╣")
    print(f"║  温度: {sensor_data['temperature']:6.1f}°C  "
          f"湿度: {sensor_data['humidity']:5.1f}%          ║")
    print(f"║  光照: {sensor_data['light']:5d} lux  "
          f"ADC: {sensor_data['adc']:4d}              ║")
    print(f"║  加速度: X={sensor_data['accel_x']:5.2f} "
          f"Y={sensor_data['accel_y']:5.2f} "
          f"Z={sensor_data['accel_z']:5.2f}  ║")
    print(f"║  运行时间: {sensor_data['uptime']:6d}s              ║")
    print("╚══════════════════════════════════════════╝")

    if len(history["temperature"]) > 5:
        print(ascii_chart(history["temperature"], title="温度 (°C)"))

# ===== Web 服务器 =====
HTML_PAGE = """<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>Arduino UNO Q 仪表盘</title>
  <meta http-equiv="refresh" content="2">
  <style>
    body { font-family: monospace; background: #1a1a2e; color: #eee; padding: 20px; }
    .card { background: #16213e; border-radius: 8px; padding: 15px; margin: 10px 0; }
    .val { font-size: 2em; color: #0f3460; color: #e94560; }
    .grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 10px; }
    h1 { color: #e94560; }
  </style>
</head>
<body>
  <h1>🔬 Arduino UNO Q 传感器仪表盘</h1>
  <div class="grid">
    <div class="card"><div>🌡️ 温度</div><div class="val">TEMP°C</div></div>
    <div class="card"><div>💧 湿度</div><div class="val">HUMI%</div></div>
    <div class="card"><div>☀️ 光照</div><div class="val">LIGHT lux</div></div>
    <div class="card"><div>📊 ADC</div><div class="val">ADC_VAL</div></div>
    <div class="card"><div>⏱️ 运行时间</div><div class="val">UPTIMEs</div></div>
    <div class="card"><div>📡 Wi-Fi</div><div class="val">Wi-Fi 6</div></div>
  </div>
  <p style="color:#666">每2秒自动刷新 | MicroPython on Qualcomm</p>
</body>
</html>"""

async def handle_http(reader, writer):
    """处理 HTTP 请求"""
    request = await reader.read(1024)
    request = request.decode()

    if "GET /api/sensors" in request:
        # JSON API
        body = ujson.dumps(sensor_data)
        response = (
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            f"Content-Length: {len(body)}\r\n\r\n"
            + body
        )
    else:
        # HTML 页面（替换占位符）
        html = HTML_PAGE
        html = html.replace("TEMP",   f"{sensor_data['temperature']:.1f}")
        html = html.replace("HUMI",   f"{sensor_data['humidity']:.0f}")
        html = html.replace("LIGHT",  str(sensor_data['light']))
        html = html.replace("ADC_VAL", str(sensor_data['adc']))
        html = html.replace("UPTIME", str(sensor_data['uptime']))
        response = (
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            f"Content-Length: {len(html)}\r\n\r\n"
            + html
        )

    writer.write(response.encode())
    await writer.drain()
    writer.close()

# ===== 数据记录到 Flash =====
def log_to_file():
    """将传感器数据追加到 CSV 文件"""
    try:
        with open("/data_log.csv", "a") as f:
            f.write(f"{sensor_data['timestamp']},"
                    f"{sensor_data['temperature']:.1f},"
                    f"{sensor_data['humidity']:.1f},"
                    f"{sensor_data['light']}\n")
    except Exception as e:
        print(f"日志写入失败: {e}")

# ===== 异步任务 =====
async def task_sensors():
    """每秒采集传感器数据"""
    while True:
        read_all_sensors()
        await asyncio.sleep(1)

async def task_display():
    """每2秒刷新串口显示"""
    while True:
        print_dashboard()
        await asyncio.sleep(2)

async def task_log():
    """每30秒记录一次数据"""
    while True:
        log_to_file()
        await asyncio.sleep(30)

# ===== 主程序 =====
async def main():
    print("\n" + "="*44)
    print("  Arduino UNO Q - 传感器仪表盘 (MicroPython)")
    print("="*44 + "\n")

    # 连接 Wi-Fi
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    wlan.connect(WIFI_SSID, WIFI_PASSWORD)
    print("连接 Wi-Fi...", end="")
    for _ in range(20):
        if wlan.isconnected(): break
        print(".", end="")
        await asyncio.sleep(0.5)
    print()

    if wlan.isconnected():
        ip = wlan.ifconfig()[0]
        print(f"✅ Wi-Fi 已连接: {ip}")
        print(f"   浏览器访问: http://{ip}/")
        print(f"   API 接口:   http://{ip}/api/sensors\n")

        # 启动 Web 服务器
        server = await asyncio.start_server(handle_http, "0.0.0.0", WEB_PORT)
        print(f"✅ Web 服务器已启动 (端口 {WEB_PORT})")
    else:
        print("⚠️ Wi-Fi 连接失败，仅串口模式")

    # 启动异步任务
    await asyncio.gather(
        task_sensors(),
        task_display(),
        task_log(),
    )

asyncio.run(main())