//
// Created by Shuhao Liu on 2018-07-13.
//

#include "coder/udp_coder_interfaces.hpp"


namespace siphon {

void IUDPEncoder::setParameters(uint32_t params) {
  // Set parameters by resetting the counter.
  params_.set(params, true);
}

uint32_t IUDPDecoder::getEncodedParameters() {
  return params_.readEncoded();
}

Message* IUDPDecoder::allocateExtraMessage(Minion* minion) {
  std::unique_ptr<Message>& new_msg =
      minion->getExtraData()->emplace_back(std::make_unique<Message>());
  return new_msg.get();
}

bool DirectPassEncoder::encode(Minion* minion) {
  Message* message = minion->getData();
  // Copy the header.
  *(encoded_message_->mutable_header()) = *(message->header());
  // Copy the payload.
  boost::asio::mutable_buffer dst_buf = encoded_message_->allocateBuffer();
  boost::asio::buffer_copy(dst_buf, boost::asio::buffer(
      message->getPayload(), message->getPayloadSize()));
  encoded_message_->resetPayload(
      (uint8_t*) dst_buf.data(), message->getPayloadSize());

  // Set coding parameters in the header.
  encoded_message_->mutable_header()->set_coding_parameters(
      params_.readEncoded());

  // Swap the Message object and recycle the one in Encoder.
  minion->swapData(encoded_message_);
  encoded_message_->recycle();
  return true;
}

bool DirectPassDecoder::decode(Minion* minion) {
  Message* message = minion->getData();
  // Copy the header.
  *(decoded_message_->mutable_header()) = *(message->header());
  // Copy the payload.
  boost::asio::mutable_buffer dst_buf = decoded_message_->allocateBuffer();
  boost::asio::buffer_copy(dst_buf, boost::asio::buffer(
      message->getPayload(), message->getPayloadSize()));
  decoded_message_->resetPayload((uint8_t*) dst_buf.data(), message->getPayloadSize());

  // Swap the Message object and recycle the one in Encoder.
  minion->swapData(decoded_message_);
  decoded_message_->recycle();
  return true;
}

}
