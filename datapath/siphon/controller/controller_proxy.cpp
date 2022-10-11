/**
 * @file ControllerProxy.cpp
 * @brief Define the entity used to communicate with the Controller.
 *
 * @author Shuhao Liu <shuhao@ece.toronto.edu>
 *
 * @date 2015-12-22
 * @ingroup controller
 */

#include "controller/controller_proxy.hpp"

#include <string>

#include "rapidjson/writer.h"


namespace siphon {

ControllerProxy::ControllerProxy(boost::asio::io_context& ios,
                                 const std::string& controller_url,
                                 uint16_t controller_port) :
    ISerializedObserver<ControlMsgNotificationType>(ios),
    ControllerConnection(
        ISerializedObserver<ControlMsgNotificationType>::handler_strand_),
    configured_(false) {
  // Connect to the controller
  local_nodeID_ = connectAndStartAtNodeID(controller_url, controller_port);
  LOG_INFO << "Local node start at # " << local_nodeID_;
}

void ControllerProxy::setup(NodeManager* node_manager,
                            Crossbar* crossbar,
                            IConnectionManager* connections) {
  // Set the pointers
  node_manager_ = node_manager;
  crossbar_ = crossbar;
  connections_ = connections;

  configured_ = true;
}

void ControllerProxy::handleNotification(
    NotificationMsgPtrType notification_msg_ptr) {
  assert(configured_);
  using namespace rapidjson;
  Value type_v;
  type_v.SetUint(notification_msg_ptr->type);

  Document msg;
  Document::AllocatorType& alloc = msg.GetAllocator();
  msg.SetObject();
  msg.AddMember("Type", type_v, alloc);
  msg.AddMember("Msg", Value().Move(), alloc);
  msg["Msg"].Swap(notification_msg_ptr->msg);
  sendControlMsg(msg);
}

void ControllerProxy::onControlMsgReceived(const rapidjson::Document& msg) {
  // Check for error in the message
  assert(configured_);
  if (!msg.IsObject() || !msg.HasMember("Type") || !msg["Type"].IsUint()
      || !msg.HasMember("Msg") || !msg["Msg"].IsObject()) {
    using namespace rapidjson;
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    msg.Accept(writer);
    LOG_ERROR << "Error control message received: " << buffer.GetString();
    return;
  }

  // Parse the message type
  auto type = (ControlMsgType) msg["Type"].GetUint();
  const rapidjson::Value& detail = msg["Msg"];
  // Dispatch the message to its proper destination entity
  dispatchControlMsgToComponent(type, detail);
}

void ControllerProxy::dispatchControlMsgToComponent(
    ControlMsgType type, const rapidjson::Value& msg) {
  // Note: we cannot post these operations to io_service, because the msg might
  //       no longer be available
  switch (type) {
    case kControlMsgNodeOnline: {
      NodeManager::MapType nodes = node_manager_->newOnlineNodes(msg);
      for (NodeManager::MapType::iterator i = nodes.begin();
           i != nodes.end(); ++i) {
        nodeID_t new_nodeID = i->first;
        if (new_nodeID == local_nodeID_) continue;
        if (connections_->shouldInitiateConnectionTo(new_nodeID)) {
          connections_->create(i->second, new_nodeID);
          LOG_INFO << "Initiating connection to Node# " << new_nodeID;
        }
      }
      break;
    }

    case kControlMsgNodeOffline: {
      nodeID_t node = node_manager_->nodeOffline(msg);
      connections_->remove(node);
      break;
    }

    case kControlMsgRoutingInfo:
      crossbar_->installForwardingTableEntry(msg);
      break;

    case kControlMsgSetSessionWeight:
      LOG_ERROR << "Unsupported Op: set session weight";
      break;

    default:
      LOG_ERROR << "Unknown ControlMsgType " << type << ", discarding...";
      break;
  }
}


}
