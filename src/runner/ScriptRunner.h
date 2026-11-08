#ifndef SCRIPT_RUNNER_H
#define SCRIPT_RUNNER_H

#ifdef ARDUINO
#include <Arduino.h>
#else
#include "../tests/arduino_stub.h"
#endif

#include <stdint.h>

#include <cstring>

#define ENABLE_LOGGING 1
#define ENABLE_PORT_LOGGING 1
#define ENABLE_DATA_LOGGING 1
#define ENABLE_LOAD_LOGGING 1
#define ENABLE_EVENT_LOGGING 1
#define ENABLE_SCRIPT_LOGGING 1

#define MAX_SCRIPTS 15
#define MAX_SCRIPT_LEN 256
#define MAX_TOKEN_LEN 64
#define MAX_UINT_VARS 10
#define MAX_INT_VARS 10
#define MAX_FLOAT_VARS 5
#define MAX_STRING_VARS 2
#define MAX_ARRAY_VARS 5
#define MAX_ARRAY_SIZE 64
#define MAX_STRING_LEN 32

#define MAX_PWM_VALUE 255
#define MAX_PARAMS 5
#define MAX_EVENT_HANDLERS 20
#define MAX_EVENT_NAME_LEN 16

#define MAX_EXTERNAL_FUNCTIONS 5
#define MAX_FUNCTION_PARAMS 5

#define MAX_CALL_PARAMS 4

#define SCRIPT_ID_BASE 1
#define EVENT_ID_OFFSET (255 - MAX_SCRIPTS)

#define TOKEN_SEPARATOR ';'
#define ARRAY_SEPARATOR ','

#define SCRIPT_EXEC_INTERVAL_MS 10
#define MAX_EVENT_PARAMS 4
#define MAX_IF_NESTING 8
#define MAX_BLOCK_NESTING 8

#define PORT_READ 0
#define PORT_WRITE 1

enum DataKind : uint8_t {
  KIND_UINT,
  KIND_INT,
  KIND_FLOAT,
  KIND_STRING
};

enum ValueType : uint8_t {
  VAL_NONE = 0,
  VAL_INT,
  VAL_UINT,
  VAL_FLOAT,
  VAL_STRING,
  VAL_ARRAY
};

enum BlockType : uint8_t {
  BLOCK_NONE = 0,
  BLOCK_IF,
  BLOCK_WHILE,
  BLOCK_ON
};

union DataValue {
  uint32_t uintVal;
  int32_t intVal;
  float floatVal;
  struct {
    uint8_t* data;
    uint8_t len;
  } stringVal;
};

struct EventHandler {
  uint32_t hash;
  uint8_t slotId;
  bool active;
};

struct IfStackEntry {
  bool result;
  bool skipElse;
};

struct ScriptContext {
  uint32_t uintVars[MAX_UINT_VARS];
  int32_t intVars[MAX_INT_VARS];
  double floatVars[MAX_FLOAT_VARS];
  char stringVars[MAX_STRING_VARS][MAX_STRING_LEN];
  uint8_t arrayVars[MAX_ARRAY_VARS][MAX_ARRAY_SIZE];
  uint8_t arrayLen[MAX_ARRAY_VARS];

  int32_t eventParams[MAX_EVENT_PARAMS];
  uint8_t eventParamCount;
};

struct Params {
  char values[MAX_PARAMS][32];
  uint8_t count;
};

struct ScriptState {
  bool active;
  bool registered;
  bool isHandler;
  bool isPersistent;
  uint8_t id;
  char script[MAX_SCRIPT_LEN];
  uint16_t scriptLen;
  uint16_t pos;
  uint32_t lastExecutionTime;
  uint8_t blockDepth;
  uint8_t blockStack[MAX_BLOCK_NESTING];
  IfStackEntry ifStack[MAX_IF_NESTING];
  uint8_t ifDepth;
  bool isWhile;
  char whileConditionBuffer[32];
  uint16_t loopStartPos;
  bool skipElse;
  uint8_t skipDepth;
  bool ifResult;
  bool inWait;
  uint32_t waitUntil;
  int32_t tempResult;
  bool hasTempResult;
  bool inEventHandler;
  int32_t callParams[MAX_CALL_PARAMS];
  uint8_t callParamCount;
};

