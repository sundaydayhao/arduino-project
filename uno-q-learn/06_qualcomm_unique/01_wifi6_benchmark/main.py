"""
WiFi 6 基准测试 - Linux MPU 端
Arduino UNO Q (Qualcomm)

双脑架构：
  MCU (Cortex-M33)  → WiFi 6 连接，吞吐量测试，延迟测量
  Linux MPU (本文件) → 接收测试结果，统计分析，REST API，Web 报告

Qualcomm QCS6490 内置 WiFi 6 (802.11ax)，支持 2.4GHz/5GHz 双频，
理论最大吞吐量 1.8Gbps，本例程测试实际性能。
"""

import datetime
import threading
import collections
from arduino.app_utils import App, Bridge
from arduino.app_bricks.web_ui import WebUI

print("WiFi 6 基准测试 MPU 端启动", flush=True)

web  = WebUI()
lock = threading.Lock()

state = {
    "ssid":           None,
    "band":           None,
    "channel":        None,
    "rssi":           None,
    "tx_mbps":        None,
    "rx_mbps":        None,
    "latency_ms":     None,
    "jitter_ms":      None,
    "packet_loss":    None,
    "wifi_standard":  None,
    "updated_at":     None,
    "test_count":     0,
}

# 历史测试结果
history = collections.deque(maxlen=100)

# 统计
stats = {
    "max_tx_mbps":  0.0,
    "max_rx_mbps":  0.0,
    "min_latency":  9999.0,
    "avg_tx_mbps":  0.0,
    "avg_rx_mbps":  0.0,
    "avg_latency":  0.0,
}


def update_benchmark(ssid, band, channel, rssi,
                     tx_mbps, rx_mbps, latency_ms, jitter_ms,
                     packet_loss, wifi_standard):
    """
    MCU 每次测试后调用：
      Bridge.call("update_benchmark", ssid, band, ch, rssi,
                  txMbps, rxMbps, latMs, jitterMs, pktLoss, standard)
    """
    now = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    tx  = float(tx_mbps)
    rx  = float(rx_mbps)
    lat = float(latency_ms)

    with lock:
        state.update({
            "ssid":          str(ssid).strip(),
            "band":          str(band).strip(),
            "channel":       int(float(channel)),
            "rssi":          int(float(rssi)),
            "tx_mbps":       round(tx, 2),
            "rx_mbps":       round(rx, 2),
            "latency_ms":    round(lat, 2),
            "jitter_ms":     round(float(jitter_ms), 2),
            "packet_loss":   round(float(packet_loss), 2),
            "wifi_standard": str(wifi_standard).strip(),
            "updated_at":    now,
        })
        state["test_count"] += 1
        n = state["test_count"]

        # 更新统计
        stats["max_tx_mbps"] = max(stats["max_tx_mbps"], tx)
        stats["max_rx_mbps"] = max(stats["max_rx_mbps"], rx)
        stats["min_latency"] = min(stats["min_latency"], lat)
        stats["avg_tx_mbps"] = round((stats["avg_tx_mbps"] * (n-1) + tx) / n, 2)
        stats["avg_rx_mbps"] = round((stats["avg_rx_mbps"] * (n-1) + rx) / n, 2)
        stats["avg_latency"] = round((stats["avg_latency"] * (n-1) + lat) / n, 2)

        history.appendleft({
            "tx_mbps": round(tx, 2), "rx_mbps": round(rx, 2),
            "latency_ms": round(lat, 2), "rssi": int(float(rssi)),
            "band": str(band).strip(), "time": now,
        })

    print(f"[{now}] WiFi6 TX:{tx:.1f}Mbps RX:{rx:.1f}Mbps "
          f"延迟:{lat:.1f}ms RSSI:{int(float(rssi))}dBm "
          f"频段:{band} 标准:{wifi_standard}", flush=True)


Bridge.provide("update_benchmark", update_benchmark)


def api_state(_req=None):
    with lock:
        return {**state, "stats": dict(stats)}


def api_history(_req=None):
    with lock:
        return {"history": list(history)}


web.expose_api("GET", "/api/state",   api_state)
web.expose_api("GET", "/api/history", api_history)

App.run()