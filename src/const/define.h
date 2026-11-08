#define CONFIG_START 0

// Default struct settings
#define CONFIG_VERSION 0

#define DEF_WIFI_DHCP 1
#define DEF_WIFI_IP \
  {192, 168, 1, 12}
#define DEF_WIFI_SUBNET \
  {255, 255, 255, 0}
#define DEF_WIFI_GETEWAY \
  {192, 168, 1, 1}
#define DEF_WIFI_DNS \
  {8, 8, 8, 8}

#define DEF_WIFI_MODE WIFI_AP
#define DEF_WIFI_SSID ""
#define DEF_WIFI_PASS ""

#define DEF_AUTH_MODE 1
#define DEF_AUTH_LOGIN "admin"
#define DEF_AUTH_PASS "admin"

#define DEF_DISCOVERY 1
#define DEF_DISCOVERY_INTERVAL 5
#define DEF_DISCOVERY_PORT 12345

#define DEF_HIDE_PASS "********"

#define DEF_DIR_DEVICE "/tmp"
#define DEF_PATH_CONFIG DEF_DIR_DEVICE "/device.io"
#define DEF_PATH_GPIO DEF_DIR_DEVICE "/gpio.io"

#define COMMAND_GET 1
#define COMMAND_SET 2
#define COMMAND_TEXT 3
#define COMMAND_SAVE 254

#ifndef DEF_DEVICE_NAME
#define DEF_DEVICE_NAME "VUESP-"
#endif

#ifndef DEF_DEVICE_FIRMWARE
#define DEF_DEVICE_FIRMWARE {0, 0, 1}
#endif
#ifndef DEF_BOARD
#define DEF_BOARD 1
#endif