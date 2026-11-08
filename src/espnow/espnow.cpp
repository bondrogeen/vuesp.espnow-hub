#include "./espnow.h"
#include "./vlp.h"

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <HTTPClient.h>

#ifndef ESPNOW_CHANNEL
#define ESPNOW_CHANNEL 6
#endif
#ifndef HUB_SERVER_URL
#define HUB_SERVER_URL "http://192.168.1.10:3005/api/v1/espnow/event"
#endif

struct HubPeer {
  uint8_t mac[6];
  String id;
  uint32_t lastSeen;
};

struct PendingEvent {
  String json;
  uint8_t attempts;
};

static HubPeer peers[20];
static uint8_t peerCount = 0;
static PendingEvent queue[16];
static uint8_t queueCount = 0;
static String hubId;
static uint32_t lastForward = 0;
static uint32_t lastAnnounce = 0;

static const uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static void addPeer(const uint8_t* mac, const String& id = "") {
  for (uint8_t i = 0; i < peerCount; i++) {
    if (memcmp(peers[i].mac, mac, 6) == 0) {
      peers[i].lastSeen = millis();
      if (id.length()) peers[i].id = id;
      return;
    }
  }
  if (peerCount >= 20) return;

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = 0;
  peer.ifidx = WIFI_IF_AP;
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.printf("ESP-NOW: cannot add peer %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return;
  }

  memcpy(peers[peerCount].mac, mac, 6);
  peers[peerCount].id = id;
  peers[peerCount].lastSeen = millis();
  peerCount++;
  Serial.printf("ESP-NOW: peer %s joined\n", id.length() ? id.c_str() : "unknown");
}

static bool sendText(const uint8_t* mac, const String& text) {
  addPeer(mac);
  return esp_now_send(mac, (const uint8_t*)text.c_str(), text.length()) == ESP_OK;
}

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
static void onRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  const uint8_t* mac = info->src_addr;
#else
static void onRecv(const uint8_t* mac, const uint8_t* data, int len) {
#endif
  VlpFrame frame;
  if (!vlpParse(data, len, frame)) {
    Serial.println("ESP-NOW: invalid VLP frame");
    return;
  }

  addPeer(mac, frame.src);
  Serial.printf("VLP %s from %s\n", frame.type.c_str(), frame.src.c_str());

  if (frame.type == "HELLO") {
    sendText(mac, vlpHelloAck(hubId, ESPNOW_CHANNEL));
    return;
  }

  if (frame.type == "EVT") {
    String json = String("{\"v\":1,\"hub\":\"") + hubId + "\",\"src\":\"" + frame.src + "\",\"seq\":" + String(frame.seq) + ",\"up\":" + String(frame.uptime) + ",\"name\":\"" + frame.kind + "\"";
    if (frame.gpio >= 0) json += ",\"gpio\":" + String(frame.gpio);
    if (frame.hasValue) json += ",\"value\":" + String(frame.value, 2);
    if (frame.unit.length()) json += ",\"unit\":\"" + frame.unit + "\"";
    json += "}";

    if (queueCount >= 16) {
      for (uint8_t i = 1; i < queueCount; i++) queue[i - 1] = queue[i];
      queueCount--;
    }
    queue[queueCount].json = json;
    queue[queueCount].attempts = 0;
    queueCount++;
    Serial.println(json);
  }
}

static void onSent(const uint8_t* mac, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) Serial.println("ESP-NOW: send failed");
}

static void forwardQueue(uint32_t now) {
  if (!queueCount || WiFi.status() != WL_CONNECTED) return;
  if (now - lastForward < 500) return;
  lastForward = now;

  HTTPClient http;
  http.setTimeout(1500);
  if (!http.begin(HUB_SERVER_URL)) return;
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(queue[0].json);
  http.end();

  if (code >= 200 && code < 300) {
    for (uint8_t i = 1; i < queueCount; i++) queue[i - 1] = queue[i];
    queueCount--;
    return;
  }

  queue[0].attempts++;
  if (queue[0].attempts >= 3) {
    Serial.println("ESP-NOW: drop event after 3 attempts");
    for (uint8_t i = 1; i < queueCount; i++) queue[i - 1] = queue[i];
    queueCount--;
  }
}

void espnowSetup() {
  hubId = vlpDeviceId();

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("Vuesp-Hub", "vuesp-hub", ESPNOW_CHANNEL);
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW: init failed");
    return;
  }

  esp_now_register_recv_cb(onRecv);
  esp_now_register_send_cb(onSent);
  addPeer(broadcastMac, "broadcast");

  Serial.printf("ESP-NOW hub %s, channel %d, MAC %s\n", hubId.c_str(), ESPNOW_CHANNEL, WiFi.macAddress().c_str());
  Serial.printf("Forward URL: %s\n", HUB_SERVER_URL);
}

void espnowLoop(uint32_t now) {
  forwardQueue(now);

  if (now - lastAnnounce > 5000) {
    lastAnnounce = now;
    // Broadcast heartbeat so clients can discover the hub even before pairing.
    String heartbeat = vlpHeartbeat(hubId, 0, now);
    esp_now_send(broadcastMac, (const uint8_t*)heartbeat.c_str(), heartbeat.length());
  }
}

const char* espnowHubId() {
  return hubId.c_str();
}
