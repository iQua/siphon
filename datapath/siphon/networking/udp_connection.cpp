//
// Created by Shuhao Liu on 2018-03-18.
//

#include "networking/udp_connection.hpp"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

#include "coder/udp_coder_factory.hpp"
#include "util/config.hpp"
#include "util/message_util.hpp"
#include <thread>


namespace siphon {

UDPSenderSocketWrapper::UDPSenderSocketWrapper(boost::asio::io_context& ios,
                                               MinionPool* pool,
                                               MinionStopQueueType* queue) :
    ISenderSocketWrapper(pool, queue), socket_(ios) {
}

IMinionStop* UDPSenderSocketWrapper::process(Minion* minion) {
  current_minion_ = minion;
  minion->getData()->mutable_header()->set_timestamp(util::CurrentUnixMicros());
  socket_.async_send(minion->getData()->toBuffer(),
                     boost::bind(&UDPSenderSocketWrapper::onDataSent,
                                 this,
                                 boost::asio::placeholders::error,
                                 boost::asio::placeholders::bytes_transferred));

  // Return null. Send it to pool in the callback.
  return nullptr;
}

void UDPSenderSocketWrapper::onDataSent(const boost::system::error_code& ec,
                                        size_t /*bytes_transferred*/) {
  if (current_minion_->getExtraData()->empty()) {
    // No more message to send. Return the minion back to the pool.
    socket_.get_io_context().post(boost::bind(
        &MinionPool::process, pool_, current_minion_));
    startSending();
    return;
  }
  // Send messages in the extra data list.
  current_minion_->swapData(current_minion_->getExtraData()->front());
  current_minion_->getExtraData()->pop_front();
  process(current_minion_);
}

UDPSender::UDPSender(boost::asio::io_context& ios,
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

bool UDPSender::resolveAndConnect(const std::string& url) {
  using boost::asio::ip::udp;
  udp::socket* socket = socket_wrapper_.socket();
  try {
    udp::resolver resolver(socket->get_io_context());
    udp::resolver::query query(url, boost::lexical_cast<std::string>(
        util::Config::get()->kUDPListeningPort));
    udp::resolver::iterator itr = resolver.resolve(query);
    socket->connect(*itr);
  }
  catch (boost::system::error_code& error) {
    LOG_ERROR << "UDP connect error: " << error.message();
    return false;
  }
  return true;
}

IMinionStop* UDPSender::process(Minion* minion) {
  // Find or create the corresponding encoder.
  const std::string& sessionID = minion->getData()->header()->session_id();
  {
    std::unique_lock<std::mutex> lock(mtx_);
    auto encoder_itr = encoders_.find(sessionID);
    if (encoder_itr == encoders_.end()) {
      encoders_[sessionID] = UDPEncoderFactory::create(
          util::Config::get()->kUDPCoderName);
      encoder_itr = encoders_.find(sessionID);
    }
    GOOGLE_DCHECK(encoder_itr != encoders_.end());

    // Encode first.
    bool has_encoded = encoder_itr->second->encode(minion);
    if (!has_encoded) return pool_;
  }

  // Modify message header to send.
  auto set_header_fields = [this](Message* message) -> void {
    MessageHeader* header = message->mutable_header();
    header->set_payload_size(message->getPayloadSize());
    header->set_ack(false);
    header->set_src(local_nodeID_);
    header->set_dst(peer_nodeID_);
    // timestamp is set before sending.
    // coding parameters are set while encoding.
  };

  // Set the header for message.
  set_header_fields(minion->getData());
  for (auto& msg : *(minion->getExtraData())) {
    set_header_fields(msg.get());
  }

  return &q_;
}

void UDPSender::startSending() {
  socket_wrapper_.startSending();
}

void UDPSender::onAck(Message* ack) {
  const std::string& session = ack->header()->session_id();
  std::unique_lock<std::mutex> lock(mtx_);
  encoders_[session]->setParameters(ack->header()->coding_parameters());
}

UDPReceiver::UDPReceiver(boost::asio::io_context& ios,
    MinionPool* pool,
    IMinionStop* crossbar) :
    IReceiver(pool),
    socket_(ios, boost::asio::ip::udp::endpoint(
        boost::asio::ip::udp::v4(), util::Config::get()->kUDPListeningPort)),
    crossbar_(crossbar),
    pending_msg_(std::make_unique<Message>()),
    serialized_ack_(util::Config::get()->kMaxMessageSize) {
}

IMinionStop* UDPReceiver::process(Minion* minion) {
  minion->swapData(pending_msg_);

  // Find decoder and decode.
  const std::string& session = minion->getData()->header()->session_id();
  bool has_decoded_message;
  uint32_t updated_coding_params;
  {
    std::unique_lock<std::mutex> lock(mtx_);
    auto decoder_iterator = decoders_.find(session);
    if (decoder_iterator == decoders_.end()) {
      decoders_[session] = UDPDecoderFactory::create(
          util::Config::get()->kUDPCoderName);
      decoder_iterator = decoders_.find(session);
    }
    assert(decoder_iterator != decoders_.end());
    has_decoded_message = decoder_iterator->second->decode(minion);
    updated_coding_params = decoder_iterator->second->getEncodedParameters();
  }

  // Generate acknowledgement.
  MessageHeader ack_header = *(minion->getData()->header());
  ack_header.set_ack(true);
  ack_header.set_payload_size(0);
  ack_header.set_coding_parameters(updated_coding_params);
  Message::HeaderSizeType ack_header_size = ack_header.ByteSize();
  uint8_t* pointer = serialized_ack_.data;
  *((Message::MessageSizeType*) pointer) =
      ack_header_size + sizeof(Message::HeaderSizeType);
  pointer += sizeof(Message::MessageSizeType);
  *((Message::HeaderSizeType*) pointer) = ack_header_size;
  pointer += sizeof(Message::HeaderSizeType);
  ack_header.SerializeToArray((void*) pointer, ack_header_size);

  // Send acknowledgement if the other end is not localhost.
  if ("test" != util::Config::get()->kUDPCoderName &&
      peer_endpoint_.address().to_string() == "127.0.0.1") {
    startReceiving();
    return has_decoded_message ? crossbar_ : pool_;
  }

  peer_endpoint_.port(util::Config::get()->kUDPListeningPort);
  socket_.async_send_to(
      serialized_ack_.sliceConstBuffer(
          0, ack_header_size + sizeof(Message::MessageSizeType) +
              sizeof(Message::HeaderSizeType)),
      peer_endpoint_,
      [this](const boost::system::error_code& error, size_t bytes) {
        if (error) {
          LOG_ERROR << "Ack sent error: " << error.message();
        }
        startReceiving();
      });

  return has_decoded_message ? crossbar_ : pool_;
}

void UDPReceiver::startReceiving() {
  pending_msg_->recycle();
  socket_.async_receive_from(pending_msg_->getReceivingBuffer(true),
      peer_endpoint_,
      boost::bind(&UDPReceiver::onDataReceived, this,
                  boost::asio::placeholders::error(),
                  boost::asio::placeholders::bytes_transferred()));
}

void UDPReceiver::setupPeer(nodeID_t nodeID, ISender* sender) {
  std::unique_lock<std::mutex> lock(mtx_);
  senders_[nodeID] = sender;
}

void UDPReceiver::removePeer(nodeID_t nodeID) {
  std::unique_lock<std::mutex> lock(mtx_);
  senders_.erase(nodeID);
}

void UDPReceiver::onDataReceived(const boost::system::error_code& error,
    size_t /*bytes_transferred*/) {
  if (error) {
    LOG_ERROR << "UDP receiving error from [" << peer_endpoint_
              << "]: " << error.message();
  }

  pending_msg_->fromBuffer(pending_msg_->getReceivingBuffer(true), true);
  if (pending_msg_->header()->ack()) {
    // Is an acknowledgement. No need for a minion.
    nodeID_t peer = pending_msg_->header()->dst();
    std::unique_lock<std::mutex> lock(mtx_);
    auto sender_iterator = senders_.find(peer);
    assert(sender_iterator != senders_.end());
    sender_iterator->second->onAck(pending_msg_.get());
    // After processing the ack, start receiving again.
    startReceiving();
  }
  else pool_->request(this);
}

}
