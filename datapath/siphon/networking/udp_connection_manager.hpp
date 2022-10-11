//
// Created by Shuhao Liu on 2018-03-18.
//

#ifndef SIPHON_UDPPORT_HPP
#define SIPHON_UDPPORT_HPP

#include <shared_mutex>
#include <unordered_map>

#include "networking/udp_connection.hpp"


namespace siphon {

using boost::asio::ip::udp;
using boost::asio::ip::address_v4;

class UDPConnectionManager : public IConnectionManager {
 public:
  UDPConnectionManager(boost::asio::io_context& ios,
                       MinionPool* pool,
                       nodeID_t local_nodeID);

  ~UDPConnectionManager() override = default;

  void init(IMinionStop* crossbar) override;

  bool shouldInitiateConnectionTo(nodeID_t nodeID) override;

  ISender* create(const std::string& url, nodeID_t nodeID) override;

  ISender* getSender(nodeID_t nodeID) override;
  IReceiver* getReceiver(nodeID_t nodeID) override;

  void remove(nodeID_t nodeID) override;

 private:
  boost::asio::io_context& ios_;

  const nodeID_t local_nodeID_;

  MinionPool* pool_;

  IMinionStop* crossbar_;

  std::unique_ptr<UDPReceiver> receiver_;

  std::unordered_map<nodeID_t, std::unique_ptr<ISender>> senders_;

  std::shared_mutex mtx_;
};


}

#endif