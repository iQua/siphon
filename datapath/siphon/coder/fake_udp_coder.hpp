//
// Created by Shuhao Liu on 2018-11-17.
//

#ifndef SIPHON_FAKE_UDP_CODER_HPP
#define SIPHON_FAKE_UDP_CODER_HPP

#include "coder/udp_coder_interfaces.hpp"

namespace siphon{
namespace test {

class FakeEncoder : public IUDPEncoder {
 public:
  FakeEncoder() {
    params_.set(1, 1, 1, 0);
  }
  bool encode(Minion* minion) override {
    minion->getData()->mutable_header()->set_coding_parameters(
        params_.readEncodedAndIncrement());
    return true;
  }
  uint32_t getCodingParams() const {
    return params_.readEncoded();
  }
};

class FakeDecoder : public IUDPDecoder {
 public:
  FakeDecoder() = default;
  bool decode(Minion* minion) override {
    uint32_t params = minion->getData()->header()->coding_parameters();
    params += 0x1010101;
    params_.set(params);
    return true;
  }
};

}
}

#endif //SIPHON_FAKE_UDP_CODER_HPP
