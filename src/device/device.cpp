#include "./device.h"

Buffer myBuffer = {KEY_BUFFER};

Device device = {KEY_DEVICE, 0, 100, 1760640900, 2, "text"};
uint32_t lastTimeDevice = 0;

// CircularBuffer<int8_t, 256> buffer(myBuffer);

void onWsEventDevice(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task) {
  AwsFrameInfo* info = (AwsFrameInfo*)arg;
  if (task == KEY_DEVICE && info->len == sizeof(device)) {
    memcpy(&device, data, sizeof(device));
  }
}

void onSendDevice() {
  wsSendAll((uint8_t*)&device, sizeof(device));
}

// only port.interrupt == GPIO_INTERRUPT_CHANGE
void deviceGPIO(Port* port, uint8_t type) {
  Serial.printf("gpio:%d, value:%d", port->gpio, port->value);
  if (type == EVENT_LONG_PRESS) {
    Serial.print(", type:long");
  } else if (type == EVENT_REPEAT) {
    Serial.print(", type:repeat");
  } else if (type == EVENT_CLICK) {
    Serial.printf(", type:click, count:%d", port->count);
  } else {
    // Serial.print(port->gpio);
  }
  Serial.println("");
}

static char displayBuffer[64] = "5";

bool dataProvider(const char* id, DataKind kind, DataValue& value, bool write) {
  Serial.print(id);
  Serial.print(write ? "WRITE" : "READ");
  Serial.print(" kind=");
  Serial.println(kind);

  if (strcmp(id, "$display") == 0) {
    if (write) {
      if (kind == KIND_STRING) {
        char buf[65];
        uint8_t len = value.stringVal.len;
        if (len > 64) len = 64;
        strncpy(buf, (char*)value.stringVal.data, len);
        buf[len] = '\0';
        strcpy(displayBuffer, buf);
        Serial.print("DISPLAY: ");
        Serial.println(buf);
      } else if (kind == KIND_INT) {
        snprintf(displayBuffer, sizeof(displayBuffer), "%d", value.intVal);
        Serial.print("DISPLAY: ");
        Serial.println(value.intVal);
      } else if (kind == KIND_UINT) {
        snprintf(displayBuffer, sizeof(displayBuffer), "%u", value.uintVal);
        Serial.print("DISPLAY: ");
        Serial.println(value.uintVal);
      } else if (kind == KIND_FLOAT) {
        snprintf(displayBuffer, sizeof(displayBuffer), "%.2f", value.floatVal);
        Serial.print("DISPLAY: ");
        Serial.println(value.floatVal);
      }
      return true;
    } else {
      if (kind == KIND_STRING) {
        value.stringVal.data = (uint8_t*)displayBuffer;
        value.stringVal.len = strlen(displayBuffer);
      } else if (kind == KIND_INT) {
        value.intVal = atoi(displayBuffer);
      } else if (kind == KIND_UINT) {
        value.uintVal = (uint32_t)atoi(displayBuffer);
      } else if (kind == KIND_FLOAT) {
        value.floatVal = atof(displayBuffer);
      }
      return true;
    }
  }
  return false;
}

bool httpHandler(uint8_t paramCount, const Value* params, Value& result, void* userData) {
  if (paramCount < 2) return false;

  const char* method = params[0].stringVal.data;
  const char* url = params[1].stringVal.data;
  const int32_t count = params[2].intVal;
  const uint32_t num = params[3].uintVal;

  const uint8_t* data = params[4].arrayVal.data;
  uint8_t len = params[4].arrayVal.len;

  for (uint8_t i = 0; i < len; i++) {
    Serial.print(data[i]);
  }

  Serial.println("method");
  Serial.println(method);
  Serial.println(url);
  Serial.println(count);
  Serial.println(num);

  result.type = VAL_INT;
  result.intVal = 200;
  return true;
}

// void sendNotification(const char* text) {
//   message.type = MESSAGE_TYPE_NOTIFICATION;
//   memset(message.text, 0, sizeof(message.text));
//   strcpy(message.text, text);
//   wsSendAll((uint8_t*)&message, sizeof(message));
// }

void setupDevice() {
  scriptRunner.setDataProvider(dataProvider);
  scriptRunner.registerFunction("http", httpHandler);
}

void setupFirstDevice() {
  getLoadDef(DEF_PATH_CONFIG, (uint8_t*)&device, sizeof(device));
}

void loopDevice(uint32_t now) {
  if (now - lastTimeDevice > 10000) {
    lastTimeDevice = now;
    onSendDevice();
    // sendNotification("Test");
  }

  if (tasks[KEY_BUFFER]) {
    wsSendAll((uint8_t*)&myBuffer, sizeof(myBuffer));
    tasks[KEY_BUFFER] = 0;
  };

  if (tasks[KEY_DEVICE]) {
    if (device.command == COMMAND_SAVE) writeFile(DEF_PATH_CONFIG, (uint8_t*)&device, sizeof(device));

    device.command = 0;
    tasks[KEY_DEVICE] = 0;
    onSendDevice();
  };
}