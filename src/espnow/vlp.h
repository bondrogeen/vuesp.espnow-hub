#pragma once

#include <Arduino.h>

// VLP/1 — compact text protocol for ESP-NOW frames.
// Keep frames below 200 bytes; ESP-NOW payload limit is 250 bytes.

struct VlpFrame {
  String type;
  String src;
  String dst;
  String kind;
  String unit;
  String mid;
  String action;
  String fw;
  String board;
  String caps;
  uint32_t seq = 0;
  uint32_t uptime = 0;
  uint32_t channel = 0;
  int gpio = -1;
  float value = 0;
  bool hasValue = false;
  bool ok = false;
};

inline String vlpDeviceId() {
  uint64_t mac = ESP.getEfuseMac();
  char buf[9];
  snprintf(buf, sizeof(buf), "%08X", (uint32_t)(mac & 0xFFFFFFFF));
  return String(buf);
}

inline String vlpBase(const String& type, const String& src, uint32_t seq, uint32_t uptime) {
  return String("VLP1|") + type + "|src=" + src + "|seq=" + String(seq) + "|up=" + String(uptime);
}

inline String vlpHello(const String& src, uint32_t seq, uint32_t uptime, const String& fw, const String& board, const String& caps, uint32_t channel) {
  return vlpBase("HELLO", src, seq, uptime) + "|fw=" + fw + "|board=" + board + "|caps=" + caps + "|ch=" + String(channel);
}

inline String vlpHelloAck(const String& src, uint32_t channel) {
  return String("VLP1|HELLO_ACK|src=") + src + "|ch=" + String(channel);
}

inline String vlpEvent(const String& src, uint32_t seq, uint32_t uptime, const String& kind, int gpio, float value, const String& unit = "") {
  String frame = vlpBase("EVT", src, seq, uptime) + "|k=" + kind;
  if (gpio >= 0) frame += "|gpio=" + String(gpio);
  frame += "|val=" + String(value, 2);
  if (unit.length()) frame += "|unit=" + unit;
  return frame;
}

inline String vlpCommand(const String& dst, const String& mid, const String& action, int gpio, float value) {
  String frame = String("VLP1|CMD|dst=") + dst + "|mid=" + mid + "|action=" + action;
  if (gpio >= 0) frame += "|gpio=" + String(gpio);
  frame += "|val=" + String(value, 2);
  return frame;
}

inline String vlpAck(const String& src, const String& mid, bool ok) {
  return String("VLP1|ACK|src=") + src + "|mid=" + mid + "|ok=" + (ok ? "1" : "0");
}

inline String vlpHeartbeat(const String& src, uint32_t seq, uint32_t uptime) {
  return vlpBase("HB", src, seq, uptime);
}

inline bool vlpParse(const uint8_t* data, size_t length, VlpFrame& out) {
  if (!data || length < 8 || length > 220) return false;

  String text;
  text.reserve(length + 1);
  for (size_t i = 0; i < length; i++) text += (char)data[i];

  int first = text.indexOf('|');
  if (first < 0 || text.substring(0, first) != "VLP1") return false;

  int start = first + 1;
  int end = text.indexOf('|', start);
  out.type = end < 0 ? text.substring(start) : text.substring(start, end);

  while (end >= 0) {
    start = end + 1;
    end = text.indexOf('|', start);
    String part = end < 0 ? text.substring(start) : text.substring(start, end);
    int eq = part.indexOf('=');
    if (eq <= 0) continue;

    String key = part.substring(0, eq);
    String value = part.substring(eq + 1);

    if (key == "src") out.src = value;
    else if (key == "dst") out.dst = value;
    else if (key == "seq") out.seq = value.toInt();
    else if (key == "up") out.uptime = value.toInt();
    else if (key == "k") out.kind = value;
    else if (key == "gpio") out.gpio = value.toInt();
    else if (key == "val") {
      out.value = value.toFloat();
      out.hasValue = true;
    } else if (key == "unit") out.unit = value;
    else if (key == "mid") out.mid = value;
    else if (key == "action") out.action = value;
    else if (key == "ok") out.ok = value == "1";
    else if (key == "ch") out.channel = value.toInt();
    else if (key == "fw") out.fw = value;
    else if (key == "board") out.board = value;
    else if (key == "caps") out.caps = value;
  }

  return out.type.length() > 0;
}
