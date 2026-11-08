#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "../runner/ScriptRunner.h"
#include "arduino_stub.h"

static uint16_t virtualPins[40] = {0};
static std::vector<std::string> testLog;
static std::map<std::string, std::string> dataStore;
static bool test_failed = false;

bool testPortProvider(uint8_t gpio, uint8_t action, uint16_t& value) {
  if (gpio >= 40) return false;
  if (action == PORT_READ) {
    value = virtualPins[gpio];
    return true;
  } else {
    virtualPins[gpio] = value;
    return true;
  }
}

void testStateChangeProvider(uint8_t gpio, uint16_t newValue) {
}

bool testDataProvider(const char* id, DataKind kind, DataValue& value, bool write) {
  if (!id) return false;

  if (strcmp(id, "$display") == 0) {
    if (write) {
      char buf[64];
      if (kind == KIND_STRING) {
        int len = value.stringVal.len < 63 ? value.stringVal.len : 63;
        strncpy(buf, (char*)value.stringVal.data, len);
        buf[len] = '\0';
      } else if (kind == KIND_UINT) {
        snprintf(buf, 64, "%u", value.uintVal);
      } else if (kind == KIND_INT) {
        snprintf(buf, 64, "%d", value.intVal);
      } else if (kind == KIND_FLOAT) {
        snprintf(buf, 64, "%.2f", value.floatVal);
      } else {
        return false;
      }
      testLog.push_back(std::string(buf));
      return true;
    }
    return true;
  }

  if (write) {
    if (kind == KIND_STRING) {
      std::string str((char*)value.stringVal.data, value.stringVal.len);
      dataStore[id] = str;
    } else if (kind == KIND_UINT) {
      char buf[32];
      snprintf(buf, 32, "%u", value.uintVal);
      dataStore[id] = buf;
    } else if (kind == KIND_INT) {
      char buf[32];
      snprintf(buf, 32, "%d", value.intVal);
      dataStore[id] = buf;
    } else if (kind == KIND_FLOAT) {
      char buf[32];
      snprintf(buf, 32, "%.6f", value.floatVal);
      dataStore[id] = buf;
    }
    return true;
  } else {
    auto it = dataStore.find(id);
    if (it != dataStore.end()) {
      if (kind == KIND_STRING) {
        value.stringVal.data = (uint8_t*)it->second.c_str();
        value.stringVal.len = it->second.length();
      } else if (kind == KIND_UINT) {
        value.uintVal = (uint32_t)std::stoul(it->second);
      } else if (kind == KIND_INT) {
        value.intVal = (int32_t)std::stoi(it->second);
      } else if (kind == KIND_FLOAT) {
        value.floatVal = std::stof(it->second);
      }
      return true;
    }
  }
  return false;
}

void testLogProvider(const char* message) {
  if (message) testLog.push_back(std::string(message));
}

void resetTestState() {
  stub_millis = 0;
  memset(virtualPins, 0, sizeof(virtualPins));
  testLog.clear();
  dataStore.clear();
  test_failed = false;
}

void runScriptUntilDone(ScriptRunner& runner, uint32_t maxSteps = 200) {
  uint32_t steps = 0;
  uint32_t idleCount = 0;

  while (runner.isBusy() && steps < maxSteps) {
    stub_millis += 10;
    runner.update();
    steps++;

    if (!runner.isBusy()) {
      idleCount++;
      if (idleCount > 5) break;
    } else {
      idleCount = 0;
    }

    if (steps % 20 == 0 && runner.isBusy()) {
      stub_millis += 100;
    }
  }
}

bool logContains(const char* text) {
  for (const auto& s : testLog) {
    if (s.find(text) != std::string::npos) return true;
  }
  return false;
}

bool logExact(const char* text) {
  for (const auto& s : testLog) {
    if (s == text) return true;
  }
  return false;
}

void printLog() {
  printf("  LOG:\n");
  for (const auto& s : testLog) {
    printf("    %s\n", s.c_str());
  }
}

