#ifndef FILES_H
#define FILES_H

#include <LittleFS.h>

#define CONFIG_LITTLEFS_SPIFFS_COMPAT 1
#define FORMAT_LITTLEFS_IF_FAILED true

#include "../const/index.h"

void initFS();
void getInfo(Info* infoFS);
uint8_t readFile(const char* path, uint8_t* buf, size_t size);
void writeFile(const char* path, const uint8_t* buf, size_t size);
bool createDir(const char* path);
void getLoadDef(const char* path, uint8_t* buf, size_t size);
bool loadScriptFromFS(uint8_t id, char* buffer, uint16_t& len);

#endif