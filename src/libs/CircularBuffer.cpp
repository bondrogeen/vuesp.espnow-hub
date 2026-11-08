#include <cstddef>
#include <cstdint>

#include "./init/init.h"

template <typename T, size_t SIZE>
class CircularBuffer {
 private:
  Buffer& buffer;

 public:
  explicit CircularBuffer(Buffer& buf) : buffer(buf) {
    buffer.head = 0;
    buffer.tail = 0;
    buffer.count = 0;
  }

  bool push(const T& item) {
    if (isFull()) {
      buffer.tail = (buffer.tail + 1) % SIZE;
    } else {
      buffer.count++;
    }
    buffer.data[buffer.head] = item;
    buffer.head = (buffer.head + 1) % SIZE;
    return true;
  }

  bool pop(T& item) {
    if (isEmpty()) {
      return false;
    }
    item = static_cast<T>(buffer.data[buffer.tail]);
    buffer.tail = (buffer.tail + 1) % SIZE;
    buffer.count--;

    return true;
  }

  bool peek(T& item, size_t index = 0) const {
    if (index >= buffer.count) {
      return false;
    }

    size_t actualIndex = (buffer.tail + index) % SIZE;
    item = static_cast<T>(buffer.data[actualIndex]);
    return true;
  }

  bool getFromNewest(T& item, size_t index = 0) const {
    if (index >= buffer.count) {
      return false;
    }

    size_t actualIndex = (buffer.head - 1 - index + SIZE) % SIZE;
    item = static_cast<T>(buffer.data[actualIndex]);
    return true;
  }

  bool isEmpty() const {
    return (buffer.count == 0);
  }

  bool isFull() const {
    return (buffer.count == SIZE);
  }

  size_t getSize() const {
    return buffer.count;
  }

  size_t getCapacity() const {
    return SIZE;
  }

  void clear() {
    buffer.head = 0;
    buffer.tail = 0;
    buffer.count = 0;
  }

  void getAll(T* outputArray, size_t& outputCount) const {
    outputCount = buffer.count;
    for (size_t i = 0; i < buffer.count; i++) {
      size_t index = (buffer.tail + i) % SIZE;
      outputArray[i] = static_cast<T>(buffer.data[index]);
    }
  }

  Buffer& getBuffer() const {
    return buffer;
  }
};