#define ASSERT_EQ(a, b)                                        \
  do {                                                         \
    if ((a) != (b)) {                                          \
      printf("  FAIL %s:%d: %s == %s (got %d, expected %d)\n", \
             __FILE__, __LINE__, #a, #b, (int)(a), (int)(b));  \
      test_failed = true;                                      \
      printLog();                                              \
      return;                                                  \
    }                                                          \
  } while (0)

#define ASSERT_FLOAT_EQ(a, b, eps)                                 \
  do {                                                             \
    float _a = (a), _b = (b);                                      \
    if (_a < _b - eps || _a > _b + eps) {                          \
      printf("  FAIL %s:%d: %s == %s (got %.3f, expected %.3f)\n", \
             __FILE__, __LINE__, #a, #b, _a, _b);                  \
      test_failed = true;                                          \
      printLog();                                                  \
      return;                                                      \
    }                                                              \
  } while (0)

#define ASSERT_TRUE(a)                                               \
  do {                                                               \
    if (!(a)) {                                                      \
      printf("  FAIL %s:%d: %s is false\n", __FILE__, __LINE__, #a); \
      test_failed = true;                                            \
      printLog();                                                    \
      return;                                                        \
    }                                                                \
  } while (0)

#define ASSERT_FALSE(a)                                             \
  do {                                                              \
    if ((a)) {                                                      \
      printf("  FAIL %s:%d: %s is true\n", __FILE__, __LINE__, #a); \
      test_failed = true;                                           \
      printLog();                                                   \
      return;                                                       \
    }                                                               \
  } while (0)

#define ASSERT_LOG_CONTAINS(text)                         \
  do {                                                    \
    if (!logContains(text)) {                             \
      printf("  FAIL %s:%d: log does not contain '%s'\n", \
             __FILE__, __LINE__, text);                   \
      test_failed = true;                                 \
      printLog();                                         \
      return;                                             \
    }                                                     \
  } while (0)

#define ASSERT_LOG_NOT_CONTAINS(text)             \
  do {                                            \
    if (logContains(text)) {                      \
      printf("  FAIL %s:%d: log contains '%s'\n", \
             __FILE__, __LINE__, text);           \
      test_failed = true;                         \
      printLog();                                 \
      return;                                     \
    }                                             \
  } while (0)

#define ASSERT_LOG_EXACT(text)                                  \
  do {                                                          \
    if (!logExact(text)) {                                      \
      printf("  FAIL %s:%d: log does not exactly match '%s'\n", \
             __FILE__, __LINE__, text);                         \
      test_failed = true;                                       \
      printLog();                                               \
      return;                                                   \
    }                                                           \
  } while (0)

#define ASSERT_LOG_EXACT_ORDER(expected, count)                         \
  do {                                                                  \
    bool ok = true;                                                     \
    for (int i = 0; i < (count); i++) {                                 \
      if (i >= (int)testLog.size() || testLog[i] != expected[i]) {      \
        ok = false;                                                     \
        break;                                                          \
      }                                                                 \
    }                                                                   \
    if (!ok) {                                                          \
      printf("  FAIL %s:%d: log order mismatch\n", __FILE__, __LINE__); \
      printLog();                                                       \
      test_failed = true;                                               \
      return;                                                           \
    }                                                                   \
  } while (0)

#define TEST(name) void test_##name()
#define RUN_TEST(name)                              \
  do {                                              \
    resetTestState();                               \
    test_failed = false;                            \
    printf("  RUN  %s\n", #name);                   \
    test_##name();                                  \
    if (!test_failed) printf("  PASS %s\n", #name); \
  } while (0)

bool testAddHandler(uint8_t paramCount, const Value* params, Value& result, void* userData) {
  if (paramCount < 2) return false;
  if (params[0].type != VAL_INT && params[0].type != VAL_UINT) return false;
  if (params[1].type != VAL_INT && params[1].type != VAL_UINT) return false;

  int32_t a = (params[0].type == VAL_INT) ? params[0].intVal : (int32_t)params[0].uintVal;
  int32_t b = (params[1].type == VAL_INT) ? params[1].intVal : (int32_t)params[1].uintVal;

  result.type = VAL_INT;
  result.intVal = a + b;
  return true;
}

bool testMulHandler(uint8_t paramCount, const Value* params, Value& result, void* userData) {
  if (paramCount < 2) return false;
  if (params[0].type != VAL_INT && params[0].type != VAL_UINT) return false;
  if (params[1].type != VAL_INT && params[1].type != VAL_UINT) return false;

  int32_t a = (params[0].type == VAL_INT) ? params[0].intVal : (int32_t)params[0].uintVal;
  int32_t b = (params[1].type == VAL_INT) ? params[1].intVal : (int32_t)params[1].uintVal;

  result.type = VAL_INT;
  result.intVal = a * b;
  return true;
}

bool testHelloHandler(uint8_t paramCount, const Value* params, Value& result, void* userData) {
  static const char* msg = "Hello from C++";
  result.type = VAL_STRING;
  result.stringVal.data = msg;
  result.stringVal.len = strlen(msg);
  return true;
}

bool testEchoHandler(uint8_t paramCount, const Value* params, Value& result, void* userData) {
  if (paramCount < 1) return false;
  if (params[0].type != VAL_STRING) return false;

  result.type = VAL_STRING;
  result.stringVal.data = params[0].stringVal.data;
  result.stringVal.len = params[0].stringVal.len;
  return true;
}

bool testSum3Handler(uint8_t paramCount, const Value* params, Value& result, void* userData) {
  if (paramCount < 3) return false;

  int32_t sum = 0;
  for (uint8_t i = 0; i < paramCount && i < 3; i++) {
    if (params[i].type == VAL_INT)
      sum += params[i].intVal;
    else if (params[i].type == VAL_UINT)
      sum += (int32_t)params[i].uintVal;
    else
      return false;
  }

  result.type = VAL_INT;
  result.intVal = sum;
  return true;
}

bool testFloatAddHandler(uint8_t paramCount, const Value* params, Value& result, void* userData) {
  if (paramCount < 2) return false;
  if (params[0].type != VAL_FLOAT || params[1].type != VAL_FLOAT) return false;

  result.type = VAL_FLOAT;
  result.floatVal = params[0].floatVal + params[1].floatVal;
  return true;
}

bool testMultiParamHandler(uint8_t paramCount, const Value* params, Value& result, void* userData) {
  if (paramCount < 4) return false;

  if (params[0].type != VAL_STRING) return false;
  if (params[1].type != VAL_STRING) return false;
  if (params[2].type != VAL_INT && params[2].type != VAL_UINT) return false;
  if (params[3].type != VAL_INT && params[3].type != VAL_UINT) return false;

  int32_t a = (params[2].type == VAL_INT) ? params[2].intVal : (int32_t)params[2].uintVal;
  int32_t b = (params[3].type == VAL_INT) ? params[3].intVal : (int32_t)params[3].uintVal;

  result.type = VAL_INT;
  result.intVal = a + b;
  return true;
}

bool testMultiParamNoSpacesHandler(uint8_t paramCount, const Value* params, Value& result, void* userData) {
  if (paramCount < 4) return false;

  if (params[0].type != VAL_STRING) return false;
  if (params[1].type != VAL_STRING) return false;
  if (params[2].type != VAL_INT && params[2].type != VAL_UINT) return false;
  if (params[3].type != VAL_INT && params[3].type != VAL_UINT) return false;

  int32_t a = (params[2].type == VAL_INT) ? params[2].intVal : (int32_t)params[2].uintVal;
  int32_t b = (params[3].type == VAL_INT) ? params[3].intVal : (int32_t)params[3].uintVal;

  result.type = VAL_INT;
  result.intVal = a + b;
  return true;
}

bool testNegativeNumberHandler(uint8_t paramCount, const Value* params, Value& result, void* userData) {
  if (paramCount < 3) return false;
  if (params[0].type != VAL_STRING) return false;
  if (params[1].type != VAL_STRING) return false;
  if (params[2].type != VAL_INT) return false;

  result.type = VAL_INT;
  result.intVal = params[2].intVal;
  return true;
}

bool test4ParamsNegativeHandler(uint8_t paramCount, const Value* params, Value& result, void* userData) {
  if (paramCount < 4) return false;

  if (params[0].type != VAL_STRING) return false;
  if (params[1].type != VAL_STRING) return false;
  if (params[2].type != VAL_INT && params[2].type != VAL_UINT) return false;
  if (params[3].type != VAL_INT && params[3].type != VAL_UINT) return false;

  int32_t a = (params[2].type == VAL_INT) ? params[2].intVal : (int32_t)params[2].uintVal;
  int32_t b = (params[3].type == VAL_INT) ? params[3].intVal : (int32_t)params[3].uintVal;

  result.type = VAL_INT;
  result.intVal = a + b;
  return true;
}

bool testArraySumHandler(uint8_t paramCount, const Value* params, Value& result, void* userData) {
  if (paramCount < 1 || params[0].type != VAL_ARRAY) return false;

  int32_t sum = 0;
  for (uint8_t i = 0; i < params[0].arrayVal.len; i++) {
    sum += params[0].arrayVal.data[i];
  }

  result.type = VAL_INT;
  result.intVal = sum;
  return true;
}

bool testArrayMulHandler(uint8_t paramCount, const Value* params, Value& result, void* userData) {
  if (paramCount < 1 || params[0].type != VAL_ARRAY) return false;

  int32_t mul = 1;
  for (uint8_t i = 0; i < params[0].arrayVal.len; i++) {
    mul *= params[0].arrayVal.data[i];
  }

  result.type = VAL_INT;
  result.intVal = mul;
  return true;
}

// ============================================================
// ТЕСТЫ IF/ELSE
// ============================================================

TEST(if_simple_true) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "if:1==1;$display='A';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("A");
}

TEST(if_simple_false) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "if:1==0;$display='A';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_NOT_CONTAINS("A");
}

TEST(if_else_true) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "if:1==1;$display='A';else;$display='B';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("A");
}

TEST(if_else_false) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "if:1==0;$display='A';else;$display='B';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("B");
}

TEST(if_nested_both_true) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "if:1==1;"
                        "  $display='1';"
                        "  if:1==1;"
                        "    $display='2';"
                        "  end;"
                        "  $display='3';"
                        "end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  const char* expected[] = {"1", "2", "3"};
  ASSERT_LOG_EXACT_ORDER(expected, 3);
}

TEST(if_nested_outer_true_inner_false) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "if:1==1;"
                        "  $display='1';"
                        "  if:1==0;"
                        "    $display='2';"
                        "  end;"
                        "  $display='3';"
                        "end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  const char* expected[] = {"1", "3"};
  ASSERT_LOG_EXACT_ORDER(expected, 2);
}