struct Value {
  ValueType type;
  union {
    int32_t intVal;
    uint32_t uintVal;
    float floatVal;
    struct {
      const char* data;
      uint8_t len;
    } stringVal;
    struct {
      const uint8_t* data;
      uint8_t len;
    } arrayVal;
  };
};

typedef bool (*ExternalFunction)(
    uint8_t paramCount,
    const Value* params,
    Value& result,
    void* userData);

typedef bool (*DataProvider)(const char* id, DataKind kind, DataValue& value, bool write);
typedef void (*LogProvider)(const char* message);
typedef bool (*PortProvider)(uint8_t gpio, uint8_t action, uint16_t& value);
typedef void (*StateChangeProvider)(uint8_t gpio, uint16_t newValue);
typedef bool (*LoadProvider)(uint8_t id, char* buffer, uint16_t& len);
typedef void (*ScriptCompleteCallback)(uint8_t slot, uint8_t id);

class ScriptRunner {
 public:
  ScriptRunner();
  ~ScriptRunner();

  bool registerScript(uint8_t id, const char* script, bool persistent = false);
  int8_t runScript(uint8_t id);
  void update();
  bool stopScript(uint8_t id);
  void stopAll();
  bool removeScript(uint8_t id);

  bool isSlotUsed(uint8_t slot) const;
  int8_t getSlotId(uint8_t slot) const;
  bool isSlotActive(uint8_t slot) const;
  bool isSlotHandler(uint8_t slot) const;
  uint16_t getSlotLen(uint8_t slot) const;
  uint8_t getTotalSlots() const;

  // Возвращённые для тестов
  bool isBusy() const;
  uint32_t getUintVar(uint8_t idx) const;
  int32_t getIntVar(uint8_t idx) const;
  float getFloatVar(uint8_t idx) const;
  void setUintVar(uint8_t idx, uint32_t value);
  uint8_t getArrayLen(uint8_t idx) const;
  uint8_t getUsedSlotsCount() const;
  uint8_t getFreeSlotsCount() const;

  static uint32_t hash(const char* str);
  const char* getScript(uint8_t slot) const;
  bool onEvent(uint32_t hash, uint8_t slotId);
  bool onEvent(const char* eventName, uint8_t slotId);
  void emitEvent(uint32_t hash);
  void emitEvent(const char* eventName);
  void emitEvent(const char* eventName, uint8_t paramCount, ...);

  bool registerFunction(const char* name, ExternalFunction func, void* userData = nullptr);

  void setDataProvider(DataProvider provider);
  void setLogProvider(LogProvider provider);
  void setPortProvider(PortProvider provider);
  void setStateChangeProvider(StateChangeProvider provider);
  void setLoadProvider(LoadProvider provider);
  void setScriptCompleteCallback(ScriptCompleteCallback callback);

 private:
  bool runScriptFrom(uint8_t slot, uint16_t offset, uint16_t len);

  ScriptState _slots[MAX_SCRIPTS];
  ScriptContext _ctx;
  EventHandler _eventHandlers[MAX_EVENT_HANDLERS];
  uint8_t _eventHandlerCount;

  DataProvider _dataProvider;
  LogProvider _logProvider;
  PortProvider _portProvider;
  StateChangeProvider _stateChangeProvider;
  LoadProvider _loadProvider;
  ScriptCompleteCallback _completeCallback;

  static ScriptRunner* _instance;

  char _tokenBuf[MAX_TOKEN_LEN];
  char _logBuf[64];
  char _handlerBody[MAX_SCRIPT_LEN];
  char _cleanedBody[MAX_SCRIPT_LEN];
  char _strBuf[MAX_STRING_LEN];
  char _nameBuf[32];

  uint8_t _lastPortValues[40];

