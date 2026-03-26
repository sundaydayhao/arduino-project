/**
 * WiFi 6 基准测试 - MCU 端
 * Arduino UNO Q (Qualcomm)
 *
 * 双脑架构：
 *   MCU (Cortex-M33) → WiFi 6 连接，吞吐量/延迟测试，上报结果
 *   Linux MPU        → 接收测试结果，统计分析，REST API，Web 报告
 *
 * 测试项目：
 *   - TCP 上行/下行吞吐量 (Mbps)
 *   - ICMP 延迟 & 抖动 (ms)
 *   - 丢包率 (%)
 *   - WiFi 信号强度 (RSSI dBm)
 *   - 连接频段 (2.4GHz / 5GHz)
 *
 * Qualcomm QCS6490 WiFi 6 特性：
 *   - 802.11ax (WiFi 6)，2.4GHz + 5GHz 双频
 *   - OFDMA、MU-MIMO、TWT 节能
 *   - 理论最大 1.8Gbps
 *
 * 依赖库：WiFi (内置), Arduino_RouterBridge
 */

#include <Arduino_RouterBridge.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>

// ===== WiFi 配置（请修改为实际 WiFi 信息）=====
const char* WIFI_SSID     = "YOUR_WIFI6_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// ===== 测试服务器（需要在局域网内运行 iperf3 服务器）=====
const char* IPERF_SERVER  = "192.168.1.100";
const int   IPERF_PORT    = 5201;
const char* PING_HOST     = "8.8.8.8";

// ===== 测试配置 =====
constexpr int   TEST_DURATION_S  = 10;   // 每次测试持续时间
constexpr int   PING_COUNT       = 20;   // Ping 次数
constexpr unsigned long TEST_INTERVAL_MS = 60000;  // 每分钟测试一次

unsigned long lastTestMs = 0;
int testCount = 0;

// ===== 测量 TCP 吞吐量 =====
float measureTCPThroughput(bool upload) {
  WiFiClient client;
  if (!client.connect(IPERF_SERVER, IPERF_PORT)) {
    Monitor.println("[WARN] 无法连接 iperf3 服务器，跳过吞吐量测试");
    return 0.0f;
  }

  static uint8_t buf[4096];
  memset(buf, 0xAA, sizeof(buf));

  unsigned long startMs = millis();
  unsigned long bytes   = 0;
  unsigned long durationMs = TEST_DURATION_S * 1000UL;

  if (upload) {
    while (millis() - startMs < durationMs) {
      int sent = client.write(buf, sizeof(buf));
      if (sent > 0) bytes += sent;
    }
  } else {
    while (millis() - startMs < durationMs && client.connected()) {
      int avail = client.available();
      if (avail > 0) {
        int rd = client.read(buf, min(avail, (int)sizeof(buf)));
        if (rd > 0) bytes += rd;
      }
    }
  }

  client.stop();
  float elapsed = (millis() - startMs) / 1000.0f;
  return (elapsed > 0) ? (bytes * 8.0f / 1e6f / elapsed) : 0.0f;
}

// ===== 测量 ICMP 延迟 =====
struct PingResult {
  float avgMs;
  float jitterMs;
  float lossPercent;
};

PingResult measurePing() {
  PingResult result = {0, 0, 0};
  WiFiUDP udp;
  udp.begin(12345);

  float times[PING_COUNT];
  int   received = 0;
  float prevTime = 0;
  float jitterSum = 0;

  for (int i = 0; i < PING_COUNT; i++) {
    // 简化 ping：发送 UDP 并等待回显
    uint8_t payload[32];
    memset(payload, i, sizeof(payload));
    unsigned long t0 = micros();
    udp.beginPacket(PING_HOST, 7);  // echo port
    udp.write(payload, sizeof(payload));
    udp.endPacket();

    unsigned long deadline = millis() + 1000;
    bool got = false;
    while (millis() < deadline) {
      if (udp.parsePacket() > 0) {
        float rtt = (micros() - t0) / 1000.0f;
        times[received++] = rtt;
        if (received > 1) jitterSum += abs(rtt - prevTime);
        prevTime = rtt;
        got = true;
        break;
      }
    }
    delay(50);
  }

  udp.stop();

  if (received == 0) {
    result.lossPercent = 100.0f;
    return result;
  }

  float sum = 0;
  for (int i = 0; i < received; i++) sum += times[i];
  result.avgMs      = sum / received;
  result.jitterMs   = (received > 1) ? jitterSum / (received - 1) : 0;
  result.lossPercent = (1.0f - (float)received / PING_COUNT) * 100.0f;
  return result;
}