TEST(if_nested_outer_false) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "if:1==0;"
                        "  $display='1';"
                        "  if:1==1;"
                        "    $display='2';"
                        "  end;"
                        "  $display='3';"
                        "end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(testLog.size(), 0u);
}

TEST(if_nested_with_else_outer_true_inner_false) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "if:1==1;"
                        "  $display='1';"
                        "  if:1==0;"
                        "    $display='2';"
                        "  else;"
                        "    $display='3';"
                        "  end;"
                        "  $display='4';"
                        "end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  const char* expected[] = {"1", "3", "4"};
  ASSERT_LOG_EXACT_ORDER(expected, 3);
}

TEST(if_nested_with_else_outer_true_inner_true) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "if:1==1;"
                        "  $display='1';"
                        "  if:1==1;"
                        "    $display='2';"
                        "  else;"
                        "    $display='3';"
                        "  end;"
                        "  $display='4';"
                        "end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  const char* expected[] = {"1", "2", "4"};
  ASSERT_LOG_EXACT_ORDER(expected, 3);
}

TEST(if_nested_with_else_outer_false) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "if:1==0;"
                        "  $display='1';"
                        "  if:1==1;"
                        "    $display='2';"
                        "  else;"
                        "    $display='3';"
                        "  end;"
                        "  $display='4';"
                        "else;"
                        "  $display='5';"
                        "end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("5");
}

TEST(if_nested_deep_3_levels) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "if:1==1;"
                        "  $display='1';"
                        "  if:1==1;"
                        "    $display='2';"
                        "    if:1==1;"
                        "      $display='3';"
                        "    end;"
                        "  end;"
                        "end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  const char* expected[] = {"1", "2", "3"};
  ASSERT_LOG_EXACT_ORDER(expected, 3);
}

TEST(if_nested_deep_3_levels_with_else) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "if:1==1;"
                        "  $display='1';"
                        "  if:1==1;"
                        "    $display='2';"
                        "    if:1==0;"
                        "      $display='3';"
                        "    else;"
                        "      $display='4';"
                        "    end;"
                        "    $display='5';"
                        "  end;"
                        "end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  const char* expected[] = {"1", "2", "4", "5"};
  ASSERT_LOG_EXACT_ORDER(expected, 4);
}

TEST(if_original_bug_1) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "$display='1';if:1==1;$display='2';if:1==1;$display='3';else;$display='4';end;$display='5';end;$display='6';");
  runner.runScript(1);
  runScriptUntilDone(runner);
  const char* expected[] = {"1", "2", "3", "5", "6"};
  ASSERT_LOG_EXACT_ORDER(expected, 5);
}

TEST(if_original_bug_2) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "$display='1';if:1==1;$display='2';if:1==0;$display='3';else;$display='4';end;$display='5';end;$display='6';");
  runner.runScript(1);
  runScriptUntilDone(runner);
  const char* expected[] = {"1", "2", "4", "5", "6"};
  ASSERT_LOG_EXACT_ORDER(expected, 5);
}

TEST(if_original_bug_3) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "$display='1';if:1==0;$display='2';if:1==1;$display='3';else;$display='4';end;$display='5';else;$display='6';end;$display='7';");
  runner.runScript(1);
  runScriptUntilDone(runner);
  const char* expected[] = {"1", "6", "7"};
  ASSERT_LOG_EXACT_ORDER(expected, 3);
}

TEST(if_original_bug_4) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "$display='1';if:1==0;$display='2';if:1==1;$display='3';else;$display='4';end;$display='5';end;$display='6';");
  runner.runScript(1);
  runScriptUntilDone(runner);
  const char* expected[] = {"1", "6"};
  ASSERT_LOG_EXACT_ORDER(expected, 2);
}

TEST(if_multiple_else_if) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "$u0=2;"
                        "if:$u0==1;"
                        "  $display='ONE';"
                        "else;"
                        "  if:$u0==2;"
                        "    $display='TWO';"
                        "  else;"
                        "    $display='OTHER';"
                        "  end;"
                        "end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("TWO");
}

TEST(if_with_variable_condition) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "$u0=5;"
                        "if:$u0>3;"
                        "  $display='GREATER';"
                        "else;"
                        "  $display='LESS';"
                        "end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("GREATER");
}

TEST(if_with_negative_condition) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "$i0=-5;"
                        "if:$i0<0;"
                        "  $display='NEGATIVE';"
                        "else;"
                        "  $display='POSITIVE';"
                        "end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("NEGATIVE");
}

TEST(if_with_string_condition) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "$s0='hello';"
                        "if:$s0=='hello';"
                        "  $display='MATCH';"
                        "else;"
                        "  $display='NO';"
                        "end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("MATCH");
}

TEST(if_with_or_condition) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "$u0=5;"
                        "$u1=10;"
                        "if:$u0==1||$u1==10;"
                        "  $display='OR_TRUE';"
                        "else;"
                        "  $display='OR_FALSE';"
                        "end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("OR_TRUE");
}

TEST(if_with_and_condition) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "$u0=5;"
                        "$u1=10;"
                        "if:$u0==5&&$u1==10;"
                        "  $display='AND_TRUE';"
                        "else;"
                        "  $display='AND_FALSE';"
                        "end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("AND_TRUE");
}

TEST(if_nested_with_skip_else) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "if:1==0;"
                        "  if:1==1;"
                        "    $display='BAD';"
                        "  else;"
                        "    $display='BAD2';"
                        "  end;"
                        "else;"
                        "  $display='GOOD';"
                        "end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("GOOD");
}

TEST(if_nested_skip_inner_else) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "if:1==1;"
                        "  if:1==0;"
                        "    $display='BAD';"
                        "  else;"
                        "    $display='GOOD_INNER';"
                        "  end;"
                        "  $display='GOOD_OUTER';"
                        "end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  const char* expected[] = {"GOOD_INNER", "GOOD_OUTER"};
  ASSERT_LOG_EXACT_ORDER(expected, 2);
}

TEST(if_deep_nested_with_skip) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "if:1==1;$display='1';if:1==0;$display='2';if:1==1;$display='3';else;$display='4';end;$display='5';else;$display='6';if:1==1;$display='7';else;$display='8';end;$display='9';end;$display='10';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  const char* expected[] = {"1", "6", "7", "9", "10"};
  ASSERT_LOG_EXACT_ORDER(expected, 5);
}

TEST(if_with_multiple_commands_in_block) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "if:1==1;"
                        "  $u0=5;"
                        "  $u1=10;"
                        "  $u2=$u0+$u1;"
                        "  $display=$u2;"
                        "end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("15");
}

TEST(else_without_if_error) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.setLogProvider(testLogProvider);
  runner.registerScript(1, "else;$display='BAD';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_CONTAINS("else without if");
}

TEST(end_without_if_error) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.setLogProvider(testLogProvider);
  runner.registerScript(1, "end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_CONTAINS("end without block");
}

