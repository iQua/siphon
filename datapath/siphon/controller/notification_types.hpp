/**
 * @file NotificationTypes.hpp
 * @brief Define all types that are used as NotificationMsgs.
 *
 * @author Shuhao Liu <shuhao@ece.toronto.edu>
 *
 * @ingroup notification
 * @date 2015-11-13
 *
 * @remark This files should include the standard library or boost library ONLY.
 *         Other dependency types should be added as forward declarations.
 */

#ifndef NOTIFICATION_TYPES_H_
#define NOTIFICATION_TYPES_H_

#include <vector>
#include <unordered_set>

#include <boost/function.hpp>
#include <boost/atomic.hpp>

#include "rapidjson/document.h"


namespace siphon {

typedef enum : uint32_t {
  kControlMsgNodeOnline = 1,
  kControlMsgNodeOffline,
  kControlMsgNewSession,
  kControlMsgQuerySessionID,
  kControlMsgRoutingInfo,
  kControlMsgReportRTT,
  kControlMsgReportBandwidth,
  kControlMsgSessionSubscribed,
  kControlMsgSetSessionWeight,
} ControlMsgType;

/**
 * @brief The message used to notify the ControllerProxy to send out a control
 *        message.
 */
typedef struct {
  ControlMsgType type;
  rapidjson::Document msg;
} ControlMsgNotificationType;

}

#endif
