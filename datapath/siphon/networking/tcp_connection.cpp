//
// Created by Shuhao Liu on 2018-11-22.
//

#include "networking/tcp_connection.hpp"

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

#include "util/config.hpp"
#include "util/message_util.hpp"


namespace siphon {

using boost::asio::ip::tcp;

TCPSenderSocketWrapper::TCPSenderSocketWrapper(boost::asio::io_context& ios,
                                               MinionPool* pool,
                                               MinionStopQueueType* queue) :
    ISenderSocketWrapper(pool, queue),
    socket_(std::make_shared<tcp::socket>(ios)) {
}

TCPSenderSocketWrapper::TCPSenderSocketWrapper(
    std::shared_ptr<tcp::socket> socket,
    MinionPool* pool,
    MinionStopQueueType* queue) :
  ISenderSocketWrapper(pool, queue), socket_(socket) {
}


IMinionStop* TCPSenderSocketWrapper::process(Minion* minion) {
  current_minion_ = minion;
  minion->getData()->mutable_header()->set_timestamp(util::CurrentUnixMicros());
  socket_->async_send(
      minion->getData()->toBuffer(),
      boost::bind(&TCPSenderSocketWrapper::onDataSent,
                  this,
                  boost::asio::placeholders::error,
                  boost::asio::placeholders::bytes_transferred));

  // Return null. Send it to pool in the callback.
  return nullptr;
}

void TCPSenderSocketWrapper::onDataSent(const boost::system::error_code& ec,
                                        size_t /*bytes_transferred*/) {
  if (current_minion_->getExtraData()->empty()) {
    // No more message to send. Return the minion back to the pool.
    socket_->get_io_context().post(boost::bind(
        &MinionPool::process, pool_, current_minion_));
    startSending();
    return;
  }

  // Send messages in the extra data list.
  current_minion_->swapData(current_minion_->getExtraData()->front());
  current_minion_->getExtraData()->pop_front();
  process(current_minion_);
}

TCPSender::TCPSender(boost::asio::io_context& ios,
                     MinionPool* pool,
                     nodeID_t local_nodeID,
                     nodeID_t peer_nodeID,
                     const std::string& peer_url) :
    ISender(pool),
    socket_wrapper_(ios, pool, &q_),
    local_nodeID_(local_nodeID),
    peer_nodeID_(peer_nodeID) {
  assert(resolveAndConnect(peer_url));
  socket_wrapper_.startSending();
}

TCPSender::TCPSender(boost::asio::io_context& ios,
                     MinionPool* pool,
                     nodeID_t local_nodeID,
                     nodeID_t peer_nodeID,
                     std::shared_ptr<tcp::socket> connected_socket) :
    ISender(pool),
    socket_wrapper_(connected_socket, pool, &q_),
    local_nodeID_(local_nodeID),
    peer_nodeID_(peer_nodeID) {
  socket_wrapper_.startSending();
}

bool TCPSender::resolveAndConnect(const std::string& url) {
  using boost::asio::ip::tcp;
  tcp::socket* socket = socket_wrapper_.socket().get();
  try {
    tcp::resolver resolver(socket->get_io_context());
    tcp::resolver::query query(url, boost::lexical_cast<std::string>(
        util::Config::get()->kUDPListeningPort));
    tcp::resolver::iterator itr = resolver.resolve(query);
    socket->connect(*itr);
    // Report nodeID to peer.
    socket->write_some(boost::asio::buffer((void*) &local_nodeID_,
                                           sizeof(nodeID_t)));
  }
  catch (boost::system::error_code& error) {
    LOG_ERROR << "TCP connect/report nodeID error: " << error.message();
    return false;
  }
  return true;
}

IMinionStop* TCPSender::process(Minion* minion) {
  // TODO: modify payload size here.

  // Modify message header to send.
  auto set_header_fields = [this](Message* message) -> void {
    MessageHeader* header = message->mutable_header();
    header->set_payload_size(message->getPayloadSize());
    header->set_ack(false);
    header->set_src(local_nodeID_);
    header->set_dst(peer_nodeID_);
    // timestamp is set before sending.
  };

  // Set the header for message.
  set_header_fields(minion->getData());
  for (auto& msg : *(minion->getExtraData())) {
    set_header_fields(msg.get());
  }
  return &q_;
}

void TCPSender::startSending() {
  socket_wrapper_.startSending();
}

void TCPSender::onAck(Message* ack) {
  LOG_ERROR << "TCP::onAck() should not be called! Dying...";
  exit(1);
}

TCPReceiver::TCPReceiver(std::shared_ptr<tcp::socket> socket,
                         nodeID_t peer_nodeID,
                         MinionPool* pool,
                         IMinionStop* crossbar) :
    IReceiver(pool),
    socket_(socket),
    peer_nodeID_(peer_nodeID),
    crossbar_(crossbar),
    pending_msg_(std::make_unique<Message>()) {
}

IMinionStop* TCPReceiver::process(Minion* minion) {
  minion->swapData(pending_msg_);
  return crossbar_;
}

void TCPReceiver::startReceiving() {
  pending_msg_->recycle();
  // Receive message size first.
  boost::asio::async_read(
      *socket_,
      pending_msg_->getMessageSizeReceivingBuffer(),
      boost::bind(&TCPReceiver::onMessageSizeReceived, this,
                  boost::asio::placeholders::error(),
                  boost::asio::placeholders::bytes_transferred()));
}

void TCPReceiver::onMessageSizeReceived(const boost::system::error_code& error,
                                        std::size_t bytes_transferred) {
  if (error) {
    LOG_ERROR << "Error receiving: " << error.message();
    return;
  }

  assert(bytes_transferred == sizeof(Message::MessageSizeType));
  assert(pending_msg_->message_size() > 0);
  boost::asio::async_read(
      *socket_,
      pending_msg_->getReceivingBuffer(false),  // Not include message size.
      boost::bind(&TCPReceiver::onDataReceived, this,
                  boost::asio::placeholders::error(),
                  boost::asio::placeholders::bytes_transferred()));
}

void TCPReceiver::onDataReceived(const boost::system::error_code& error,
                                 size_t /*bytes_transferred*/) {
  if (error) {
    LOG_ERROR << "TCP receiving error from Node [" << peer_nodeID_
              << "]: " << error.message();
  }

  pending_msg_->fromBuffer(pending_msg_->getReceivingBuffer(false), false);
  GOOGLE_DCHECK(!pending_msg_->header()->ack());
  pool_->request(this);
  startReceiving();
}

}