// ============================================================
// НОВЫЕ ТЕСТЫ ДЛЯ СОКРАЩЕННОГО СИНТАКСИСА IF
// ============================================================

TEST(if_short_true) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "if:1;$display='A';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("A");
}

TEST(if_short_false) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "if:0;$display='A';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_NOT_CONTAINS("A");
}

TEST(if_short_variable_true) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=5;if:$u0;$display='A';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("A");
}

TEST(if_short_variable_false) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=0;if:$u0;$display='A';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_NOT_CONTAINS("A");
}

TEST(if_short_negate_true) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=0;if:!$u0;$display='A';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("A");
}

TEST(if_short_negate_false) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=5;if:!$u0;$display='A';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_NOT_CONTAINS("A");
}

TEST(if_short_negate_number_true) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "if:!0;$display='A';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("A");
}

TEST(if_short_negate_number_false) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "if:!1;$display='A';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_NOT_CONTAINS("A");
}

TEST(if_short_string_non_empty) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$s0='hello';if:$s0;$display='A';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("A");
}

TEST(if_short_string_empty) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$s0='';if:$s0;$display='A';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_NOT_CONTAINS("A");
}

TEST(if_short_string_negate_empty) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$s0='';if:!$s0;$display='A';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("A");
}

TEST(if_short_string_negate_non_empty) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$s0='hello';if:!$s0;$display='A';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_NOT_CONTAINS("A");
}

TEST(if_short_nested) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "$u0=1;$u1=0;"
                        "if:$u0;"
                        "  $display='1';"
                        "  if:!$u1;"
                        "    $display='2';"
                        "  else;"
                        "    $display='3';"
                        "  end;"
                        "end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  const char* expected[] = {"1", "2"};
  ASSERT_LOG_EXACT_ORDER(expected, 2);
}

TEST(if_short_float_true) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$f0=3.14;if:$f0;$display='A';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("A");
}

TEST(if_short_float_false) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$f0=0.0;if:$f0;$display='A';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_NOT_CONTAINS("A");
}

TEST(if_short_float_negate) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$f0=0.0;if:!$f0;$display='A';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("A");
}

TEST(if_short_with_else) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "$u0=0;"
                        "if:$u0;"
                        "  $display='TRUE';"
                        "else;"
                        "  $display='FALSE';"
                        "end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("FALSE");
}

TEST(if_short_complex) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "$u0=1;$u1=0;$u2=0;"
                        "if:$u0;"
                        "  $display='1';"
                        "  if:!$u1;"
                        "    $display='2';"
                        "    if:$u2;"
                        "      $display='3';"
                        "    else;"
                        "      $display='4';"
                        "    end;"
                        "  end;"
                        "end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  const char* expected[] = {"1", "2", "4"};
  ASSERT_LOG_EXACT_ORDER(expected, 3);
}

// ============================================================
// СТАРЫЕ ТЕСТЫ (без изменений)
// ============================================================

TEST(variable_assignment_uint) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=42;$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 42u);
  ASSERT_LOG_EXACT("42");
}

TEST(variable_assignment_int) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$i0=-10;$display=$i0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getIntVar(0), -10);
  ASSERT_LOG_EXACT("-10");
}

TEST(variable_assignment_float) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$f0=3.14;$display=$f0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  float val = runner.getFloatVar(0);
  ASSERT_FLOAT_EQ(val, 3.14f, 0.01f);
  ASSERT_LOG_CONTAINS("3.14");
}

TEST(variable_assignment_string) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$s0='Hello';$display=$s0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("Hello");
}

TEST(arithmetic_add) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=10;$u0=$u0+5;$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 15u);
  ASSERT_LOG_EXACT("15");
}

TEST(arithmetic_sub) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=20;$u0=$u0-8;$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 12u);
  ASSERT_LOG_EXACT("12");
}

TEST(arithmetic_mul) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=6;$u0=$u0*7;$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 42u);
  ASSERT_LOG_EXACT("42");
}

TEST(arithmetic_div) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=100;$u0=$u0/4;$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 25u);
  ASSERT_LOG_EXACT("25");
}

TEST(arithmetic_mod) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=17;$u0=$u0%5;$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 2u);
  ASSERT_LOG_EXACT("2");
}

TEST(arithmetic_int_negative) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$i0=20;$i0=8-$i0;$display=$i0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getIntVar(0), -12);
  ASSERT_LOG_EXACT("-12");
}

TEST(arithmetic_float) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$f0=10.5;$f0=$f0+2.5;$display=$f0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  float val = runner.getFloatVar(0);
  ASSERT_FLOAT_EQ(val, 13.0f, 0.1f);
  ASSERT_LOG_CONTAINS("13.00");
}

TEST(bitwise_and) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=255;$u0=$u0&15;$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 15u);
  ASSERT_LOG_EXACT("15");
}

TEST(bitwise_or) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=240;$u0=$u0|15;$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 255u);
  ASSERT_LOG_EXACT("255");
}

TEST(bitwise_xor) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=255;$u0=$u0^255;$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 0u);
  ASSERT_LOG_EXACT("0");
}

TEST(bitwise_not) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$i0=0;$i0=~$i0;$display=$i0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ((int32_t)runner.getIntVar(0), -1);
  ASSERT_LOG_EXACT("-1");
}

TEST(shift_left) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=1;$u0=$u0<<4;$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 16u);
  ASSERT_LOG_EXACT("16");
}

TEST(shift_right) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=16;$u0=$u0>>2;$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 4u);
  ASSERT_LOG_EXACT("4");
}

TEST(condition_if_true) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=1;if:$u0==1;$display='TRUE';else;$display='FALSE';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_CONTAINS("TRUE");
  ASSERT_LOG_NOT_CONTAINS("FALSE");
}

TEST(condition_if_false) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=0;if:$u0==1;$display='TRUE';else;$display='FALSE';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_CONTAINS("FALSE");
  ASSERT_LOG_NOT_CONTAINS("TRUE");
}

TEST(condition_if_else_chain) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=2;if:$u0==1;$display='ONE';else;if:$u0==2;$display='TWO';else;$display='OTHER';end;end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_CONTAINS("TWO");
}

TEST(condition_logical_and) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=1;$u1=1;if:$u0==1&&$u1==1;$display='BOTH';else;$display='NONE';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_CONTAINS("BOTH");
}

TEST(condition_logical_or) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=0;$u1=1;if:$u0==1||$u1==1;$display='ANY';else;$display='NONE';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_CONTAINS("ANY");
}

TEST(condition_compare_string) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$s0='LEDON';if:$s0=='LEDON';$display='MATCH';else;$display='NOMATCH';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_CONTAINS("MATCH");
}

TEST(loop_while) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=0;while:$u0<5;$u0=$u0+1;end;$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 5u);
  ASSERT_LOG_EXACT("5");
}

TEST(string_concat) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$s0='hello';$s0=$s0+'world';$display=$s0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("helloworld");
}

TEST(string_copy) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$s0='hello';$s1=$s0;$display=$s1");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("hello");
}

TEST(string_concat_number) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=42;$s0='Value: ';$s0=$s0+$u0;$display=$s0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("Value: 42");
}

TEST(string_concat_float) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$f0=3.14;$s0='Pi: ';$s0=$s0+$f0;$display=$s0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_CONTAINS("Pi: 3.14");
}

