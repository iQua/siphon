/**
 * @file InfoManager.cpp
 * @brief Define the managers that are used to maintain status information, by
 *        the ControllerProxy
 *
 * @author Shuhao Liu <shuhao@ece.toronto.edu>
 *
 * @date 2015-12-28
 * @ingroup controller
 */

#include "controller/node_manager.hpp"

#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "util/config.hpp"


namespace siphon {

NodeManager::NodeManager(AppManager* apps, nodeID_t local_nodeID) :
    local_nodeID_(local_nodeID), apps_(apps) {
  const auto& pseudo_sessions = util::Config::get()->pseudo_session_configs_;
  for (const auto& session : pseudo_sessions) {
    if (session.src == local_nodeID) {
      dst_node_to_sessionID_[session.dst] = session.sessionID;
    }
  }
}

const NodeManager::SetType& NodeManager::getOnlineNodeSet() const {
  return online_node_set_;
}

bool NodeManager::addNodeInfo(nodeID_t nodeID, const std::string& url) {
  if (nodeID == local_nodeID_) return false;

  MapType::iterator i = node_url_map_.find(nodeID);
  if (i != node_url_map_.end()) {
    i->second = url;
    return false;
  }
  else {
    node_url_map_[nodeID] = url;
    return true;
  }
}

NodeManager::StatusType NodeManager::getNodeStatus(nodeID_t nodeID) {
  if (nodeID == local_nodeID_) {
    return {true, "localhost"};
  }

  // Find node URL
  MapType::iterator url_i = node_url_map_.find(nodeID);
  if (url_i == node_url_map_.end()) {
    LOG_ERROR << "Node#" << nodeID << "does not exist in NodeManager!";
    throw;
  }
  // Find online status
  SetType::iterator online_i = online_node_set_.find(nodeID);
  return {(online_i == online_node_set_.end()), url_i->second};
}

void NodeManager::newOnlineNode(nodeID_t nodeID) {
  if (nodeID != local_nodeID_) {
    online_node_set_.insert(nodeID);

    // Start local source if the new node is a pseudo session destination.
    if (dst_node_to_sessionID_.find(nodeID) != dst_node_to_sessionID_.end()) {
      const std::string& session = dst_node_to_sessionID_[nodeID];
      PseudoAppSource* src = (PseudoAppSource*) apps_->getSourceApp(session);
      src->start();
    }
  }
}

const NodeManager::MapType NodeManager::newOnlineNodes(
    const rapidjson::Value& msg) {
  using namespace rapidjson;
  MapType new_node_set;

  // Check for error
  if (!msg.HasMember("NewNode") || !msg["NewNode"].IsArray()) {
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    msg.Accept(writer);
    LOG_ERROR << "NewOnlineNode message is incomplete: " << buffer.GetString();
    return new_node_set;
  }

  // Get the nodeID value
  const Value& node_list = msg["NewNode"];
  for (SizeType j = 0; j < node_list.Size(); ++j) {
    if (!node_list[j].IsObject()) {
      LOG_ERROR << "NewNode List contains invalied nodeID: #" << j;
      continue;
    }
    nodeID_t nodeID = node_list[j]["NodeID"].GetUint();
    string hostname = node_list[j]["Hostname"].GetString();
    addNodeInfo(nodeID, hostname);
    if (nodeID != local_nodeID_) {
      newOnlineNode(nodeID);
      MapType::iterator i = node_url_map_.find(nodeID);
      assert(i != node_url_map_.end());
      new_node_set.insert(*i);
    }
  }

  return new_node_set;
}

void NodeManager::nodeOffline(nodeID_t nodeID) {
  SetType::iterator i = online_node_set_.find(nodeID);
  assert(i != online_node_set_.end());
  online_node_set_.erase(i);
}

nodeID_t NodeManager::nodeOffline(const rapidjson::Value& msg) {
  // Check for error
  if (!msg.HasMember("OfflineNode") || !msg["OfflineNode"].IsUint()) {
    using namespace rapidjson;
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    msg.Accept(writer);
    LOG_ERROR << "OfflineNode message is incomplete: " << buffer.GetString();
  }

  // Get the nodeID value
  nodeID_t nodeID = msg["OfflineNode"].GetUint();
  nodeOffline(nodeID);
  return nodeID;
}


}