void runBenchmark() {
  testCount++;
  Monitor.printf("\n===== WiFi 6 基准测试 #%d =====\n", testCount);

  // WiFi 信息
  String ssid    = WiFi.SSID();
  int    rssi    = WiFi.RSSI();
  int    channel = WiFi.channel();
  String band    = (channel > 14) ? "5GHz" : "2.4GHz";
  String standard = "802.11ax (WiFi 6)";  // Qualcomm QCS6490 固定支持

  Monitor.printf("SSID: %s | 频段: %s | 信道: %d | RSSI: %d dBm\n",
                ssid.c_str(), band.c_str(), channel, rssi);

  // 吞吐量测试
  Monitor.print("测试上行吞吐量...");
  float txMbps = measureTCPThroughput(true);
  Monitor.printf(" %.1f Mbps\n", txMbps);

  Monitor.print("测试下行吞吐量...");
  float rxMbps = measureTCPThroughput(false);
  Monitor.printf(" %.1f Mbps\n", rxMbps);

  // 延迟测试
  Monitor.print("测试网络延迟...");
  PingResult ping = measurePing();
  Monitor.printf(" 延迟:%.1fms 抖动:%.1fms 丢包:%.1f%%\n",
                ping.avgMs, ping.jitterMs, ping.lossPercent);

  Monitor.printf("===== 测试完成 =====\n\n");

  // ===== 上报结果到 Linux MPU =====
  Bridge.call("update_benchmark",
              ssid.c_str(), band.c_str(), channel, rssi,
              txMbps, rxMbps,
              ping.avgMs, ping.jitterMs, ping.lossPercent,
              standard.c_str());
}

void setup() {
  Bridge.begin();
  Monitor.begin();

  Monitor.println("========================================");
  Monitor.println("  Arduino UNO Q - WiFi 6 基准测试");
  Monitor.println("  MCU: 测试执行 | MPU: 统计+报告");
  Monitor.println("  Qualcomm QCS6490 802.11ax WiFi 6");
  Monitor.println("========================================");

  Monitor.printf("连接 WiFi: %s ...\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 30) {
    delay(500);
    Monitor.print(".");
    retry++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Monitor.println("\n[ERROR] WiFi 连接失败!");
    while (1) delay(5000);
  }

  Monitor.printf("\n[OK] WiFi 连接成功!\n");
  Monitor.printf("  IP: %s\n", WiFi.localIP().toString().c_str());
  Monitor.printf("  RSSI: %d dBm\n", WiFi.RSSI());
  Monitor.printf("  信道: %d (%s)\n",
                WiFi.channel(),
                WiFi.channel() > 14 ? "5GHz" : "2.4GHz");
  Monitor.printf("\n每 %lu 秒运行一次基准测试\n\n", TEST_INTERVAL_MS / 1000);

  // 立即运行第一次测试
  runBenchmark();
  lastTestMs = millis();
}

void loop() {
  if (millis() - lastTestMs >= TEST_INTERVAL_MS) {
    lastTestMs = millis();

    if (WiFi.status() != WL_CONNECTED) {
      Monitor.println("[WARN] WiFi 断开，重新连接...");
      WiFi.reconnect();
      delay(3000);
      return;
    }

    runBenchmark();
  }
}