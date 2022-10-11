//
// Created by Shuhao Liu on 2018-03-20.
//

#include "networking/tcp_connection_manager.hpp"

#include <boost/lexical_cast.hpp>

#include "util/config.hpp"


namespace siphon {

using boost::asio::ip::tcp;
using boost::asio::ip::address_v4;

TCPAcceptor::TCPAcceptor(boost::asio::io_context& ios)
    : ios_(ios), acceptor_(ios) {
}

void TCPAcceptor::listen(int backlog) {
  using namespace boost::asio::ip;

  // Resolve the local endpoint
  tcp::resolver resolver(ios_);
  tcp::resolver::query query(tcp::v4(), boost::lexical_cast<std::string>(
      util::Config::get()->kTCPListeningPort));
  tcp::endpoint listenEndpoint = *resolver.resolve(query);

  // Start listening on acceptor_
  acceptor_.open(listenEndpoint.protocol());
  acceptor_.set_option(tcp::acceptor::reuse_address(true));
  acceptor_.bind(listenEndpoint);
  acceptor_.listen(backlog);
}

void TCPAcceptor::startAccept(
    std::shared_ptr<tcp::socket> socket,
    boost::function<void (const boost::system::error_code&)> on_accept) {
  acceptor_.async_accept(*socket, on_accept);
}

void TCPAcceptor::close(boost::system::error_code& ec) {
  if (acceptor_.is_open()) {
    acceptor_.cancel(ec);
    if (ec && ec != boost::asio::error::operation_not_supported) {
      return;
    }
    acceptor_.close(ec);
    if (ec) {
      return;
    }
  }
}

TCPConnectionManager::TCPConnectionManager(boost::asio::io_context& ios,
                                           MinionPool* pool,
                                           nodeID_t local_nodeID) :
    ios_(ios), local_nodeID_(local_nodeID), pool_(pool), crossbar_(nullptr),
    acceptor_(ios) {
}

void TCPConnectionManager::init(IMinionStop* crossbar) {
  crossbar_ = crossbar;
  if (!util::Config::get()->kLocalDebugNoReceivingSocket) {
    listening_socket_ = std::make_shared<tcp::socket>(ios_);
    acceptor_.listen();
    acceptor_.startAccept(
        listening_socket_,
        [this](const boost::system::error_code& ec) {
          if (ec) {
            LOG_ERROR << "TCP accept connection error: " << ec.message();
            return;
          }
          onNewIncomingConnection();
        });
  }
}

bool TCPConnectionManager::shouldInitiateConnectionTo(nodeID_t nodeID) {
  assert(local_nodeID_ != nodeID);
  if ((local_nodeID_ + nodeID) % 2 == 0) {
    return nodeID > local_nodeID_;
  }
  else {
    return nodeID < local_nodeID_;
  }
}

ISender* TCPConnectionManager::create(const std::string& url,
                                      nodeID_t peer_nodeID) {
  assert(shouldInitiateConnectionTo(peer_nodeID));
  TCPSender* sender = nullptr;
  {
    std::unique_lock<std::shared_mutex> grd(mtx_);
    // Create sender.
    senders_[peer_nodeID] = std::make_unique<TCPSender>(
        ios_, pool_, local_nodeID_, peer_nodeID, url);
    sender = senders_[peer_nodeID].get();

    // Create and start receiver.
    auto socket = sender->socket();
    if (!util::Config::get()->kLocalDebugNoReceivingSocket) {
      receivers_[peer_nodeID] = std::make_unique<TCPReceiver>(
          socket, peer_nodeID, pool_, crossbar_);
      receivers_[peer_nodeID]->startReceiving();
    }
  }
  assert(sender);
  return sender;
}

ISender* TCPConnectionManager::getSender(nodeID_t nodeID) {
  std::shared_lock<std::shared_mutex> grd(mtx_);
  auto itr = senders_.find(nodeID);
  if (itr == senders_.end()) return nullptr;
  else return itr->second.get();
}

IReceiver* TCPConnectionManager::getReceiver(nodeID_t nodeID) {
  std::shared_lock<std::shared_mutex> grd(mtx_);
  auto itr = receivers_.find(nodeID);
  if (itr == receivers_.end()) return nullptr;
  else return itr->second.get();
}

void TCPConnectionManager::remove(nodeID_t nodeID) {
  std::unique_lock<std::shared_mutex> grd(mtx_);
  senders_.erase(nodeID);
  receivers_.erase(nodeID);
}

void TCPConnectionManager::onNewIncomingConnection() {
  nodeID_t peer_nodeID = 0;
  boost::system::error_code ec;
  boost::asio::read(
      *listening_socket_,
      boost::asio::buffer((void*) &peer_nodeID, sizeof(nodeID_t)),
      ec);
  if (peer_nodeID == 0 || ec) {
    LOG_ERROR << "Connection accepted, but receiving peer nodeID failed: "
              << ec.message();
    exit(1);
  }

  LOG_INFO << "TCP connection accepted from nodeID: " << (int) peer_nodeID;
  {
    std::unique_lock<std::shared_mutex> grd(mtx_);
    // Create and start sender.
    senders_[peer_nodeID] = std::make_unique<TCPSender>(
        ios_, pool_, local_nodeID_, peer_nodeID, listening_socket_);
    senders_[peer_nodeID]->startSending();
    // Create and start receiver.
    receivers_[peer_nodeID] = std::make_unique<TCPReceiver>(
        listening_socket_, peer_nodeID, pool_, crossbar_);
    receivers_[peer_nodeID]->startReceiving();
  }

  // Start next round of accepting.
  listening_socket_ = std::make_shared<tcp::socket>(ios_);
  acceptor_.startAccept(
      listening_socket_,
      [this](const boost::system::error_code& ec) {
        if (ec) {
          LOG_ERROR << "TCP accept connection error: " << ec.message();
          return;
        }
        onNewIncomingConnection();
      });
}

}
