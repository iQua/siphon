//
// Created by Shuhao Liu on 2018-03-20.
//

#ifndef SIPHON_UDP_CODER_INTERFACES_HPP
#define SIPHON_UDP_CODER_INTERFACES_HPP

#include "base/message.hpp"
#include "base/minion_pool.hpp"
#include "coder/coding_parameters.hpp"

#include <map>


namespace siphon {

class IUDPEncoder {
 public:
  virtual ~IUDPEncoder() = default;

  // Return true iff there is at least one message encoded.
  // Note: must write coding parameter to the message header.
  virtual bool encode(Minion* minion) = 0;

  void setParameters(uint32_t params);

 protected:
  CodingParameters params_;
};

class IUDPDecoder {
 public:
  virtual ~IUDPDecoder() = default;

  // Return true iff there is at least one message decoded.
  virtual bool decode(Minion* minion) = 0;

  uint32_t getEncodedParameters();

 protected:
  Message* allocateExtraMessage(Minion* minion);

 protected:
  CodingParameters params_;
};


class DirectPassEncoder : public IUDPEncoder {
 public:
  DirectPassEncoder() : encoded_message_(std::make_unique<Message>()) {}
  ~DirectPassEncoder() override = default;

  bool encode(Minion* minion) override;

 private:
  std::unique_ptr<Message> encoded_message_;
};


class DirectPassDecoder : public IUDPDecoder {
 public:
  DirectPassDecoder() : decoded_message_(std::make_unique<Message>()) {};
  ~DirectPassDecoder() override = default;

  bool decode(Minion* minion) override;

 private:
  std::unique_ptr<Message> decoded_message_;
};


}

#endif //SIPHON_UDPCODER_HPP
