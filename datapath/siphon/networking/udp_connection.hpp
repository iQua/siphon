//
// Created by Shuhao Liu on 2018-03-18.
//

#ifndef SIPHON_UDPCONNECTION_HPP
#define SIPHON_UDPCONNECTION_HPP

#include <mutex>
#include <unordered_map>

#include <boost/thread.hpp>

#include "coder/udp_coder_interfaces.hpp"
#include "networking/connection_interfaces.hpp"
#include "util/config.hpp"

namespace siphon {

using boost::asio::ip::udp;
using boost::asio::ip::address_v4;

class UDPSenderSocketWrapper : public ISenderSocketWrapper {
 public:
  UDPSenderSocketWrapper(boost::asio::io_context& ios,
                         MinionPool* pool,
                         MinionStopQueueType* queue);

  IMinionStop* process(Minion* minion) override;

  udp::socket* socket() {
    return &socket_;
  }

 protected:
  void onDataSent(const boost::system::error_code& ec,
                  size_t bytes_transferred) override;

 private:
  udp::socket socket_;

  Minion* current_minion_;
};

class UDPSender: public ISender {
 public:
  friend class UDPConnectionTest_CanReceiveDataCorrectly_Test;

  UDPSender(boost::asio::io_context& ios,
            MinionPool* pool,
            nodeID_t local_nodeID,
            nodeID_t peer_nodeID,
            const std::string& peer_url);

  IMinionStop* process(Minion* minion) override;

  void startSending() override;

  void onAck(Message* ack) override;

 protected:
  bool resolveAndConnect(const std::string& url);

 private:
  MinionStopQueueType q_;

  UDPSenderSocketWrapper socket_wrapper_;

  const nodeID_t local_nodeID_;
  const nodeID_t peer_nodeID_;

  std::unordered_map<std::string, std::unique_ptr<IUDPEncoder>> encoders_;
  std::mutex mtx_;
};


class UDPReceiver : public IReceiver {
 public:
  friend class UDPConnectionTest_CanReceiveDataCorrectly_Test;

  UDPReceiver(boost::asio::io_context& ios,
              MinionPool* pool,
              IMinionStop* crossbar);

  IMinionStop* process(Minion* minion) override;

  void startReceiving() override;

  void setupPeer(nodeID_t nodeID, ISender* sender);
  void removePeer(nodeID_t nodeID);

 protected:
  void onDataReceived(const boost::system::error_code& error,
                      std::size_t bytes_transferred) override;

 private:
  boost::asio::ip::udp::socket socket_;

  IMinionStop* crossbar_;

  std::unique_ptr<Message> pending_msg_;
  OwnedMemoryChunk serialized_ack_;

  std::unordered_map<std::string, std::unique_ptr<IUDPDecoder>> decoders_;
  std::unordered_map<nodeID_t, ISender*> senders_;
  std::mutex mtx_;

  boost::asio::ip::udp::endpoint peer_endpoint_;
};



}


#endif //SIPHON_UDPCONNECTION_HPP