TEST(string_concat_direct) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$s0='Value: ';$s0=$s0+123;$display=$s0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("Value: 123");
}

TEST(port_write) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.setPortProvider(testPortProvider);
  runner.registerScript(1, "$p13=200");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(virtualPins[13], 200u);
}

TEST(port_read) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.setPortProvider(testPortProvider);
  runner.setStateChangeProvider(testStateChangeProvider);
  virtualPins[13] = 200;
  runner.registerScript(1, "$u0=$p13;$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 200u);
  ASSERT_LOG_EXACT("200");
}

TEST(condition_with_port) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.setPortProvider(testPortProvider);
  virtualPins[13] = 0;
  runner.registerScript(1, "if:$p13==0;$display='LOW';else;$display='HIGH';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_CONTAINS("LOW");
}

TEST(register_and_run) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(5, "$display='REGISTERED'");
  runner.runScript(5);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("REGISTERED");
}

TEST(wait_time) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  uint32_t start = stub_millis;
  runner.registerScript(1, "$display='START';wait(100ms);$display='END'");
  runner.runScript(1);
  runScriptUntilDone(runner, 50);
  uint32_t elapsed = stub_millis - start;
  ASSERT_TRUE(elapsed >= 80);
  ASSERT_LOG_CONTAINS("START");
  ASSERT_LOG_CONTAINS("END");
}

TEST(nested_if) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=1;$u1=1;if:$u0==1;if:$u1==1;$display='DEEP';end;end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("DEEP");
}

TEST(loop_in_if) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=0;$u1=1;if:$u1==1;while:$u0<3;$u0=$u0+1;end;end;$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 3u);
  ASSERT_LOG_EXACT("3");
}

TEST(float_copy) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$f0=3.14;$f1=$f0;$display=$f1");
  runner.runScript(1);
  runScriptUntilDone(runner);
  float val = runner.getFloatVar(1);
  ASSERT_FLOAT_EQ(val, 3.14f, 0.01f);
  ASSERT_LOG_CONTAINS("3.14");
}

TEST(strategy_restart) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$display='FIRST'");
  runner.runScript(1);
  runScriptUntilDone(runner, 10);
  testLog.clear();
  runner.registerScript(1, "$display='SECOND'");
  runner.runScript(1);
  runScriptUntilDone(runner, 10);
  ASSERT_LOG_EXACT("SECOND");
}

TEST(strategy_restart_if_same) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$display='SAME'");
  runner.runScript(1);
  runScriptUntilDone(runner, 10);
  testLog.clear();
  runner.registerScript(1, "$display='SAME'");
  runner.runScript(1);
  runScriptUntilDone(runner, 10);
  ASSERT_LOG_EXACT("SAME");
}

TEST(macro_var_to_var) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=99;$u1=$u0;$display=$u1");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(1), 99u);
  ASSERT_LOG_EXACT("99");
}

TEST(macro_float_to_uint) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$f0=12.5;$u0=$f0;$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 12u);
  ASSERT_LOG_EXACT("12");
}

TEST(variable_increment) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=5;$u0=$u0+3;$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 8u);
  ASSERT_LOG_EXACT("8");
}

TEST(variable_decrement) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=10;$u0=$u0-4;$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 6u);
  ASSERT_LOG_EXACT("6");
}

TEST(variable_multiply_assign) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=4;$u0=$u0*5;$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 20u);
  ASSERT_LOG_EXACT("20");
}

TEST(variable_divide_assign) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=30;$u0=$u0/6;$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 5u);
  ASSERT_LOG_EXACT("5");
}

TEST(array_init) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$a0={10,20,30};$u2=len($a0);$display=$u2");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getArrayLen(0), 3u);
  ASSERT_LOG_EXACT("3");
}

TEST(array_get) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$a0={10,20,30};$u0=get($a0,1);$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 20u);
  ASSERT_LOG_EXACT("20");
}

TEST(array_set) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$a0={10,20,30};set($a0,1,99);$u0=get($a0,1);$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 99u);
  ASSERT_LOG_EXACT("99");
}

TEST(array_copy) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$a0={10,20,30};$a1=$a0;$u0=get($a1,0);$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 10u);
  ASSERT_LOG_EXACT("10");
}

TEST(array_len) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$a0={10,20,30};$u2=len($a0);$display=$u2");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(2), 3u);
  ASSERT_LOG_EXACT("3");
}

TEST(chr_ord) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$s0=chr(65);$u0=ord($s0);$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 65u);
  ASSERT_LOG_EXACT("65");
}

TEST(division_by_zero) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.setLogProvider(testLogProvider);
  runner.registerScript(1, "$u0=10;$u0=$u0/0;$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_TRUE(true);
}

TEST(duplicate_script_id) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$display='FIRST'");
  bool result = runner.registerScript(1, "$display='SECOND'");
  ASSERT_TRUE(result);
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("SECOND");
}

TEST(very_long_string) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  char longStr[MAX_STRING_LEN + 10];
  memset(longStr, 'A', sizeof(longStr) - 1);
  longStr[sizeof(longStr) - 1] = '\0';
  char script[256];
  snprintf(script, sizeof(script), "$s0='%s';$display=$s0", longStr);
  runner.registerScript(1, script);
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("AAAAA"));
}

TEST(port_out_of_range) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.setPortProvider(testPortProvider);
  runner.registerScript(1, "$p99=255");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_TRUE(true);
}

TEST(recursive_call) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.setLogProvider(testLogProvider);
  runner.registerScript(1, "call(1)");
  runner.runScript(1);
  runScriptUntilDone(runner, 20);
  ASSERT_TRUE(true);
}

TEST(if_without_else) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=1;if:$u0==1;$display='TRUE';end;$display='DONE'");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_CONTAINS("TRUE");
  ASSERT_LOG_CONTAINS("DONE");
}

TEST(if_without_end) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=1;if:$u0==1;$display='TRUE'");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_TRUE(true);
}

TEST(while_with_break) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=0;while:$u0<10;$u0=$u0+1;if:$u0==5;$u0=10;end;end;$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 10u);
}

TEST(array_out_of_bounds) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$a0={1,2,3};$u0=get($a0,10);$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 0u);
}

TEST(string_index_out_of_bounds) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$s0='ABC';$u0=ord($s0,10);$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_TRUE(true);
}

TEST(negative_array_index) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$a0={1,2,3};$u0=get($a0,-1);$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 0u);
}

TEST(float_compare_equal) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$f0=3.14;$f1=3.14;if:$f0==$f1;$display='EQUAL';else;$display='NOT';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("EQUAL");
}

TEST(float_compare_not_equal) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$f0=3.14;$f1=3.15;if:$f0!=$f1;$display='NOT';else;$display='EQUAL';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("NOT");
}

TEST(string_compare_not_equal) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$s0='hello';$s1='world';if:$s0!=$s1;$display='NOT';else;$display='EQUAL';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("NOT");
}

TEST(script_exceeds_max_len) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  char script[MAX_SCRIPT_LEN + 10];
  memset(script, 'A', sizeof(script) - 1);
  script[sizeof(script) - 1] = '\0';
  bool result = runner.registerScript(1, script);
  ASSERT_FALSE(result);
}

