/**
 * @file ControllerConnection.cpp
 * @brief Define the communication component with the controller.
 *
 * @author Shuhao Liu <shuhao@ece.toronto.edu>
 *
 * @date 2015-12-25
 * @ingroup controller
 */

#include "controller/controller_connection.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <google/protobuf/message.h>

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "util/config.hpp"
#include "util/logger.hpp"
#include "util/types.hpp"


namespace siphon {

ControllerConnection::ControllerConnection(
    boost::asio::io_context::strand& strand) :
    ios_(strand.get_io_service()),
    output_strand_(strand),
    socket_(strand.get_io_service()),
    should_call_send_(true) {
}

nodeID_t ControllerConnection::connectAndStartAtNodeID(const std::string& url,
                                                       uint16_t port) {
  // Try to connect
  bool connected = false;
  boost::system::error_code ec;
  for (int i = 0; i < 5; ++i) {
    if (connect(url, port, ec)) {
      connected = true;
      break;
    }
    else {
      // Cannot connect... Wait to retry
      LOG_INFO << "Connect to Controller Failed: Try #" << i;
      boost::asio::deadline_timer t(ios_);
      t.expires_from_now(boost::posix_time::seconds(1 << i));
      t.wait();
    }
  }
  if (!connected) {
    onError(ec);
    return 0;
  }

  // Set options
  socket_.set_option(boost::asio::ip::tcp::no_delay(true), ec);
  if (ec) onError(ec);
  boost::asio::socket_base::receive_buffer_size recvSizeOption;
  socket_.get_option(recvSizeOption, ec);
  if (ec) {
    onError(ec);
    return 0;
  }

  // Report local hostname
  std::string local_hostname = "127.0.1.1";
  if (!util::Config::get()->kLocalDebugNoReceivingSocket) {
    local_hostname = boost::asio::ip::host_name();
  }

  LOG_INFO << "Local hostname resolved: " << local_hostname;
  nodeID_t hostname_length = local_hostname.size();
  boost::asio::write(socket_, boost::asio::buffer(
          (uint8_t*) &hostname_length, sizeof(nodeID_t)), ec);
  boost::asio::write(socket_, boost::asio::buffer(
          local_hostname.c_str(), local_hostname.size()), ec);

  // Get local NodeID
  nodeID_t nodeID;
  uint8_t* buf = (uint8_t*) &nodeID;
  boost::asio::read(socket_,
                    boost::asio::buffer(buf, sizeof(nodeID_t)), ec);
  LOG_INFO << "Local node ID: " << nodeID;

  if (ec) {
    onError(ec);
    return 0;
  }

  LOG_INFO << "Connection to Controller established.";

  // Start sending and receiving
  startRecv();
  output_strand_.post(boost::bind(&ControllerConnection::startSend, this));

  return nodeID;
}

bool ControllerConnection::connect(const std::string& url,
                                   uint16_t port,
                                   boost::system::error_code& ec) {
  bool succeeded = true;
  using boost::asio::ip::tcp;
  try {
    tcp::resolver resolver(ios_);
    tcp::resolver::query query(url, boost::lexical_cast<std::string>(port));
    tcp::resolver::iterator itr = resolver.resolve(query);
    socket_.connect(*itr);
  }
  catch (boost::system::error_code& error) {
    succeeded = false;
    ec = error;
  }
  return succeeded;
}

void ControllerConnection::startRecv() {
  boost::asio::async_read_until(socket_, input_buf_, '\0',
      boost::bind(&ControllerConnection::onRecv, this,
                  boost::asio::placeholders::error,
                  boost::asio::placeholders::bytes_transferred));
}

void ControllerConnection::onRecv(const boost::system::error_code& ec,
                                  size_t bytes_transferred) {
  if (ec) {
    onError(ec);
    return;
  }

  std::istream istr(&input_buf_);
  std::ostream ostr(&input_buf_);
  std::string msg;
  while (1) {
    // Extract a json string
    std::getline(istr, msg, '\0');
    if (istr.eof()) {
      ostr << msg;
      break;
    }
    else {
      // Parse it
      using namespace rapidjson;
      Document doc;
      doc.Parse(msg.c_str());
      onControlMsgReceived(doc);
    }
  }

  // Start next round of receiving
  startRecv();
}

void ControllerConnection::startSend() {
  if (!should_call_send_) {
    return;
  }
  should_call_send_ = false;

  if (!output_queue_.empty()) {
    // If there is pending messages, try sending them.
    output_buffer_.splice(output_buffer_.begin(), output_queue_);
    output_buffer_chunks_.splice(output_buffer_chunks_.begin(),
        output_queue_chunks_);
  }
  assert(output_buffer_.size() == output_buffer_chunks_.size());
  if (output_buffer_.empty()) {
    should_call_send_ = true;
    return;
  }
  boost::asio::async_write(socket_, output_buffer_, output_strand_.wrap(
          boost::bind(&ControllerConnection::onSent, this,
                      boost::asio::placeholders::error,
                      boost::asio::placeholders::bytes_transferred)));
}

void ControllerConnection::onSent(const boost::system::error_code& ec,
                                  size_t bytes_transferred) {
  should_call_send_ = true;
  assert(bytes_transferred == boost::asio::buffer_size(output_buffer_));
  // Clear the previous messages from output_buf_.
  output_buffer_.clear();
  output_buffer_chunks_.clear();

  startSend();
}

void ControllerConnection::onError(const boost::system::error_code& ec) {
  LOG_FATAL << "FATAL: ControllerConnection error encountered " << ec.message();
  throw ec;
}

void ControllerConnection::sendControlMsg(const rapidjson::Value& msg) {
  // Serialize the message
  using namespace rapidjson;
  StringBuffer str_buf;
  Writer<StringBuffer> writer(str_buf);
  msg.Accept(writer);

  // Convert to string and enqueue, try send
  size_t msg_size = str_buf.GetSize() + 1;
      GOOGLE_DCHECK(msg_size <= util::Config::get()->kMaxControlMessageSize)
          << "Control message is too large: " << str_buf.GetString();
  const char* msg_ptr = str_buf.GetString();

  // Create a message at the end of output_queue_.
  OwnedMemoryChunk& new_msg = output_queue_chunks_.emplace_back(msg_size);
  boost::asio::buffer_copy(new_msg.getMutableBuffer(),
      boost::asio::buffer(msg_ptr, msg_size));
  output_queue_.emplace_back(new_msg.getConstBuffer());
  if (should_call_send_) startSend();
}


}
