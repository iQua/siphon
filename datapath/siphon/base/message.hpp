//
// Created by Shuhao Liu on 2018-11-07.
//

#ifndef SIPHON_MESSAGE_HPP
#define SIPHON_MESSAGE_HPP

#include <array>

#include <proto/cpp/message_header.pb.h>

#include "base/owned_memory_chunk.hpp"

namespace siphon {

class Message {
 public:
  using ConstBufferSequence = std::array<boost::asio::const_buffer, 4>;
  using MutableBufferSequence = std::vector<boost::asio::mutable_buffer>;

  using MessageSizeType = uint32_t;
  using HeaderSizeType = uint16_t;

  Message();

  // Non-copyable.
  Message(const Message &rhs) = delete;
  Message &operator=(const Message &rhs) = delete;

  ~Message() = default;

  // For use as a receiving buffer.
  boost::asio::mutable_buffer getMessageSizeReceivingBuffer();
  MessageSizeType message_size() const;
  // For TCP, we need to read size in first. For UDP, we read the entire
  // datagram.
  MutableBufferSequence getReceivingBuffer(bool include_message_size = false);
  void fromBuffer(MutableBufferSequence recv_buffer,
                  bool include_message_size = false);

  // For use as a sending buffer.
  ConstBufferSequence toBuffer();

  // Access header.
  const MessageHeader* header() const;
  MessageHeader* mutable_header();

  // Access payload.
  const uint8_t* getPayload() const;
  uint8_t* getMutablePayload();
  size_t getPayloadSize() const;

  // Access allocated buffer.
  boost::asio::mutable_buffer allocateBuffer();
  // Change the payload to a region in allocated_buffer_.
  void resetPayload(const uint8_t *data, size_t length);

  // Reset to initial state.
  void recycle();

 private:
  MessageHeader header_;

  MessageSizeType message_size_;
  HeaderSizeType header_size_;

  OwnedMemoryChunk buffer_;
  OwnedMemoryChunk allocated_buffer_;

  enum ReadBufferState {
    kState00 = 0,  // Header, payload both in buffer_.
    kState01 = 1,  // Header in buffer_, payload in allocated_buffer_.
    kState10 = 2,  // Header in allocated_buffer, payload in buffer_.
  };
  ReadBufferState read_buffer_state_;
  ConstBufferSequence read_buffer_;
};

}

#endif //SIPHON_MESSAGE_HPP
