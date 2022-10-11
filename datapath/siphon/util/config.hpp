//
// Created by Shuhao Liu on 2018-03-04.
//

#ifndef SIPHON_CONFIG_HPP
#define SIPHON_CONFIG_HPP

#include <cstdint>
#include <cstddef>
#include <list>
#include <memory>
#include <string>

#include "util/types.hpp"


namespace siphon {
namespace util {

class Config {
 public:
  static const Config* get();

  static Config* getMutable();

  bool loadFromConfigurationFile(const std::string& file_path);

  Config(const Config& rhs) = delete;
  Config& operator=(const Config& rhs) = delete;

  const uint16_t kControllerPortNumber = 6699;

  // Control message-related configurations.
  const size_t kMaxControlMessageSize = 1024;

  // Message-related configurations.
  size_t kMaxMessageSize = 35000;

  // I/O-related configurations.
  bool kUseUDP = true;
  unsigned short kUDPListeningPort = 6868;
  bool kUseTCP = false;
  unsigned short kTCPListeningPort = 6868;

  // Coding-related configurations.
  std::string kUDPCoderName = "";
  std::string kOriDataPath = "oriData.txt";

  int kDefaultUDPCoderDelayConstraint = 10;
  int kDefaultUDPCoderLossRate = -1;
  int kDefaultUDPCoderLossBurst = -1;
  size_t kMessageSize = 30000;
  size_t kPayloadSize = 5;

  // Debug options.
  bool kLocalDebugNoReceivingSocket = false;

  std::list<PseudoSessionConfig> pseudo_session_configs_;

 private:
  // Default constructor. Everything is initialized to their default value.
  Config() = default;

  void configWithJsonObject(const rapidjson::Document& d);

  static Config* instance_;
};

}
}

#endif //SIPHON_CONFIG_HPP
