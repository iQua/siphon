//
// Created by Shuhao Liu on 2018-11-22.
//

#ifndef SIPHON_TCP_CONNECTION_HPP
#define SIPHON_TCP_CONNECTION_HPP

#include "networking/connection_interfaces.hpp"

#include <memory>


namespace siphon {

using boost::asio::ip::tcp;
using boost::asio::ip::address_v4;

class TCPSenderSocketWrapper : public ISenderSocketWrapper {
 public:
  TCPSenderSocketWrapper(boost::asio::io_context& ios,
                         MinionPool* pool,
                         MinionStopQueueType* queue);
  TCPSenderSocketWrapper(std::shared_ptr<tcp::socket> socket,
                         MinionPool* pool,
                         MinionStopQueueType* queue);

  IMinionStop* process(Minion* minion) override;

  std::shared_ptr<tcp::socket> socket() {
    return socket_;
  }

 protected:
  void onDataSent(const boost::system::error_code& ec,
                  size_t bytes_transferred) override;

 private:
  std::shared_ptr<tcp::socket> socket_;

  Minion* current_minion_;
};

class TCPSender : public ISender {
 public:
  TCPSender(boost::asio::io_context& ios,
            MinionPool* pool,
            nodeID_t local_nodeID,
            nodeID_t peer_nodeID,
            const std::string& peer_url);
  TCPSender(boost::asio::io_context& ios,
            MinionPool* pool,
            nodeID_t local_nodeID,
            nodeID_t peer_nodeID,
            std::shared_ptr<tcp::socket> connected_socket);

  IMinionStop* process(Minion* minion) override;

  void startSending() override;

  void onAck(Message* ack) override;

  std::shared_ptr<tcp::socket> socket() {
    return socket_wrapper_.socket();
  }

 protected:
  bool resolveAndConnect(const std::string& url);

 private:
  MinionStopQueueType q_;

  TCPSenderSocketWrapper socket_wrapper_;

  const nodeID_t local_nodeID_;
  const nodeID_t peer_nodeID_;
};

class TCPReceiver : public IReceiver {
 public:
  TCPReceiver(std::shared_ptr<tcp::socket> socket,
              nodeID_t peer_nodeID,
              MinionPool* pool,
              IMinionStop* crossbar);

  IMinionStop* process(Minion* minion) override;

  void startReceiving() override;

 protected:
  void onMessageSizeReceived(const boost::system::error_code& error,
                             std::size_t bytes_transferred);

  void onDataReceived(const boost::system::error_code& error,
                      std::size_t bytes_transferred) override;

 private:
  std::shared_ptr<tcp::socket> socket_;

  const nodeID_t peer_nodeID_;

  IMinionStop* crossbar_;

  std::unique_ptr<Message> pending_msg_;
};

}

#endif //SIPHON_TCP_CONNECTION_HPP
