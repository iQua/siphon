//
// Created by Shuhao Liu on 2018-11-06.
//

#ifndef SIPHON_MESSAGE_UTIL_HPP
#define SIPHON_MESSAGE_UTIL_HPP

#include <stdint.h>
#include <string>


namespace siphon {
namespace util {

int64_t CurrentUnixMicros();

bool IsSpecialSession(const std::string& sessionID);

/**
 * @brief Get the sessionID for the parent session, used for universal
 *        matching for sessionIDs.
 *
 * @param sessionID The child sessionID.
 *
 * @return The parent sessionID, which is the the beginning of the child
 *         sessionID before the separator `@`.
 */
std::string GetParentSessionID(const std::string& sessionID);

bool IsNodeID(const std::string& s);

}
}

#endif //SIPHON_MESSAGE_UTIL_HPP