  struct ExternalFunctionEntry {
    char name[16];
    ExternalFunction func;
    void* userData;
    bool active;
  };

  ExternalFunctionEntry _extFuncs[MAX_EXTERNAL_FUNCTIONS];
  uint8_t _extFuncCount;
  Value _funcParams[MAX_FUNCTION_PARAMS];
  char _funcStrBufs[MAX_FUNCTION_PARAMS][MAX_STRING_LEN];

  void resetScriptState(uint8_t idx);
  int8_t findSlotById(uint8_t id) const;
  int8_t findFreeSlot();
  void initSlotPools();

  Params parseParams(const char* str) const;
  uint32_t parseTime(const char* str) const;
  uint32_t parseUint(const char** p) const;
  int32_t parseInt(const char** p) const;
  float parseFloat(const char** p) const;
  bool parseString(const char** p, char* buf) const;
  bool parseValue(const char** p, ScriptState& s, int32_t& result, DataKind expectedKind);
  bool parseArray(const char** p, uint8_t idx);
  bool getVariable(uint8_t type, uint8_t idx, ScriptState& s, Value& val);
  bool parseExpression(const char*& p, ScriptState& s, Value& result);

  bool processToken(const char* token, ScriptState& s, uint32_t now);
  bool processCommand(const char* token, ScriptState& s, uint32_t now);

  bool handleCall(const Params& params, ScriptState& s);
  bool handleOn(const Params& params, ScriptState& s, uint32_t now);
  bool handleWait(const Params& params, ScriptState& s, uint32_t now);
  bool handleWhile(const char* params, ScriptState& s);
  bool handleIf(const char* params, ScriptState& s);
  bool handleElse(ScriptState& s);
  bool handleEnd(ScriptState& s);
  bool handleAssignment(const char* token, ScriptState& s);
  bool handleLog(const Params& params, ScriptState& s);

  bool handleSet(const Params& params, ScriptState& s);
  bool handleGet(const Params& params, ScriptState& s);
  bool handleLen(const Params& params, ScriptState& s);
  bool handleChr(const Params& params, ScriptState& s);
  bool handleOrd(const Params& params, ScriptState& s);

  bool parseCondition(const char* token, ScriptState& s);
  bool parseVarData(const char* start, int32_t& result, const char** p, DataKind expectedKind);

  void processScript(uint8_t idx, uint32_t now);
  bool getNextToken(ScriptState& s, char* token, uint16_t& tokenLen);
  void finishScript(ScriptState& s, uint8_t idx);

  bool isExternalFunction(const char* name) const;
  bool callExternalFunction(const char* name, uint8_t paramCount, const Value* params, Value& result);

  void setError(const char* msg);
  void setError(const char* msg, uint8_t slot, uint16_t pos);
  void setError(const char* msg, const char* token, uint8_t slot, uint16_t pos);

  uint8_t readPort(uint8_t pin);
  void writePort(uint8_t pin, uint16_t value, uint8_t slot = 0);
  void writePortSilent(uint8_t pin, uint16_t value);

#if ENABLE_LOGGING
  void logPortAction(uint8_t gpio, uint8_t action, uint16_t value, uint8_t slot);
  void logDataAction(const char* id, DataKind kind, bool write, const char* value, uint8_t slot);
  void logLoadAction(uint8_t id, uint16_t len, bool cached, uint8_t slot);
  void logEventAction(const char* eventName, uint8_t paramCount = 0, const int32_t* params = nullptr);
  void logScriptAction(uint8_t slot, const char* action);
#else
  inline void logPortAction(uint8_t, uint8_t, uint16_t, uint8_t) {}
  inline void logDataAction(const char*, DataKind, bool, const char*, uint8_t) {}
  inline void logLoadAction(uint8_t, uint16_t, bool, uint8_t) {}
  inline void logEventAction(const char*, uint8_t = 0, const int32_t* = nullptr) {}
  inline void logScriptAction(uint8_t, const char*) {}
#endif
};

#endif