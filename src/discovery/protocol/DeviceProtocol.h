#ifndef DEVICE_PROTOCOL_H
#define DEVICE_PROTOCOL_H

#include <Arduino.h>
#include <stdint.h>

#include <functional>
#include <map>

const uint32_t BROADCAST_ADDRESS = 0xFFFFFFFF;

const uint32_t DEFAULT_ANNOUNCE_INTERVAL = 5000;
const uint32_t DEFAULT_PEER_TIMEOUT = 15000;
const uint32_t DEFAULT_ANNOUNCE_JITTER = 500;

const size_t PACKET_HEADER_SIZE = 5;
const size_t TX_BUFFER_SIZE = 1500;

enum class PacketType : uint8_t {
  TEXT = 0x00,
  BINARY = 0x01
};

enum class DeviceStatus : uint8_t {
  OFFLINE = 0,
  ONLINE = 1
};

struct PeerInfo {
  uint32_t id;
  uint32_t address;
  uint32_t lastSeen;
  uint8_t status;

  PeerInfo() : id(0), address(0), lastSeen(0), status(0) {}
  PeerInfo(uint32_t i, uint32_t a) : id(i), address(a), lastSeen(millis()), status(1) {}
};

struct OutgoingPacket {
  const uint8_t* data;
  size_t length;
  uint32_t targetAddress;
  PacketType type;
};

class DeviceProtocol {
 public:
  using SendCallback = std::function<bool(const OutgoingPacket& packet)>;
  using PeerDiscoveredCallback = std::function<void(const PeerInfo& peer)>;
  using PeerLostCallback = std::function<void(const PeerInfo& peer)>;
  using TextReceivedCallback = std::function<void(uint32_t senderId, const String& text)>;
  using BinaryReceivedCallback = std::function<void(uint32_t senderId, const uint8_t* data, size_t length)>;

  DeviceProtocol();

  void setup(uint32_t myId);

  void onSend(SendCallback callback);
  void onPeerDiscovered(PeerDiscoveredCallback callback);
  void onPeerLost(PeerLostCallback callback);
  void onTextReceived(TextReceivedCallback callback);
  void onBinaryReceived(BinaryReceivedCallback callback);

  bool sendText(uint32_t peerId, const String& text);
  bool sendBinary(uint32_t peerId, const uint8_t* data, size_t length);
  bool sendAnnounce();

  void handleIncomingData(uint32_t senderAddress, const uint8_t* data, size_t length);

  const PeerInfo* getPeer(uint32_t id) const;
  const PeerInfo* getPeerByIndex(size_t index) const;
  size_t getPeerCount() const;
  void forEachPeer(std::function<void(const PeerInfo& peer)> callback) const;

  void update();

  void setAnnounceInterval(uint32_t ms);
  void setPeerTimeout(uint32_t ms);
  void setAnnounceJitter(uint32_t maxMs);

 private:
  uint32_t _myId;
  bool _initialized;

  SendCallback _sendCallback;
  PeerDiscoveredCallback _peerDiscoveredCallback;
  PeerLostCallback _peerLostCallback;
  TextReceivedCallback _textReceivedCallback;
  BinaryReceivedCallback _binaryReceivedCallback;

  std::map<uint32_t, PeerInfo> _peers;

  uint32_t _announceInterval = DEFAULT_ANNOUNCE_INTERVAL;
  uint32_t _peerTimeout = DEFAULT_PEER_TIMEOUT;
  uint32_t _announceJitter = DEFAULT_ANNOUNCE_JITTER;

  DeviceStatus _deviceStatus;
  uint32_t _lastAnnounceTime;

  OutgoingPacket preparePacket(uint32_t targetId, PacketType type,
                               const uint8_t* payload, size_t payloadLength);

  uint8_t _txBuffer[TX_BUFFER_SIZE];
};

#endif