TEST(get_uint_var_out_of_range) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  uint32_t val = runner.getUintVar(99);
  ASSERT_EQ(val, 0u);
}

TEST(get_int_var_out_of_range) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  int32_t val = runner.getIntVar(99);
  ASSERT_EQ(val, 0);
}

TEST(get_float_var_out_of_range) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  float val = runner.getFloatVar(99);
  ASSERT_FLOAT_EQ(val, 0.0f, 0.01f);
}

TEST(set_uint_var_out_of_range) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.setUintVar(99, 42);
  ASSERT_TRUE(true);
}

TEST(memory_slots) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  ASSERT_EQ(runner.getTotalSlots(), MAX_SCRIPTS);
  ASSERT_EQ(runner.getFreeSlotsCount(), MAX_SCRIPTS);
  runner.registerScript(1, "test");
  ASSERT_EQ(runner.getUsedSlotsCount(), 1);
  ASSERT_EQ(runner.getFreeSlotsCount(), MAX_SCRIPTS - 1);
}

TEST(script_too_long) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  char longScript[MAX_SCRIPT_LEN + 10];
  memset(longScript, 'A', sizeof(longScript) - 1);
  longScript[sizeof(longScript) - 1] = '\0';
  bool result = runner.registerScript(1, longScript);
  ASSERT_FALSE(result);
}

TEST(stop_script) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "wait(1000ms);$display='SHOULD_NOT_RUN'");
  runner.runScript(1);
  runner.stopScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_NOT_CONTAINS("SHOULD_NOT_RUN");
}

TEST(stop_all) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "wait(1000ms);$display='S1'");
  runner.registerScript(2, "wait(1000ms);$display='S2'");
  runner.runScript(1);
  runner.runScript(2);
  runner.stopAll();
  runScriptUntilDone(runner);
  ASSERT_LOG_NOT_CONTAINS("S1");
  ASSERT_LOG_NOT_CONTAINS("S2");
}

TEST(is_busy) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "wait(100ms)");
  runner.runScript(1);
  ASSERT_TRUE(runner.isBusy());
  runScriptUntilDone(runner);
  ASSERT_FALSE(runner.isBusy());
}

TEST(wait_until) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  uint32_t start = stub_millis;
  runner.registerScript(1, "$display='START';wait(50ms);wait(50ms);$display='END'");
  runner.runScript(1);
  runScriptUntilDone(runner);
  uint32_t elapsed = stub_millis - start;
  ASSERT_TRUE(elapsed >= 80);
  ASSERT_LOG_CONTAINS("START");
  ASSERT_LOG_CONTAINS("END");
}

TEST(while_infinite_protection) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=0;while:$u0<10;$u0=$u0+1;end;$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner, 50);
  ASSERT_EQ(runner.getUintVar(0), 10u);
  ASSERT_LOG_EXACT("10");
}

TEST(nested_on_inside_on) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "on('EVENT1');"
                        "  $display='OUTER';"
                        "  on('EVENT2');"
                        "    $display='INNER';"
                        "  end;"
                        "end");
  runner.runScript(1);
  runScriptUntilDone(runner);

  runner.emitEvent("EVENT1");
  runScriptUntilDone(runner);
  ASSERT_LOG_CONTAINS("OUTER");

  runner.emitEvent("EVENT2");
  runScriptUntilDone(runner);
  ASSERT_LOG_CONTAINS("INNER");
}

TEST(nested_if_inside_on) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "on('EVENT');"
                        "  $u0=1;"
                        "  if:$u0==1;"
                        "    $display='IF_TRUE';"
                        "  else;"
                        "    $display='IF_FALSE';"
                        "  end;"
                        "end");
  runner.runScript(1);
  runScriptUntilDone(runner);

  runner.emitEvent("EVENT");
  runScriptUntilDone(runner);
  ASSERT_LOG_CONTAINS("IF_TRUE");
  ASSERT_LOG_NOT_CONTAINS("IF_FALSE");
}

TEST(nested_while_inside_on) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "on('EVENT');"
                        "  $u0=0;"
                        "  while:$u0<3;"
                        "    $u0=$u0+1;"
                        "  end;"
                        "  $display=$u0;"
                        "end");
  runner.runScript(1);
  runScriptUntilDone(runner);

  runner.emitEvent("EVENT");
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("3");
}

TEST(if_inside_while_inside_on) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "on('EVENT');"
                        "  $u0=0;"
                        "  while:$u0<5;"
                        "    $u0=$u0+1;"
                        "    if:$u0==3;"
                        "      $display='THREE';"
                        "    end;"
                        "  end;"
                        "end");
  runner.runScript(1);
  runScriptUntilDone(runner);

  runner.emitEvent("EVENT");
  runScriptUntilDone(runner);
  ASSERT_LOG_CONTAINS("THREE");
}

TEST(on_inside_if) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1,
                        "$u0=1;"
                        "if:$u0==1;"
                        "  on('EVENT');"
                        "    $display='INSIDE_IF';"
                        "  end;"
                        "end");
  runner.runScript(1);
  runScriptUntilDone(runner);

  runner.emitEvent("EVENT");
  runScriptUntilDone(runner);
  ASSERT_LOG_CONTAINS("INSIDE_IF");
}

TEST(external_function_add) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);

  bool registered = runner.registerFunction("add", testAddHandler);
  ASSERT_TRUE(registered);

  runner.registerScript(1, "$u0=add(10,20);$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 30u);
  ASSERT_LOG_EXACT("30");
}

TEST(external_function_add_with_vars) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);

  runner.registerFunction("add", testAddHandler);

  runner.registerScript(1, "$u0=5;$u1=7;$u2=add($u0,$u1);$display=$u2");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(2), 12u);
  ASSERT_LOG_EXACT("12");
}

TEST(external_function_mul) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);

  runner.registerFunction("mul", testMulHandler);

  runner.registerScript(1, "$u0=mul(6,7);$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 42u);
  ASSERT_LOG_EXACT("42");
}

TEST(external_function_string) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);

  runner.registerFunction("hello", testHelloHandler);

  runner.registerScript(1, "$s0=hello();$display=$s0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("Hello from C++");
}

TEST(external_function_echo) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);

  runner.registerFunction("echo", testEchoHandler);

  runner.registerScript(1, "$s0=echo('Test message');$display=$s0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("Test message");
}

TEST(external_function_echo_with_var) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);

  runner.registerFunction("echo", testEchoHandler);

  runner.registerScript(1, "$s0='Hello world';$s1=echo($s0);$display=$s1");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("Hello world");
}

TEST(external_function_3_params) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);

  runner.registerFunction("sum3", testSum3Handler);

  runner.registerScript(1, "$u0=sum3(1,2,3);$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 6u);
  ASSERT_LOG_EXACT("6");
}

TEST(external_function_3_params_with_vars) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);

  runner.registerFunction("sum3", testSum3Handler);

  runner.registerScript(1, "$u0=1;$u1=2;$u2=3;$u3=sum3($u0,$u1,$u2);$display=$u3");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(3), 6u);
  ASSERT_LOG_EXACT("6");
}

TEST(external_function_int_to_uint) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);

  runner.registerFunction("add", testAddHandler);

  runner.registerScript(1, "$i0=add(10,20);$display=$i0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getIntVar(0), 30);
  ASSERT_LOG_EXACT("30");
}

