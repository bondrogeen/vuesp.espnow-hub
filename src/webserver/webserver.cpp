#include "./webserver.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/esp");
// AsyncWebSocketClient *client;

Ping ping = {KEY_PING};
Progress progress = {KEY_PROGRESS, 5, 0, 0, 0};

uint32_t clientID = 0;
uint8_t hold = 255;
uint32_t lastTime = 0;
uint8_t progressSendCount = 0;
const uint8_t MAX_PROGRESS_SENDS = 10;

static AsyncAuthenticationMiddleware basicAuth;
static AsyncCorsMiddleware cors;

String status(uint8_t state) {
  return (state) ? "{\"state\":true}" : "{\"state\":false}";
}

void wsSend(uint8_t* message, size_t len) {
  if (message == nullptr || len == 0) return;
  if (clientID && ws.hasClient(clientID)) {
    ws.binary(clientID, message, len);
  }
}

void wsSendAll(uint8_t* message, size_t len) {
  if (message == nullptr || len == 0) return;
  ws.binaryAll(message, len);
}

void emitEvent(char* message) {
  if (message == nullptr) return;
  memset(event.name, 0, sizeof(event.name));
  strcpy(event.name, message);
  wsSendAll((uint8_t*)&event, sizeof(event));
  scriptRunner.emitEvent(message);
}

void sendProgress() {
  if (progress.status == 1 || progress.status == 0 || hold > 15) {
    if (ws.count() > 0) {
      ws.binaryAll((uint8_t*)&progress, sizeof(progress));
      hold = 0;
    }
    if (progress.status == 0 || progress.status == 5) {
      progressSendCount = 0;
    }
  }
  hold++;
}

void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len) {
  clientID = client->id();
  if (type == WS_EVT_DISCONNECT) {
    if (clientID && !ws.hasClient(clientID)) {
      clientID = 0;
    }
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo* info = (AwsFrameInfo*)arg;
    if (info->opcode == WS_BINARY) {
      if (info->final && info->index == 0 && info->len == len) {
        uint8_t task = data[0];
        if (task > KEY_END) return;
        onWsEventTasks(arg, data, len, clientID, task);
        onWsEventDevice(arg, data, len, clientID, task);
      }
    }
  }
}

void onReqUpload(AsyncWebServerRequest* request) {
  uint8_t method = request->method();
  if (request->hasParam("file")) {
    const AsyncWebParameter* p = request->getParam("file");
    if (method == HTTP_GET)
      if (LittleFS.exists(p->value())) return request->send(LittleFS, p->value(), String(), true);
    if (method == HTTP_DELETE)
      if (LittleFS.exists(p->value()) && (LittleFS.remove(p->value()) || LittleFS.rmdir(p->value()))) return request->send(200, RES_TYPE_JSON, status(1));
  }
  if (method == HTTP_POST && request->hasParam("format"))
    if (LittleFS.format()) return request->send(200, RES_TYPE_JSON, status(1));

  if (method == HTTP_POST) {
    progress.size = 0;
    progress.status = 0;
    progressSendCount = 0;
    return request->send(200, RES_TYPE_JSON, status(1));
  }
  request->send(404, RES_TYPE_JSON, status(0));
}

void onUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
  const uint32_t MAX_FILE_SIZE = 5 * 1024 * 1024;  // 5MB

  progress.size += len;
  progress.status = !index ? 1 : 2;

  if (progress.size > MAX_FILE_SIZE) {
    if (request->_tempFile) {
      request->_tempFile.close();
      LittleFS.remove(filename);
    }
    progress.status = 0;
    request->send(413, RES_TYPE_JSON, status(0));  // 413 Payload Too Large
    return;
  }

  if (!index) {
    progress.size = 0;
    progress.length = request->contentLength();
    if (request->contentLength() > MAX_FILE_SIZE) {
      request->send(413, RES_TYPE_JSON, status(0));
      return;
    }

    if (request->_tempFile) request->_tempFile.close();
    request->_tempFile = LittleFS.open(filename, "w");
    if (!request->_tempFile) {
      request->send(500, RES_TYPE_JSON, status(0));
      return;
    }
    // Serial.println(filename);
  }
  if (len && request->_tempFile) {
    if (progress.size <= MAX_FILE_SIZE) request->_tempFile.write(data, len);
  }
  if (final) {
    if (request->_tempFile) request->_tempFile.close();
    progressSendCount = 0;
  }
  sendProgress();
}

void onReqUpdate(AsyncWebServerRequest* request) {
  uint8_t isReboot = !Update.hasError();
  AsyncWebServerResponse* response = request->beginResponse(200, RES_TYPE_JSON, status(isReboot));
  response->addHeader("Connection", "close");
  request->send(response);
  delay(100);
  tasks[KEY_REBOOT] = true;
}

