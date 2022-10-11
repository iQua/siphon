//
// Created by Shuhao Liu on 2018-11-07.
//

#include "base/message.hpp"

#include <google/protobuf/io/coded_stream.h>

#include "util/config.hpp"
#include "util/logger.hpp"



namespace siphon {

Message::Message():
    buffer_(util::Config::get()->kMaxMessageSize),
    allocated_buffer_(util::Config::get()->kMaxMessageSize),
    read_buffer_state_(kState01),
    read_buffer_({boost::asio::const_buffer((const void*) &message_size_,
                                            sizeof(MessageSizeType)),
                  boost::asio::const_buffer((const void*) &header_size_,
                                            sizeof(HeaderSizeType)),
                  buffer_.getConstBuffer(),
                  allocated_buffer_.getConstBuffer()}) {
}

boost::asio::mutable_buffer Message::getMessageSizeReceivingBuffer() {
  return boost::asio::mutable_buffer((void*) &message_size_,
                                     sizeof(MessageSizeType));
}

Message::MessageSizeType Message::message_size() const {
  return message_size_;
}

Message::MutableBufferSequence Message::getReceivingBuffer(
    bool include_message_size) {
  if (include_message_size) {
    // For UDP, we don't need the message size before hand.
    return {boost::asio::mutable_buffer((void*) &message_size_,
                                    sizeof(MessageSizeType)),
            boost::asio::mutable_buffer((void*) &header_size_,
                                        sizeof(HeaderSizeType)),
            buffer_.getMutableBuffer()};
  }
  else {
    return {boost::asio::mutable_buffer((void*) &header_size_,
                                        sizeof(HeaderSizeType)),
            boost::asio::mutable_buffer(
                (void*) buffer_.data, message_size_ - sizeof(HeaderSizeType))};
  }
}

void Message::fromBuffer(MutableBufferSequence recv_buffer,
    bool include_message_size) {
  int index_offset = 0;
  if (include_message_size) index_offset = 1;
  if (recv_buffer[index_offset].data() != (void*) &header_size_) {
    LOG_WARNING << "Message::FromBuffer() called with a buffer region not owned"
                   " by the Message. It triggers memory copy!";
    boost::asio::buffer_copy(getReceivingBuffer(), recv_buffer);
  }

  // Deserialize header first.
  // message_header_ (if included) and header_size_ are automatically set.
  google::protobuf::io::CodedInputStream stream(
      (const uint8_t*) recv_buffer[1 + index_offset].data(), header_size_);
  assert(header_.ParseFromCodedStream(&stream));
  size_t payload_size = header_.payload_size();
  assert(header_size_ == header_.ByteSize());
  assert(message_size_ == header_size_ + header()->payload_size() +
                          sizeof(HeaderSizeType));

  // Compute read_buffer_.
  read_buffer_state_ = kState00;
  read_buffer_[2] = buffer_.sliceConstBuffer(0, header_size_);
  read_buffer_[3] = buffer_.sliceConstBuffer(header_size_, payload_size);
}

Message::ConstBufferSequence Message::toBuffer() {
  size_t payload_size = read_buffer_[3].size();
  header_.set_payload_size(payload_size);
  header_size_ = (uint16_t) header_.ByteSize();

  // Calculate message size.
  message_size_ = header_size_ + payload_size + sizeof(HeaderSizeType);

  size_t original_header_size = read_buffer_[2].size();
  if (header_size_ <= original_header_size || read_buffer_state_ == kState01) {
    // Payload in allocated_buffer_, serialize new header to buffer_.
    assert(header_.SerializeToArray(buffer_.data, header_size_));
    read_buffer_[2] = buffer_.sliceConstBuffer(0, header_size_);
  }
  else {
    // buffer_ cannot be used by serializing header, move header to
    // allocated_buffer_.
    read_buffer_state_ = kState10;
    assert(header_.SerializeToArray(allocated_buffer_.data, header_size_));
    read_buffer_[2] = allocated_buffer_.sliceConstBuffer(0, header_size_);
  }
  return read_buffer_;
}

const MessageHeader* Message::header() const {
  return &header_;
}

MessageHeader* Message::mutable_header() {
  return &header_;
}

const uint8_t* Message::getPayload() const {
  return (const uint8_t*) read_buffer_[3].data();
}

uint8_t* Message::getMutablePayload() {
  return (uint8_t*) read_buffer_[3].data();
}

size_t Message::getPayloadSize() const {
  return read_buffer_[3].size();
}

boost::asio::mutable_buffer Message::allocateBuffer() {
  return allocated_buffer_.getMutableBuffer();
}

void Message::resetPayload(const uint8_t *data, size_t length) {
  GOOGLE_DCHECK(
      data >= allocated_buffer_.data &&
      data + length <= allocated_buffer_.data + allocated_buffer_.length)
      << "New payload must be a region in allocated buffer!";
  read_buffer_state_ = kState01;
  read_buffer_[3] = boost::asio::const_buffer(data, length);
}

void Message::recycle() {
  read_buffer_state_ = kState01;
  read_buffer_[2] = buffer_.getConstBuffer();
  read_buffer_[3] = allocated_buffer_.getConstBuffer();
}

}
