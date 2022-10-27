//
// Created by Shuhao Liu on 2018-11-15.
//

#ifndef SIPHON_UDP_CODER_FACTORY_HPP
#define SIPHON_UDP_CODER_FACTORY_HPP

#include <string>

#include "coder/fake_udp_coder.hpp"
#include "coder/udp_coder_interfaces.hpp"



namespace siphon {

class UDPEncoderFactory {
 public:
  static std::unique_ptr<IUDPEncoder> create(const std::string& coder_name) {
    if (coder_name == "DirectPass")
      return std::make_unique<DirectPassEncoder>();

    if (coder_name == "test")
      return std::make_unique<test::FakeEncoder>();

    return nullptr;

  }
};

class UDPDecoderFactory {
 public:
  static std::unique_ptr<IUDPDecoder> create(const std::string& coder_name) {
    if (coder_name == "DirectPass")
      return std::make_unique<DirectPassDecoder>();

    if (coder_name == "test")
        return std::make_unique<test::FakeDecoder>();

    return nullptr;
  }
};

}

#endif //SIPHON_UDP_CODER_FACTORY_HPP
