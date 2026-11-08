enum keys {
  KEY_INIT,
  KEY_SETTINGS,
  KEY_INFO,
  KEY_PING,
  KEY_SCAN,
  KEY_PROGRESS,
  KEY_FILES,
  KEY_REBOOT,
  KEY_EVENT,
  KEY_PORT,
  KEY_MESSAGE,
  KEY_DALLAS,
  KEY_BUFFER,
  KEY_DEVICE,
  KEY_DISCOVERY,
  KEY_END
};

struct Init {
  uint8_t key;
};
struct Settings {
  uint8_t key;
  uint8_t wifiDhcp;
  uint8_t wifiMode;
  uint8_t authMode;
  uint16_t version;
  uint16_t device;

  uint8_t discovery;
  uint8_t discoveryInterval;
  uint16_t discoveryPort;

  uint8_t wifiIp[4];
  uint8_t wifiSubnet[4];
  uint8_t wifiGateway[4];
  uint8_t wifiDns[4];

  char wifiSsid[32];
  char wifiPass[32];
  char authLogin[12];
  char authPass[12];
};

struct Info {
  uint8_t key;
  uint8_t firmware[3];
  uint32_t totalBytes;
  uint32_t usedBytes;
  uint32_t id;
  uint64_t uptime;
  char name[20];
  uint32_t board;
};

struct Ping {
  uint8_t key;
};

struct Scan {
  uint8_t key;
  uint8_t id;
  uint8_t isHidden;
  uint8_t encryptionType;
  int32_t channel;
  int32_t rssi;
  char ssid[32];
};

struct Progress {
  uint8_t key;
  uint8_t status;
  uint16_t empty;
  uint32_t size;
  uint32_t length;
};

struct Files {
  uint8_t key;
  uint8_t isDir;
  uint8_t isFile;
  uint8_t command;
  uint32_t size;
  char name[32];
};

struct Event {
  uint8_t key;
  uint8_t comm;
  uint16_t comm2;
  uint32_t value1;
  uint32_t value2;
  char name[32];
};

struct Port {
  uint8_t key;
  uint8_t gpio;
  uint16_t value;
  uint8_t mode;
  uint8_t interrupt;
  uint16_t list;
  uint8_t disabled;
  uint8_t isButton;
  uint8_t command;
  uint8_t isPressed;
  uint8_t count;
  uint8_t empty;
  uint16_t valueOld;
  uint32_t pressStart;
};

struct Message {
  uint8_t key;
  uint8_t type;
  uint8_t id;
  uint8_t active;
  uint8_t handler;
  uint8_t index;
  uint16_t len;
  uint32_t date;
  char text[256];
};

struct Dallas {
  uint8_t key;
  uint8_t comm;
  uint16_t comm2;
  uint8_t address[8];
  float temp;
};

struct Buffer {
  uint8_t key;
  uint8_t empty;
  uint16_t head;
  uint16_t tail;
  uint16_t count;
  int8_t data[256];
};
struct Discovery {
  uint8_t key;
  uint8_t comm;
  uint8_t status;
  uint8_t empty;
  uint32_t id;
  uint32_t ip;
  uint32_t lastSeen;
};

struct Device {
  uint8_t key;
  uint8_t command;
  uint16_t analog;
  uint32_t now;
  uint32_t list;
  char message[32];
};