void onUpdate(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
  progress.size += len;
  progress.status = !index ? 1 : 2;

  if (!index) {
    progress.size = 0;
    progress.length = request->contentLength();
#if defined(ESP8266)
    Update.runAsync(true);
#endif
    int cmd = (filename == "littlefs.bin") ? 100 : U_FLASH;
    size_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if (!Update.begin((cmd == 100) ? infoFS.totalBytes : maxSketchSpace, cmd)) {
      Update.printError(Serial);
      request->send(400, RES_TYPE_JSON, status(0));
      return;
    }
  }

  if (!Update.hasError()) {
    size_t written = Update.write(data, len);
    if (written != len) {
      Update.printError(Serial);
    }
  }

  if (final) {
    if (Update.hasError()) {
      Update.printError(Serial);
      request->send(500, RES_TYPE_JSON, status(0));
      return;
    }
    if (!Update.end(true)) {
      Update.printError(Serial);
      request->send(500, RES_TYPE_JSON, status(0));
      return;
    }
  }

  sendProgress();
}

void onRecovery(AsyncWebServerRequest* request) {
  if (LittleFS.exists("/www/index.html") && !request->hasParam("force")) {
    request->redirect("/");
  } else {
    AsyncWebServerResponse* response = request->beginResponse_P(200, "text/html", recovery, sizeof(recovery));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  }
}

// void onGetData(AsyncWebServerRequest *request) {
//   AsyncResponseStream *response = request->beginResponseStream("application/x-binary", sizeof(infoFS));
//   response->write((const uint8_t *)&infoFS, sizeof(infoFS));
//   request->send(response);
// }
char cmdScriptBuffer[256];

void onCmd(AsyncWebServerRequest* request) {
  if (!request->hasParam("id") || !request->hasParam("script")) {
    request->send(400, RES_TYPE_JSON, status(0));
    return;
  }

  String idStr = request->getParam("id")->value();
  String script = request->getParam("script")->value();

  // Serial.print(script.c_str());
  uint8_t id = idStr.toInt();

  script.replace("%2C", ",");
  script.replace("%3A", ":");
  script.replace("%2A", "*");
  script.replace("%2F", "/");
  script.replace("%3C", "<");
  script.replace("%3E", ">");
  script.replace("%3D", "=");
  script.replace("%21", "!");

  if (id == 0) {
    request->send(400, RES_TYPE_JSON, status(0));
    return;
  }

  if (script.length() >= 255) {
    request->send(400, RES_TYPE_JSON, status(0));
    return;
  }

  strncpy(cmdScriptBuffer, script.c_str(), 255);
  cmdScriptBuffer[255] = '\0';

  bool result = scriptRunner.registerScript(id, cmdScriptBuffer) && scriptRunner.runScript(id);
  request->send(200, RES_TYPE_JSON, status(result));
}

void onRoot(AsyncWebServerRequest* request) {
  if (LittleFS.exists("/www/index.html")) {
    request->send(LittleFS, "/www/index.html", "text/html");
  } else {
    request->redirect("/recovery");
  }
}

void onRedirectHome(AsyncWebServerRequest* request) {
  request->redirect("/");
}

void setupServer() {
  // cors.setOrigin("*");
  // cors.setMethods("POST, GET, OPTIONS, DELETE");
  // cors.setHeaders("X-Custom-Header");
  // cors.setAllowCredentials(true);
  // cors.setMaxAge(600);
  // server.addMiddleware(&cors);

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.serveStatic("/", LittleFS, "/www/").setCacheControl("max-age=600").setDefaultFile("index.html");

  server.on("/fs", HTTP_ANY, onReqUpload, onUpload);
  server.on("/cmd", HTTP_GET, onCmd);
  server.on("/update", HTTP_POST, onReqUpdate, onUpdate);
  server.on("/recovery", HTTP_GET, onRecovery);
  server.on("/", HTTP_GET, onRoot);
  server.on("*", HTTP_ANY, onRoot);

  if (settings.authMode) {
    basicAuth.setUsername(settings.authLogin);
    basicAuth.setPassword(settings.authPass);
    basicAuth.setRealm("vuesp");
    basicAuth.setAuthFailureMessage("Authentication failed");
    basicAuth.setAuthType(AsyncAuthType::AUTH_BASIC);
    basicAuth.generateHash();
    server.addMiddleware(&basicAuth);
  }

  server.begin();
}

void loopServer(uint32_t now) {
  ws.cleanupClients();
  if (ws.count()) {
    if (now - lastTime > 1000) {
      lastTime = now;
      if (progress.status == 0) {
        sendProgress();
        progress.status = 5;
      } else {
        ws.binaryAll((uint8_t*)&ping, sizeof(ping));
      }
    }
  }
}