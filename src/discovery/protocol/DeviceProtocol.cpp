#include "DeviceProtocol.h"

DeviceProtocol::DeviceProtocol() : _myId(0), _initialized(false), _sendCallback(nullptr), _peerDiscoveredCallback(nullptr), _peerLostCallback(nullptr), _textReceivedCallback(nullptr), _binaryReceivedCallback(nullptr), _deviceStatus(DeviceStatus::OFFLINE), _lastAnnounceTime(0) {}

void DeviceProtocol::setup(uint32_t myId) {
  _myId = myId;
  _initialized = true;
  _deviceStatus = DeviceStatus::OFFLINE;
  _lastAnnounceTime = 0;
}

void DeviceProtocol::onSend(SendCallback callback) { _sendCallback = callback; }
void DeviceProtocol::onPeerDiscovered(PeerDiscoveredCallback callback) { _peerDiscoveredCallback = callback; }
void DeviceProtocol::onPeerLost(PeerLostCallback callback) { _peerLostCallback = callback; }
void DeviceProtocol::onTextReceived(TextReceivedCallback callback) { _textReceivedCallback = callback; }
void DeviceProtocol::onBinaryReceived(BinaryReceivedCallback callback) { _binaryReceivedCallback = callback; }

OutgoingPacket DeviceProtocol::preparePacket(uint32_t targetId, PacketType type,
                                             const uint8_t* payload, size_t payloadLength) {
  OutgoingPacket packet;
  packet.type = type;

  _txBuffer[0] = static_cast<uint8_t>(type);
  _txBuffer[1] = (_myId >> 24) & 0xFF;
  _txBuffer[2] = (_myId >> 16) & 0xFF;
  _txBuffer[3] = (_myId >> 8) & 0xFF;
  _txBuffer[4] = _myId & 0xFF;

  if (payload && payloadLength > 0) {
    memcpy(_txBuffer + PACKET_HEADER_SIZE, payload, payloadLength);
  }

  packet.data = _txBuffer;
  packet.length = PACKET_HEADER_SIZE + payloadLength;

  if (targetId == 0) {
    packet.targetAddress = BROADCAST_ADDRESS;
  } else {
    const PeerInfo* peer = getPeer(targetId);
    packet.targetAddress = peer ? peer->address : BROADCAST_ADDRESS;
  }

  return packet;
}

bool DeviceProtocol::sendAnnounce() {
  if (!_initialized || !_sendCallback) return false;
  return _sendCallback(preparePacket(0, PacketType::BINARY, nullptr, 0));
}

bool DeviceProtocol::sendText(uint32_t peerId, const String& text) {
  if (!_initialized || !_sendCallback) return false;
  return _sendCallback(preparePacket(peerId, PacketType::TEXT, (const uint8_t*)text.c_str(), text.length()));
}

bool DeviceProtocol::sendBinary(uint32_t peerId, const uint8_t* data, size_t length) {
  if (!_initialized || !_sendCallback) return false;
  return _sendCallback(preparePacket(peerId, PacketType::BINARY, data, length));
}

void DeviceProtocol::handleIncomingData(uint32_t senderAddress, const uint8_t* data, size_t length) {
  if (!_initialized) return;
  if (length < PACKET_HEADER_SIZE) return;

  PacketType type = static_cast<PacketType>(data[0]);
  uint32_t senderId = ((uint32_t)data[1] << 24) | ((uint32_t)data[2] << 16) |
                      ((uint32_t)data[3] << 8) | data[4];

  const uint8_t* payload = data + PACKET_HEADER_SIZE;
  size_t payloadLength = length - PACKET_HEADER_SIZE;

  bool isNew = (_peers.find(senderId) == _peers.end());
  _peers[senderId] = PeerInfo(senderId, senderAddress);
  _peers[senderId].lastSeen = millis();

  if (isNew && _peerDiscoveredCallback) _peerDiscoveredCallback(_peers[senderId]);

  if (type == PacketType::BINARY && payloadLength == 0) return;

  if (type == PacketType::TEXT && _textReceivedCallback) {
    String text;
    text.reserve(payloadLength + 1);
    for (size_t i = 0; i < payloadLength; i++) text += (char)payload[i];
    _textReceivedCallback(senderId, text);
  }

  if (type == PacketType::BINARY && payloadLength > 0 && _binaryReceivedCallback) {
    _binaryReceivedCallback(senderId, payload, payloadLength);
  }
}

const PeerInfo* DeviceProtocol::getPeer(uint32_t id) const {
  auto it = _peers.find(id);
  return (it != _peers.end()) ? &it->second : nullptr;
}

const PeerInfo* DeviceProtocol::getPeerByIndex(size_t index) const {
  if (index >= _peers.size()) {
    return nullptr;
  }
  size_t currentIndex = 0;
  for (const auto& entry : _peers) {
    if (currentIndex == index) {
      return &entry.second;
    }
    currentIndex++;
  }
  return nullptr;
}

size_t DeviceProtocol::getPeerCount() const {
  return _peers.size();
}

void DeviceProtocol::forEachPeer(std::function<void(const PeerInfo& peer)> callback) const {
  for (const auto& entry : _peers) {
    callback(entry.second);
  }
}

void DeviceProtocol::update() {
  if (!_initialized) return;

  uint32_t now = millis();

  for (auto it = _peers.begin(); it != _peers.end();) {
    if (now - it->second.lastSeen > _peerTimeout) {
      it->second.status = 0;
      uint32_t id = it->first;
      if (_peerLostCallback) _peerLostCallback(_peers[id]);
      it = _peers.erase(it);
    } else {
      it->second.status = 1;
      ++it;
    }
  }

  if (_deviceStatus == DeviceStatus::ONLINE) {
    if (now - _lastAnnounceTime > _announceInterval) {
      uint32_t jitter = random(0, _announceJitter);
      if (now - _lastAnnounceTime > _announceInterval + jitter) {
        sendAnnounce();
        _lastAnnounceTime = now;
      }
    }
  }
}

void DeviceProtocol::setAnnounceInterval(uint32_t ms) { _announceInterval = ms; }
void DeviceProtocol::setPeerTimeout(uint32_t ms) { _peerTimeout = ms; }
void DeviceProtocol::setAnnounceJitter(uint32_t maxMs) { _announceJitter = maxMs; }
