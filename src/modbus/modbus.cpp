// #include "./include/modbus.h"

// #include <SoftwareSerial.h>

// #include "./include/init.h"

// EspSoftwareSerial::UART ModbusSerial;

// Modbus modbus = {KEY_MODBUS, 0, 255, 0};

// uint32_t lastTimeModbus = 0;

// ModbusSetting modbusSetting = {
//     KEY_MODBUS_SETTING,
//     EspSoftwareSerial::SWSERIAL_8N2,
//     9600,
//     0,
// };

// void setModbusSetup() {
//   ModbusSerial.begin(9600, EspSoftwareSerial::SWSERIAL_8N2, GPIO_485RX, GPIO_485TX);
// }

// void uint16ToByte(uint16_t crc, uint8_t *bytes) {
//   bytes[1] = (crc & 0xFF00) >> 8;
//   bytes[0] = crc & 0x00FF;
// }

// uint16_t getCrc16(uint8_t *data, uint16_t len) {
//   uint8_t lo;
//   union {
//     uint16_t value;
//     struct {
//       uint8_t lo, hi;
//     } bytes;
//   } crc;

//   crc.value = 0xFFFF;

//   while (len--) {
//     lo = crc.bytes.lo;
//     crc.bytes.lo = crc.bytes.hi;
//     crc.bytes.hi = lo ^ *data++;
//     uint8_t mask = 1;
//     if (crc.bytes.hi & mask) crc.value ^= 0x0240;
//     if (crc.bytes.hi & (mask << 1)) crc.value ^= 0x0480;
//     if (crc.bytes.hi & (mask << 2)) crc.bytes.hi ^= 0x09;
//     if (crc.bytes.hi & (mask << 3)) crc.bytes.hi ^= 0x12;
//     if (crc.bytes.hi & (mask << 4)) crc.bytes.hi ^= 0x24;
//     if (crc.bytes.hi & (mask << 5)) crc.bytes.hi ^= 0x48;
//     if (crc.bytes.hi & (mask << 6)) crc.bytes.hi ^= 0x90;
//     if (crc.bytes.hi & (mask << 7)) crc.value ^= 0x2001;
//   }
//   return crc.value;
// }

// uint8_t transmitData(uint8_t *buffer, size_t size, uint8_t isCheck) {
//   ModbusSerial.write(buffer, size);
//   uint8_t bufferIndex = 0;
//   u_int8_t status = 1;
//   lastTimeModbus = millis();

//   while (status) {
//     if (ModbusSerial.available() > 0) {
//       buffer[bufferIndex] = ModbusSerial.read();
//       bufferIndex++;
//     }
//     if (millis() - lastTimeModbus > 300) {
//       status = 0;
//     }
//   }
//   if (isCheck) {
//     uint16_t crc = getCrc16(buffer, bufferIndex - 2);
//     uint8_t bytes[2];
//     uint16ToByte(crc, bytes);
//     if (bytes[0] == buffer[bufferIndex - 2] && bytes[1] == buffer[bufferIndex - 1]) {
//       return bufferIndex;
//     }
//     return 0;
//   }
//   return bufferIndex;
// }