TEST(external_function_float_result) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);

  runner.registerFunction("fadd", testFloatAddHandler);

  runner.registerScript(1, "$f0=1.5;$f1=2.5;$f2=fadd($f0,$f1);$display=$f2");
  runner.runScript(1);
  runScriptUntilDone(runner);
  float val = runner.getFloatVar(2);
  ASSERT_FLOAT_EQ(val, 4.0f, 0.1f);
  ASSERT_LOG_CONTAINS("4.00");
}

TEST(external_function_4_params) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);

  runner.registerFunction("multi", testMultiParamHandler);

  runner.registerScript(1, "$u0=multi('POST','http://test.com',154,21);$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 175u);
  ASSERT_LOG_EXACT("175");
}

TEST(external_function_4_params_no_spaces) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);

  runner.registerFunction("multi", testMultiParamNoSpacesHandler);

  runner.registerScript(1, "$u0=multi('POST','http://test.com',154,21);$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 175u);
  ASSERT_LOG_EXACT("175");
}

TEST(external_function_negative_number) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);

  runner.registerFunction("neg", testNegativeNumberHandler);

  runner.registerScript(1, "$i0=neg('POST','http://test.com',-16000);$display=$i0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getIntVar(0), -16000);
  ASSERT_LOG_EXACT("-16000");
}

TEST(external_function_4_params_negative) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);

  runner.registerFunction("multi", test4ParamsNegativeHandler);

  runner.registerScript(1, "$i0=multi('POST','http://test.com',-16000,4121556);$display=$i0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getIntVar(0), 4105556);
  ASSERT_LOG_EXACT("4105556");
}

TEST(external_function_array_sum) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);

  runner.registerFunction("sum", testArraySumHandler);

  runner.registerScript(1, "$a0={10,20,30,40,50};$u0=sum($a0);$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 150u);
  ASSERT_LOG_EXACT("150");
}

TEST(external_function_array_mul) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);

  runner.registerFunction("mul", testArrayMulHandler);

  runner.registerScript(1, "$a0={1,2,3,4,5};$u0=mul($a0);$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 120u);
  ASSERT_LOG_EXACT("120");
}

TEST(external_function_array_return_array) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);

  auto doubleArrayHandler = [](uint8_t paramCount, const Value* params, Value& result, void* userData) -> bool {
    if (paramCount < 1 || params[0].type != VAL_ARRAY) return false;

    static uint8_t resultArray[MAX_ARRAY_SIZE];
    uint8_t len = params[0].arrayVal.len;
    if (len > MAX_ARRAY_SIZE) len = MAX_ARRAY_SIZE;

    for (uint8_t i = 0; i < len; i++) {
      resultArray[i] = params[0].arrayVal.data[i] * 2;
    }

    result.type = VAL_ARRAY;
    result.arrayVal.data = resultArray;
    result.arrayVal.len = len;
    return true;
  };

  runner.registerFunction("double", doubleArrayHandler);

  runner.registerScript(1, "$a0={1,2,3,4};$a1=double($a0);$u0=get($a1,2);$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 6u);
  ASSERT_LOG_EXACT("6");
}

TEST(external_function_array_and_int) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);

  auto arrayAndIntHandler = [](uint8_t paramCount, const Value* params, Value& result, void* userData) -> bool {
    if (paramCount < 2 || params[0].type != VAL_ARRAY || params[1].type != VAL_INT) return false;

    int32_t sum = 0;
    for (uint8_t i = 0; i < params[0].arrayVal.len; i++) {
      sum += params[0].arrayVal.data[i];
    }
    sum += params[1].intVal;

    result.type = VAL_INT;
    result.intVal = sum;
    return true;
  };

  runner.registerFunction("addToArray", arrayAndIntHandler);

  runner.registerScript(1, "$a0={10,20,30,40};$u0=addToArray($a0,100);$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 200u);
  ASSERT_LOG_EXACT("200");
}

TEST(external_function_overflow) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);

  for (int i = 0; i < 5; i++) {
    char name[16];
    snprintf(name, sizeof(name), "func%d", i);
    runner.registerFunction(name, testAddHandler);
  }

  bool result = runner.registerFunction("func5", testAddHandler);
  ASSERT_FALSE(result);
}

TEST(external_function_type_mismatch) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.setLogProvider(testLogProvider);

  runner.registerFunction("hello", testHelloHandler);

  runner.registerScript(1, "$u0=hello();$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_CONTAINS("type mismatch");
}

TEST(ternary_simple_true) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=10>5?100:0;$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("100");
}

TEST(ternary_simple_false) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=10>15?100:0;$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("0");
}

TEST(ternary_with_variable) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=7;$u1=$u0>5?$u0*2:$u0/2;$display=$u1");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("14");
}

TEST(ternary_with_arithmetic_no_parens) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=3;$u1=4;$u2=$u0*$u1+1>6?$u0+$u1*2:$u0-$u1*3;$display=$u2");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("14");
}

TEST(ternary_nested) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=5;$u1=$u0>0?$u0<10?1:2:0;$display=$u1");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("1");
}

TEST(ternary_float) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$f0=3.14;$f1=$f0>3.0?6.28:1.57;$display=$f1");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_CONTAINS("1.57");
}

TEST(ternary_port) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.setPortProvider(testPortProvider);
  virtualPins[0] = 0;
  runner.registerScript(1, "$p1=$p0==0?255:0;$display=$p1");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(virtualPins[1], 255u);
  ASSERT_LOG_EXACT("255");
}

TEST(ternary_with_compare_equal) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=5;$u1=$u0==5?100:0;$display=$u1");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("100");
}

TEST(ternary_with_compare_not_equal) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=5;$u1=$u0!=5?100:0;$display=$u1");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("0");
}

TEST(ternary_with_less_equal) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=5;$u1=$u0<=5?1:0;$display=$u1");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("1");
}

TEST(ternary_with_greater_equal) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=5;$u1=$u0>=5?1:0;$display=$u1");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("1");
}

TEST(arithmetic_with_parens) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=(2+3)*4;$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("20");
}

TEST(ternary_with_parens) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=5;$u1=$u0>0?($u0<10?1:2):0;$display=$u1");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("1");
}

TEST(ternary_complex_with_parens) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=3;$u1=4;$u2=(($u0+$u1)*2)>10?100:0;$display=$u2");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("100");
}

TEST(arithmetic_nested_parens) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=((2+3)*4+5)*2;$display=$u0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("50");
}

TEST(ternary_with_parens_and_compare) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$u0=10;$u1=5;$u2=($u0>$u1)?($u0*$u1):($u0+$u1);$display=$u2");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("50");
}

