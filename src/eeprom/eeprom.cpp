#include "./eeprom.h"

void initEEprom() {
  EEPROM.begin(256);
}

bool saveSettings(Settings& settings) {
  EEPROM.put(CONFIG_START, settings);
  return EEPROM.commit();
}

void loadConfig(Settings& settings, uint32_t id) {
  uint16_t version = 0;
  EEPROM.get(CONFIG_START + 4, version);
  if (version == SETTINGS_VERSION) {
    EEPROM.get(CONFIG_START, settings);
  } else {
    char nameDevice[20];
    sprintf(nameDevice, "%s%02X", DEF_DEVICE_NAME, id);
    strcpy(settings.wifiSsid, nameDevice);
    settings.version = SETTINGS_VERSION;
    saveSettings(settings);
  }
}
