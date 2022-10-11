#ifndef SIPHON_TEST_FAKE_CONTROLLER
#define SIPHON_TEST_FAKE_CONTROLLER

#include <string>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

#include "base/owned_memory_chunk.hpp"
#include "util/config.hpp"
#include "util/logger.hpp"
#include "util/types.hpp"


namespace siphon {
namespace test {

class FakeController {
 public:
  static constexpr uint16_t kFakeControllerPort = 6699;
  static constexpr nodeID_t kNodeID = 2;

  explicit FakeController(boost::asio::io_context& ios) :
      ios_(ios), socket_(ios), acceptor_(ios) {
  }

  void start(const uint16_t port = kFakeControllerPort) {
   using namespace boost::asio::ip;
   // Resolve the local endpoint
   tcp::resolver resolver(ios_);
   tcp::resolver::query query(tcp::v4(),
                              boost::lexical_cast<std::string>(port));
   tcp::endpoint listen_endpoint = *resolver.resolve(query);

   // Start listening on acceptor_
   acceptor_.open(listen_endpoint.protocol());
   acceptor_.set_option(tcp::acceptor::reuse_address(true));
   acceptor_.bind(listen_endpoint);
   acceptor_.listen(2); // Backlog set to 2.

   // Start accepting.
   acceptor_.async_accept(socket_,
       boost::bind(&FakeController::onConnection, this,
                   boost::asio::placeholders::error()));
  }

  void close() {
    if (acceptor_.is_open()) {
      // Ignore errors.
      boost::system::error_code ec;
      acceptor_.cancel(ec);
      acceptor_.close(ec);
    }
  }

  void onConnection(const boost::system::error_code& ec) {
    if (ec) {
      LOG_ERROR << "Connection establishment error.";
      return;
    }

    boost::system::error_code error;
    nodeID_t hostname_length;
    socket_.read_some(boost::asio::mutable_buffer(
        (uint8_t*) &hostname_length, sizeof(nodeID_t)), error);
    OwnedMemoryChunk buffer(hostname_length);
    boost::asio::mutable_buffer hostname_buffer = buffer.getMutableBuffer();
    socket_.read_some(hostname_buffer, error);
    hostname_ = std::string((char*) hostname_buffer.data(),
        hostname_buffer.size());

    hostname_length = kNodeID;
    socket_.write_some(boost::asio::const_buffer(
        (uint8_t*) &hostname_length, sizeof(nodeID_t)), error);

    startReceive();
  }

  void startReceive() {
    boost::asio::async_read_until(socket_, input_buf_, '\0',
        boost::bind(&FakeController::onRecv, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
  }

  void onRecv(const boost::system::error_code& ec, size_t bytes) {
    startReceive();
  }

  std::string hostname() const {
    return hostname_;
  }

  void sendMessage(const std::string& json) {
    boost::system::error_code ec;
    boost::asio::write(socket_,
        boost::asio::buffer(json.c_str(), json.length() + 1), ec);
  }


 private:
  boost::asio::io_context& ios_;
  boost::asio::ip::tcp::socket socket_;
  boost::asio::ip::tcp::acceptor acceptor_;
  boost::asio::streambuf input_buf_;
  std::string hostname_;
};

}
}

#endif
