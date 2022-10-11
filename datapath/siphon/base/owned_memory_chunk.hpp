//
// Created by Shuhao Liu on 2018-11-06.
//

#ifndef SIPHON_BUFFER_HPP
#define SIPHON_BUFFER_HPP

#include <boost/asio.hpp>
#include <memory>

namespace siphon {

struct OwnedMemoryChunk {
  explicit OwnedMemoryChunk(size_t len) : data(new uint8_t[len]), length(len) {}
  ~OwnedMemoryChunk() {
    delete[] data;
  }

  // Non-copyable.
  OwnedMemoryChunk(const OwnedMemoryChunk &rhs) = delete;
  OwnedMemoryChunk &operator=(const OwnedMemoryChunk &rhs) = delete;

  // Integration with boost::asio::const_buffer.
  boost::asio::const_buffer getConstBuffer() const {
    return boost::asio::const_buffer(data, length);
  }
  boost::asio::const_buffer sliceConstBuffer(
      size_t start_offset, size_t len) const {
    return boost::asio::const_buffer(data + start_offset, len);
  }

  // Integration with boost::asio::mutable_buffer.
  boost::asio::mutable_buffer getMutableBuffer(size_t size = 0) const {
    if (size == 0) {
      size = length;
    }
    return boost::asio::mutable_buffer(data, size);
  }

  uint8_t *data;
  size_t length;
};

}

#endif