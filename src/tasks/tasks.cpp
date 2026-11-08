#include "./tasks.h"

uint8_t tasks[KEY_END];

Scan scan = {KEY_SCAN, 0, 0, 0, 0, 0, ""};
char safePass[32];
Files files = {KEY_FILES, 0, 0, 0, 0, ""};
Port port = {KEY_PORT, 0, 0, 0};
Message message = {KEY_MESSAGE, MESSAGE_TYPE_LOG};

void onWsEventTasks(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task) {
  AwsFrameInfo* info = (AwsFrameInfo*)arg;
  tasks[task] = true;
  if (task == KEY_SETTINGS && info->len == sizeof(settings)) {
    memcpy(&settings, data, sizeof(settings));
    if (strcmp(settings.wifiPass, DEF_HIDE_PASS) == 0) {
      memset(settings.wifiPass, 0, sizeof(settings.wifiPass));
      strcpy(settings.wifiPass, safePass);
    }
    saveSettings(settings);
  }
  if (task == KEY_FILES && info->len == sizeof(files)) {
    memcpy(&files, data, sizeof(files));
  }
  if (task == KEY_PORT && info->len == sizeof(port)) {
    memcpy(&port, data, sizeof(port));
  }
  if (task == KEY_MESSAGE && info->len == sizeof(message)) {
    memcpy(&message, data, sizeof(message));
  }
  if (task == KEY_DISCOVERY && info->len == sizeof(discovery)) {
    memcpy(&discovery, data, sizeof(discovery));
  }
}

#if defined(ESP8266)
void getFile(char* name) {
  Dir dir = LittleFS.openDir(name);
  while (dir.next()) {
    memset(files.name, 0, sizeof(files.name));
    dir.fileName().toCharArray(files.name, 32);
    files.size = dir.fileSize();
    files.isFile = dir.isFile();
    files.isDir = dir.isDirectory();
    wsSend((uint8_t*)&files, sizeof(files));
  }
}

#elif defined(ESP32)

void getFile(char* name) {
  File root = LittleFS.open(name);
  File file = root.openNextFile();
  String str = "";
  while (file) {
    memset(files.name, 0, sizeof(files.name));
    strcpy(files.name, file.name());
    files.size = file.size();
    files.isFile = !file.isDirectory();
    files.isDir = file.isDirectory();
    wsSend((uint8_t*)&files, sizeof(files));
    file = root.openNextFile();
  }
}
#endif

void scanWiFi() {
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    scan.id = i;
    memset(scan.ssid, 0, sizeof(scan.ssid));
    WiFi.SSID(i).toCharArray(scan.ssid, 32);
    scan.channel = WiFi.channel(i);
    scan.rssi = WiFi.RSSI(i);
    scan.encryptionType = WiFi.encryptionType(i);
    // scan.isHidden = WiFi.isHidden(i);
    wsSend((uint8_t*)&scan, sizeof(scan));
  };
}

void sendLog(const char* text) {
  message.type = MESSAGE_TYPE_LOG;
  memset(message.text, 0, sizeof(message.text));
  strcpy(message.text, text);
  wsSendAll((uint8_t*)&message, sizeof(message));
}

void loopTask(uint32_t now) {
  if (tasks[KEY_SETTINGS]) {
    if (strcmp(settings.wifiPass, DEF_HIDE_PASS) != 0) {
      memset(safePass, 0, sizeof(safePass));
      strcpy(safePass, settings.wifiPass);
      memset(settings.wifiPass, 0, sizeof(settings.wifiPass));
      strcpy(settings.wifiPass, DEF_HIDE_PASS);
    }
    wsSend((uint8_t*)&settings, sizeof(settings));
    tasks[KEY_SETTINGS] = false;
  };
  if (tasks[KEY_INFO]) {
    getInfo(&infoFS);
    wsSend((uint8_t*)&infoFS, sizeof(infoFS));
    tasks[KEY_INFO] = false;
  }
  if (tasks[KEY_FILES]) {
    tasks[KEY_FILES] = false;
    getFile(files.name);
  }
  if (tasks[KEY_REBOOT]) reboot();

  if (tasks[KEY_SCAN]) {
    tasks[KEY_SCAN] = false;
    scanWiFi();
  }
  if (tasks[KEY_MESSAGE]) {
    tasks[KEY_MESSAGE] = false;
    eventRunner();
  }
  if (tasks[KEY_DISCOVERY]) {
    tasks[KEY_DISCOVERY] = false;
    commDiscovery();
  }
}