/**
 * @file ControllerConnection.hpp
 * @brief Define the communication component with the controller.
 *
 * @author Shuhao Liu <shuhao@ece.toronto.edu>
 *
 * @date 2015-12-25
 * @ingroup controller
 */

#ifndef CONTROLLER_CONNECTION_H_
#define CONTROLLER_CONNECTION_H_

#include <list>

#include <boost/asio.hpp>

#include "base/owned_memory_chunk.hpp"
#include "util/types.hpp"


namespace siphon {

/**
 * @brief Define the object that handles the TCP connection to the Controller.
 *
 * Connect to the Controller synchronously, and report the local nodeID
 * information to the Controller right after the connection is established. All
 * following communications are in a JSON-based protocol. There is a `\0`
 * between two consecutive JSON messages. As a result, `\0` is used as the
 * delimeter to identify each JSON message in the stream.
 */
class ControllerConnection {
 public:
   /**
    * @brief Constructor.
    *
    * @param output_strand The reference to the strand, which is used to
    *                      serialize the handler of output JSON messages.
    */
  explicit ControllerConnection(boost::asio::io_context::strand& strand);

  /**
   * @brief Destructor.
   */
  ~ControllerConnection() = default;

  /**
   * @brief Connect to the controller with given information and report the
   *        handshake message (local nodeID). Then, start reading and sending
   *        sequences.
   *
   * @param url The URL of the Controller.
   * @param port The port on which the Controller is listening.
   * @return The nodeID of the current node.
   */
  virtual nodeID_t connectAndStartAtNodeID(const std::string& url,
                                           uint16_t port);

 protected:
  /**
   * @brief Try to connect to the Controller synchronously.
   *
   * @param url The URL of the Controller.
   * @param port The port on which the Controller is listening.
   * @param ec The error code instance, which is required by the connect
   *           operation in `boost::asio` library.
   *
   * @return true If connection is established successfully.
   * @return false If connection fails to be established.
   */
  bool connect(const std::string& url,
               uint16_t port,
               boost::system::error_code& ec);

  /**
   * @brief Start one round of receiving loop, until at least one JSON message
   *        is ready in the `input_buf_`.
   */
  void startRecv();

  /**
   * @brief Send all data currently available in the `output_buffer_` to the
   *        Controller.
   */
  void startSend();

  /**
   * @brief The callback function to be called after all data in the
   *        `output_buffer_` has been sent to the Controller.
   *
   * @param ec The error_code parameter, as is required by the completion
   *           handler.
   * @param bytes_transferred The number of bytes that has been transferred, as
   *                          is required by the completion handler
   *
   * @remark Because this operation modifies the output queue/buffer, it will be
   *         serialized by wrapping to the `output_strand_`.
   */
  virtual void onSent(const boost::system::error_code& ec,
                      size_t bytes_transferred);

  /**
   * @brief The callback function to be called after at least one JSON message
   *        is ready in the `input_buf_`. Extract and process all available JSON
   *        messages.
   *
   * @param ec The error_code parameter, as is required by the completion
   *           handler.
   * @param bytes_transferred The number of bytes that has been transferred, as
   *                          is required by the completion handler
   */
  virtual void onRecv(const boost::system::error_code& ec,
                      size_t bytes_transferred);

  /**
   * @brief A common function used to handle errors.
   *
   * @param ec The code of the error.
   */
  void onError(const boost::system::error_code& ec);

  /**
   * @brief Process a received control message.
   *
   * @param msg The parsed JSON (rapidjson) document.
   *
   * @remark To be overriden by the derived classes.
   */
  virtual void onControlMsgReceived(const rapidjson::Document& msg) = 0;

  /**
   * @brief Stringify a control message contained in a json value, and send to
   *        the Controller.
   * @param msg The controller message which contains all required information
   *            in a json value.
   *
   * @remark Because this operation modifies the output queue/buffer, it will be
   *         serialized by wrapping to the `output_strand_`.
   */
  void sendControlMsg(const rapidjson::Value& msg);

 protected:
  /**
   * @brief Reference to the global `boost::asio::io_context` object.
   */
  boost::asio::io_context& ios_;

  /**
   * @brief Reference to the control message handling strand.
   */
  boost::asio::io_context::strand& output_strand_;

  /**
   * @brief The underlying TCP socket.
   */
  boost::asio::ip::tcp::socket socket_;

  /**
   * @brief The output buffer sequence which will be read directly by the
   *        socket.
   */
  std::list<boost::asio::const_buffer> output_buffer_;
  std::list<OwnedMemoryChunk> output_buffer_chunks_;

  /**
   * @brief The output queue used to buffer the pending output messages.
   */
  std::list<boost::asio::const_buffer> output_queue_;
  std::list<OwnedMemoryChunk> output_queue_chunks_;

  /**
   * @brief A flag used to determine whether we should explicitly call
   *        `startSend()` whenever there is a message to send out.
   */
  bool should_call_send_;

  /**
   * @brief The stream buffer used to read incoming messages sent by the
   *        controller.
   */
  boost::asio::streambuf input_buf_;
};

}

#endif
