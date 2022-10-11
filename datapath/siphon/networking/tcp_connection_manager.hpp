//
// Created by Shuhao Liu on 2018-03-18.
//

#ifndef SIPHON_TCPPORT_HPP
#define SIPHON_TCPPORT_HPP

#include <shared_mutex>
#include <unordered_map>

#include <boost/function.hpp>

#include "networking/tcp_connection.hpp"


namespace siphon {

using boost::asio::ip::tcp;
using boost::asio::ip::address_v4;

class TCPAcceptor {
 public:
  explicit TCPAcceptor(boost::asio::io_context& ios);

  /**
   * @brief Start listening on a given port.
   *
   * @param listen_port The port number to listen to.
   */
  void listen(int backlog = boost::asio::socket_base::max_connections);

  /**
   * @brief Create a new connection to accept the connect request on the
   *        listening port.
   */
  void startAccept(
      std::shared_ptr<tcp::socket> socket,
      boost::function<void (const boost::system::error_code&)> on_accept);

  /**
   * @brief Clean up everything.
   */
  void close(boost::system::error_code& ec);

 protected:
  /**
   * @brief Reference to the global `boost::asio::io_context` object.
   */
  boost::asio::io_context& ios_;

  /**
   * @brief The Acceptor instance to accept connect request.
   */
  tcp::acceptor acceptor_;
};

class TCPConnectionManager : public IConnectionManager {
 public:
  TCPConnectionManager(boost::asio::io_context& ios,
                       MinionPool* pool,
                       nodeID_t local_nodeID);

  ~TCPConnectionManager() override = default;

  void init(IMinionStop* crossbar) override;

  bool shouldInitiateConnectionTo(nodeID_t nodeID) override;

  ISender* create(const std::string& url, nodeID_t nodeID) override;

  ISender* getSender(nodeID_t nodeID) override;
  IReceiver* getReceiver(nodeID_t nodeID) override;

  void remove(nodeID_t nodeID) override;

 protected:
  void onNewIncomingConnection();

 private:
  boost::asio::io_context& ios_;

  const nodeID_t local_nodeID_;

  MinionPool* pool_;

  IMinionStop* crossbar_;

  std::shared_ptr<tcp::socket> listening_socket_;
  TCPAcceptor acceptor_;

  std::unordered_map<nodeID_t, std::unique_ptr<TCPSender>> senders_;
  std::unordered_map<nodeID_t, std::unique_ptr<TCPReceiver>> receivers_;
  std::shared_mutex mtx_;
};


}

#endif