int main() {
  printf("=== ScriptRunner Tests ===\n\n");

  RUN_TEST(variable_assignment_uint);
  RUN_TEST(variable_assignment_int);
  RUN_TEST(variable_assignment_float);
  RUN_TEST(variable_assignment_string);

  RUN_TEST(arithmetic_add);
  RUN_TEST(arithmetic_sub);
  RUN_TEST(arithmetic_mul);
  RUN_TEST(arithmetic_div);
  RUN_TEST(arithmetic_mod);
  RUN_TEST(arithmetic_int_negative);
  RUN_TEST(arithmetic_float);

  RUN_TEST(bitwise_and);
  RUN_TEST(bitwise_or);
  RUN_TEST(bitwise_xor);
  RUN_TEST(bitwise_not);
  RUN_TEST(shift_left);
  RUN_TEST(shift_right);

  RUN_TEST(condition_if_true);
  RUN_TEST(condition_if_false);
  RUN_TEST(condition_if_else_chain);
  RUN_TEST(condition_logical_and);
  RUN_TEST(condition_logical_or);
  RUN_TEST(condition_compare_string);

  RUN_TEST(loop_while);

  RUN_TEST(string_concat);
  RUN_TEST(string_copy);
  RUN_TEST(string_concat_number);
  RUN_TEST(string_concat_float);
  RUN_TEST(string_concat_direct);

  RUN_TEST(port_write);
  RUN_TEST(port_read);
  RUN_TEST(condition_with_port);

  RUN_TEST(register_and_run);

  RUN_TEST(wait_time);

  RUN_TEST(nested_if);
  RUN_TEST(loop_in_if);

  RUN_TEST(float_copy);

  RUN_TEST(strategy_restart);
  RUN_TEST(strategy_restart_if_same);

  RUN_TEST(macro_var_to_var);
  RUN_TEST(macro_float_to_uint);

  RUN_TEST(variable_increment);
  RUN_TEST(variable_decrement);
  RUN_TEST(variable_multiply_assign);
  RUN_TEST(variable_divide_assign);

  RUN_TEST(array_init);
  RUN_TEST(array_get);
  RUN_TEST(array_set);
  RUN_TEST(array_copy);
  RUN_TEST(array_len);

  RUN_TEST(chr_ord);

  RUN_TEST(memory_slots);
  RUN_TEST(script_too_long);
  RUN_TEST(duplicate_script_id);
  RUN_TEST(script_exceeds_max_len);

  RUN_TEST(stop_script);
  RUN_TEST(stop_all);
  RUN_TEST(is_busy);

  RUN_TEST(wait_until);

  RUN_TEST(while_infinite_protection);
  RUN_TEST(while_with_break);

  RUN_TEST(if_without_else);
  RUN_TEST(if_without_end);

  RUN_TEST(float_compare_equal);
  RUN_TEST(float_compare_not_equal);

  RUN_TEST(string_compare_not_equal);

  RUN_TEST(array_out_of_bounds);
  RUN_TEST(negative_array_index);

  RUN_TEST(string_index_out_of_bounds);
  RUN_TEST(very_long_string);

  RUN_TEST(port_out_of_range);

  RUN_TEST(division_by_zero);
  RUN_TEST(recursive_call);

  RUN_TEST(get_uint_var_out_of_range);
  RUN_TEST(get_int_var_out_of_range);
  RUN_TEST(get_float_var_out_of_range);
  RUN_TEST(set_uint_var_out_of_range);

  RUN_TEST(nested_on_inside_on);
  RUN_TEST(nested_if_inside_on);
  RUN_TEST(nested_while_inside_on);
  RUN_TEST(if_inside_while_inside_on);
  RUN_TEST(on_inside_if);

  RUN_TEST(external_function_add);
  RUN_TEST(external_function_add_with_vars);
  RUN_TEST(external_function_mul);
  RUN_TEST(external_function_string);
  RUN_TEST(external_function_echo);
  RUN_TEST(external_function_echo_with_var);
  RUN_TEST(external_function_3_params);
  RUN_TEST(external_function_3_params_with_vars);
  RUN_TEST(external_function_int_to_uint);
  RUN_TEST(external_function_float_result);
  RUN_TEST(external_function_4_params);
  RUN_TEST(external_function_4_params_no_spaces);
  RUN_TEST(external_function_negative_number);
  RUN_TEST(external_function_4_params_negative);
  RUN_TEST(external_function_array_sum);
  RUN_TEST(external_function_array_mul);
  RUN_TEST(external_function_array_return_array);
  RUN_TEST(external_function_array_and_int);
  RUN_TEST(external_function_overflow);

  RUN_TEST(if_simple_true);
  RUN_TEST(if_simple_false);
  RUN_TEST(if_else_true);
  RUN_TEST(if_else_false);
  RUN_TEST(if_nested_both_true);
  RUN_TEST(if_nested_outer_true_inner_false);
  RUN_TEST(if_nested_outer_false);
  RUN_TEST(if_nested_with_else_outer_true_inner_false);
  RUN_TEST(if_nested_with_else_outer_true_inner_true);
  RUN_TEST(if_nested_with_else_outer_false);
  RUN_TEST(if_nested_deep_3_levels);
  RUN_TEST(if_nested_deep_3_levels_with_else);
  RUN_TEST(if_original_bug_1);
  RUN_TEST(if_original_bug_2);
  RUN_TEST(if_original_bug_3);
  RUN_TEST(if_original_bug_4);
  RUN_TEST(if_multiple_else_if);
  RUN_TEST(if_with_variable_condition);
  RUN_TEST(if_with_negative_condition);
  RUN_TEST(if_with_string_condition);
  RUN_TEST(if_with_or_condition);
  RUN_TEST(if_with_and_condition);
  RUN_TEST(if_nested_with_skip_else);
  RUN_TEST(if_nested_skip_inner_else);
  RUN_TEST(if_deep_nested_with_skip);
  RUN_TEST(if_with_multiple_commands_in_block);
  RUN_TEST(else_without_if_error);
  RUN_TEST(end_without_if_error);

  RUN_TEST(if_short_true);
  RUN_TEST(if_short_false);
  RUN_TEST(if_short_variable_true);
  RUN_TEST(if_short_variable_false);
  RUN_TEST(if_short_negate_true);
  RUN_TEST(if_short_negate_false);
  RUN_TEST(if_short_negate_number_true);
  RUN_TEST(if_short_negate_number_false);
  RUN_TEST(if_short_string_non_empty);
  RUN_TEST(if_short_string_empty);
  RUN_TEST(if_short_string_negate_empty);
  RUN_TEST(if_short_string_negate_non_empty);
  RUN_TEST(if_short_nested);
  RUN_TEST(if_short_float_true);
  RUN_TEST(if_short_float_false);
  RUN_TEST(if_short_float_negate);
  RUN_TEST(if_short_with_else);
  RUN_TEST(if_short_complex);

  RUN_TEST(external_function_type_mismatch);

  RUN_TEST(ternary_simple_true);
  RUN_TEST(ternary_simple_false);
  RUN_TEST(ternary_with_variable);
  RUN_TEST(ternary_with_arithmetic_no_parens);
  RUN_TEST(ternary_nested);
  RUN_TEST(ternary_float);
  RUN_TEST(ternary_port);
  RUN_TEST(ternary_with_compare_equal);
  RUN_TEST(ternary_with_compare_not_equal);
  RUN_TEST(ternary_with_less_equal);
  RUN_TEST(ternary_with_greater_equal);

  RUN_TEST(arithmetic_with_parens);
  RUN_TEST(ternary_with_parens);
  RUN_TEST(ternary_complex_with_parens);
  RUN_TEST(arithmetic_nested_parens);
  RUN_TEST(ternary_with_parens_and_compare);

  printf("\n=== All tests passed ===\n");
  return 0;
}