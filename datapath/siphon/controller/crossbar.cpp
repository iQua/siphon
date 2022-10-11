/**
 * @file Crossbar.hpp
 * @brief Crossbar is the key component in Siphon to forward packet to the
 *        its proper destination connection queue.
 *
 * @author Shuhao Liu <shuhao@ece.toronto.edu>
 * @date 2015-12-04
 *
 * @ingroup routing
 */

#include "controller/crossbar.hpp"

#include <mutex>
#include <memory>

#include <boost/lexical_cast.hpp>

#include "util/message_util.hpp"


namespace siphon {

PendingPacketArchive::PendingPacketArchive() {
  // There will not be too many pending sessions...
  pending_minion_map_.reserve(ForwardingTable::kForwardingTableMaxSize >> 3);
}

bool PendingPacketArchive::archiveMinion(const std::string& sessionID,
                                         Minion* minion) {
  assert(sessionID == minion->getData()->header()->session_id());
  // Find existing archive queue (if any)
  MapType::iterator i;
  std::unique_lock<std::shared_mutex> find_guard(map_lock_);
  i = pending_minion_map_.find(sessionID);
  if (i != pending_minion_map_.end()) {
    // Found existing
    if (!i->second) {
      // Found the one that has no queue
      boost::atomic_store(&(i->second), boost::make_shared<QueueType>());
    }
    i->second->push(minion);
    return false;
  }
  // Existing archive not found, we should create one
  std::pair<MapType::iterator, bool> temp;
  temp = pending_minion_map_.emplace(sessionID,
                                     boost::make_shared<QueueType>());
  temp.first->second->push(minion);
  return true;
}

PendingPacketArchive::MapType::mapped_type
PendingPacketArchive::getPendingPacketQueue(const std::string& sessionID) {
  std::shared_lock<std::shared_mutex> find_guard(map_lock_);
  MapType::iterator i = pending_minion_map_.find(sessionID);
  if (i == pending_minion_map_.end()) {
    // Did not find the archive, return a nullptr
    return nullptr;
  }
  else {
    return boost::atomic_load(&(i->second));
  }
}

PendingPacketArchive::MapType::mapped_type
PendingPacketArchive::removeSessionFromArchive(const std::string& sessionID) {
  MapType::mapped_type q = getPendingPacketQueue(sessionID);
  if (q) {
    std::unique_lock<std::shared_mutex> delete_guard(map_lock_);
    pending_minion_map_.erase(sessionID);
  }
  return q;
}

PendingPacketArchive::MapType::mapped_type
PendingPacketArchive::resetSessionFromArchive(const std::string& sessionID) {
  std::shared_lock<std::shared_mutex> find_guard(map_lock_);
  MapType::iterator i = pending_minion_map_.find(sessionID);
  if (i == pending_minion_map_.end()) {
    // Did not find the archive, return a nullptr
    return nullptr;
  }
  else {
    MapType::mapped_type q = boost::atomic_load(&(i->second));
    boost::atomic_store(&(i->second), MapType::mapped_type());
    return q;
  }
}

Crossbar::Crossbar(boost::asio::io_context& ios,
    MinionPool* pool,
    IMinionStop* local_app,
    IConnectionManager* conn_manager) :
    INotification<ControlMsgNotificationType>(),
    forwarding_table_(ios),
    pool_(pool),
    local_app_(local_app),
    conn_manager_(conn_manager) {
}

void Crossbar::installForwardingTableEntry(const rapidjson::Value& v) {
  std::pair<std::string, ForwardingTableEntry::ptr> temp;

  // Try parse and construct the new entry
  try {
    temp = forwarding_table_.newEntry(v);
  }
  catch (const std::exception& e) {
    LOG_ERROR << "Invalid ForwardingTableEntry Message: " << e.what();
    return;
  }
  catch (...) {
    throw;
  }

  boost::shared_ptr<MPSCQueue<Minion*>> q
      = packet_archive_.getPendingPacketQueue(temp.first);
  if (q) {
    // Already there, pop all pending packet and send out
    // Since controller messages are process in serial, one consumer policy is
    // guaranteed to be satisfied
    MPSCQueue<Minion*>::Node* node_packet = q->popAll();
    handleArchivedPacket(node_packet, temp.second);
  }
  // Install the rule directly
  forwarding_table_.insertEntry(temp.first, temp.second);
  // Then remove the entry from the archive
  packet_archive_.removeSessionFromArchive(temp.first);
}

IMinionStop* Crossbar::process(Minion* minion) {
  if (!minion->getData()->getPayload()) {
    return pool_;
  }
  const std::string& sessionID = minion->getData()->header()->session_id();

  // If it is a test session for direct path selection.
  // if (util::IsNodeID(sessionID)) {
  //   nodeID_t dst = boost::lexical_cast<nodeID_t>(sessionID);
  //   if (dst == local_nodeID_) return local_app_;
  //   IMinionStop* sender = conn_manager_->getSender(dst);
  //   if (sender != nullptr) return sender;
  // }

  ForwardingTableEntry::result_t nexthops
      = forwarding_table_.getNextHop(sessionID);
  // Not found, try parent sessionID_
   if (nexthops.empty()) {
     auto parent_sessionID = util::GetParentSessionID(sessionID);
     if (parent_sessionID != sessionID) {
       nexthops = forwarding_table_.getNextHop(parent_sessionID);
     }
  }

  // If still empty, archive the minion and return nullptr
  if (nexthops.empty()) {
    if (packet_archive_.archiveMinion(sessionID, minion)) {
      queryForwardingTableEntry(sessionID);
    }
    return nullptr;
  }
  else {
    // Clear pending packets in the packet_archive_
    boost::shared_ptr<MPSCQueue<Minion*>> q
        = packet_archive_.resetSessionFromArchive(sessionID);
    if (q) {
      MPSCQueue<Minion*>::Node* node_packet = q->popAll();
      handleArchivedPacket(node_packet, forwarding_table_.getEntry(sessionID));
    }
    // Dispatch the current message to `nexthops`
    // TODO: Remove support for multicast
    nodeID_t nodeID = *(nexthops.begin());
    if (nodeID == local_nodeID_) {
      return local_app_;
    }
    else {
      return conn_manager_->getSender(nodeID);
    }
  }
}

void Crossbar::handleArchivedPacket(
    MPSCQueue<Minion*>::Node* packet_node,
    ForwardingTableEntry::ptr entry) {
  ForwardingTableEntry::result_t nexthops = entry->getNextHop();
  // TODO: Remove support for multicast
  Minion* packet = packet_node->data;
  nodeID_t nodeID = *(nexthops.begin());
  if (nodeID == local_nodeID_) {
    packet->wakeup(local_app_);
  }
  else {
    IMinionStop* stop = conn_manager_->getSender(nodeID);
    packet->wakeup(stop);
  }
}

void Crossbar::queryForwardingTableEntry(const std::string& sessionID) {
  // Allocate the message
  std::shared_ptr<ControlMsgNotificationType> nexthop_msg =
      std::make_shared<ControlMsgNotificationType>();
  nexthop_msg->type = kControlMsgRoutingInfo;

  // Modify the content of the JSON message
  using namespace rapidjson;
  Document::AllocatorType& alloc = nexthop_msg->msg.GetAllocator();
  Value& v = nexthop_msg->msg;
  v.SetObject();
  v.AddMember("SessionID", Value().SetString(sessionID.c_str(), alloc), alloc);

  // Emit the notification
  LOG_INFO << "Querying Forwarding Info for session# " << sessionID;
  INotification<ControlMsgNotificationType>::notify(nexthop_msg);
}


}
