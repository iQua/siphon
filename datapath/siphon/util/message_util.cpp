//
// Created by Shuhao Liu on 2018-11-06.
//

#include "util/message_util.hpp"

#include <algorithm>
#include <chrono>


namespace siphon {
namespace util {

int64_t CurrentUnixMicros() {
  std::chrono::system_clock::time_point now =
      std::chrono::system_clock::now();
  return std::chrono::duration_cast<std::chrono::microseconds>(
      now.time_since_epoch()).count();
}

bool IsSpecialSession(const std::string& sessionID) {
  return sessionID.length() == 0;
}

std::string GetParentSessionID(const std::string& sessionID) {
  size_t separator_index = sessionID.find('@');
  if (separator_index == std::string::npos) {
    return sessionID;
  }
  else {
    return sessionID.substr(0, separator_index);
  }
}

bool IsNodeID(const std::string& s) {
  return !s.empty() &&
      std::find_if(s.begin(),
                   s.end(),
                   [](char c) {
                     return !std::isdigit(c);
                   }) == s.end();
}

}
}