#include "./runner.h"

uint32_t lastTimeRunner = 0;

uint8_t indexSlot = 0;
uint8_t totalSlots = 0;
uint8_t isSendSlots = 0;

void printSlotsInfo() {
  message.type = MESSAGE_TYPE_SLOT;
  message.id = scriptRunner.getSlotId(indexSlot);
  message.index = indexSlot;
  message.len = scriptRunner.getSlotLen(indexSlot);
  message.active = scriptRunner.isSlotActive(indexSlot);
  message.handler = scriptRunner.isSlotHandler(indexSlot);
  memset(message.text, 0, sizeof(message.text));
  if (scriptRunner.isSlotUsed(indexSlot)) {
    strcpy(message.text, scriptRunner.getScript(indexSlot));
  }
  wsSendAll((uint8_t*)&message, sizeof(message));
}

void eventRunner() {
  if (message.type == MESSAGE_TYPE_SCRIPT_START) {
    scriptRunner.registerScript(message.id, message.text);
    indexSlot = scriptRunner.runScript(message.id);
    printSlotsInfo();
  }
  if (message.type == MESSAGE_TYPE_SCRIPT_STOP) {
    indexSlot = scriptRunner.stopScript(message.id);
    printSlotsInfo();
  }
  if (message.type == MESSAGE_TYPE_SCRIPT_STOP_ALL) {
    scriptRunner.stopAll();
  }
  if (message.type == MESSAGE_TYPE_SCRIPT_REMOVE) {
    indexSlot = scriptRunner.removeScript(message.id);
    printSlotsInfo();
  }
  if (message.type == MESSAGE_TYPE_SCRIPT_GET_ALL_SLOT) {
    isSendSlots = 1;
  }
  indexSlot = 0;
}

void onScriptComplete(uint8_t slot, uint8_t id) {
    Serial.print("Script ");
    Serial.print(id);
    Serial.print(" completed in slot ");
    Serial.println(slot);
}

void setupRunner() {
  scriptRunner.setLoadProvider(loadScriptFromFS);
  scriptRunner.setLogProvider(sendLog);
  scriptRunner.setScriptCompleteCallback(onScriptComplete);
  scriptRunner.runScript(0);
  totalSlots = scriptRunner.getTotalSlots();
}

void loopRunner(uint32_t now) {
  scriptRunner.update();
  if (!isSendSlots) return;
  if (now - lastTimeRunner > 100) {
    Serial.println(indexSlot);
    lastTimeRunner = now;
    printSlotsInfo();
    indexSlot++;
  }
  if (indexSlot >= totalSlots) {
    isSendSlots = 0;
    indexSlot = 0;
  }
}
