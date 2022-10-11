//
// Created by Shuhao Liu on 2018-03-20.
//

#include "networking/udp_connection_manager.hpp"
#include <boost/lexical_cast.hpp>


namespace siphon {

using boost::asio::ip::udp;
using boost::asio::ip::address_v4;

UDPConnectionManager::UDPConnectionManager(boost::asio::io_context& ios,
                                           MinionPool* pool,
                                           nodeID_t local_nodeID) :
    ios_(ios), local_nodeID_(local_nodeID), pool_(pool), crossbar_(nullptr) {
}

void UDPConnectionManager::init(IMinionStop* crossbar) {
  crossbar_ = crossbar;
  if (!util::Config::get()->kLocalDebugNoReceivingSocket) {
    receiver_ = std::make_unique<UDPReceiver>(ios_, pool_, crossbar_);
    receiver_->startReceiving();
  }
}

bool UDPConnectionManager::shouldInitiateConnectionTo(nodeID_t /*nodeID*/) {
  return true;
}

ISender* UDPConnectionManager::create(const std::string& url,
                                      nodeID_t peer_nodeID) {
  ISender* sender = nullptr;
  {
    std::unique_lock<std::shared_mutex> grd(mtx_);
    senders_[peer_nodeID] = std::make_unique<UDPSender>(
        ios_, pool_, local_nodeID_, peer_nodeID, url);
    sender = senders_[peer_nodeID].get();
  }
  assert(sender);
  if (!util::Config::get()->kLocalDebugNoReceivingSocket) {
    receiver_->setupPeer(peer_nodeID, sender);
  }
  return sender;
}

ISender* UDPConnectionManager::getSender(nodeID_t nodeID) {
  std::shared_lock<std::shared_mutex> grd(mtx_);
  auto itr = senders_.find(nodeID);
  if (itr == senders_.end()) return nullptr;
  else return itr->second.get();
}

IReceiver* UDPConnectionManager::getReceiver(nodeID_t nodeID) {
  return receiver_.get();
}

void UDPConnectionManager::remove(nodeID_t nodeID) {
  receiver_->removePeer(nodeID);
  std::unique_lock<std::shared_mutex> grd(mtx_);
  senders_.erase(nodeID);
}

}
