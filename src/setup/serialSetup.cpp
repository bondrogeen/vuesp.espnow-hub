#include "./serialSetup.h"

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

#include "../eeprom/eeprom.h"
#include "../init/init.h"

static String serialLine;

static void printWifiScan() {
  Serial.println(F("@VUESP:SCAN:BEGIN"));

  auto oldMode = WiFi.getMode();
  WiFi.mode(WIFI_AP_STA);

  int count = WiFi.scanNetworks();
  if (count < 0) {
    Serial.println(F("@VUESP:SCAN:ERROR"));
  } else {
    for (int i = 0; i < count; i++) {
      String ssid = WiFi.SSID(i);
      ssid.replace("\t", " ");
      ssid.replace("\r", " ");
      ssid.replace("\n", " ");
      int secure = WiFi.encryptionType(i) != 0 ? 1 : 0;
      Serial.printf("@VUESP:WIFI:%s\t%d\t%d\n", ssid.c_str(), WiFi.RSSI(i), secure);
    }
  }

  WiFi.scanDelete();
  WiFi.mode(oldMode);
  Serial.println(F("@VUESP:SCAN:END"));
}

static void setWifi(const String& command) {
  int first = command.indexOf('\t');
  int second = first >= 0 ? command.indexOf('\t', first + 1) : -1;
  String ssid = first >= 0 ? command.substring(first + 1, second < 0 ? command.length() : second) : String();
  String pass = second >= 0 ? command.substring(second + 1) : String();

  ssid.trim();
  if (!ssid.length() || ssid.length() > 31 || pass.length() > 31) {
    Serial.println(F("@VUESP:SET:ERROR"));
    return;
  }

  memset(settings.wifiSsid, 0, sizeof(settings.wifiSsid));
  memset(settings.wifiPass, 0, sizeof(settings.wifiPass));
  strncpy(settings.wifiSsid, ssid.c_str(), sizeof(settings.wifiSsid) - 1);
  strncpy(settings.wifiPass, pass.c_str(), sizeof(settings.wifiPass) - 1);
  settings.wifiMode = WIFI_STA;
  settings.wifiDhcp = 1;

  if (!saveSettings(settings)) {
    Serial.println(F("@VUESP:SET:ERROR"));
    return;
  }

  Serial.println(F("@VUESP:SET:OK"));
  delay(250);
  reboot();
}

static void handleSerialLine(const String& raw) {
  String command = raw;
  command.trim();
  if (!command.startsWith("VUESP:")) return;

  if (command == "VUESP:PING") {
    Serial.println(F("@VUESP:PONG"));
  } else if (command == "VUESP:SCAN") {
    printWifiScan();
  } else if (command.startsWith("VUESP:SET\t")) {
    setWifi(command);
  }
}

void loopSerialSetup() {
  while (Serial.available() > 0) {
    char value = static_cast<char>(Serial.read());
    if (value == '\n' || value == '\r') {
      if (serialLine.length()) {
        handleSerialLine(serialLine);
        serialLine = "";
      }
    } else if (serialLine.length() < 200) {
      serialLine += value;
    }
  }
}
