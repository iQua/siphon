/**
 * @file Types.hpp
 * @brief All custom type defines
 *
 * @author Shuhao Liu (shuhao@ece.toronto.edu)
 * @date November 2, 2015
 *
 * @ingroup fundamentals
 */

#ifndef TYPES_HPP
#define TYPES_HPP

#include <string>

#include "rapidjson/document.h"


namespace siphon {

/**
 * @brief Define the node identifier type.
 */
typedef uint32_t nodeID_t;

struct PseudoSessionConfig {
  PseudoSessionConfig(const std::string& sessionID_,
                      const std::string& sessionType_,
                      const std::string& originalDataPath_,
                      nodeID_t src_,
                      nodeID_t dst_,
                      double rate,
                      size_t burst,
                      size_t size,
                      size_t payload_length) :
      sessionID(sessionID_), sessionType(sessionType_), originalDataPath(originalDataPath_), src(src_), dst(dst_),
      average_rate(rate), burst_size(burst), message_size(size), payload_size(payload_length) {}

  std::string sessionID;
  std::string sessionType;
  std::string originalDataPath;
  nodeID_t src;
  nodeID_t dst;
  double average_rate;
  size_t burst_size;
  size_t message_size;
  size_t payload_size;
};

}

#endif
