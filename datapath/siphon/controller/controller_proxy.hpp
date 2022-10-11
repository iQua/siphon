/**
 * @file ControllerProxy.hpp
 * @brief Define the entity used to communicate with the Controller.
 *
 * @author Shuhao Liu <shuhao@ece.toronto.edu>
 *
 * @date 2015-12-22
 * @ingroup controller
 */

#ifndef CONTROLLER_PROXY_H_
#define CONTROLLER_PROXY_H_

#include "controller/controller_connection.hpp"
#include "controller/crossbar.hpp"
#include "controller/node_manager.hpp"
#include "controller/notification_observers.hpp"
#include "networking/connection_interfaces.hpp"


namespace siphon {

/**
 * @brief Proxy of the remote controller. It is treated as the controller
 *        locally, and communicates with the central controller using
 *        a JSON-based protocol.
 */
class ControllerProxy :
    public ISerializedObserver<ControlMsgNotificationType>,
    protected ControllerConnection {
 public:
  /**
   * @brief Constructor.
   *
   * @param ios Reference to the global `boost::asio::io_context` object.
   * @param controller_url URL of the central controller.
   * @param controller_port Port number of the central controller.
   */
  ControllerProxy(boost::asio::io_context& ios,
                  const std::string& controller_url,
                  uint16_t controller_port);

  nodeID_t local_nodeID() const {
    return local_nodeID_;
  }

  /**
   * @brief Setup the member pointers and connect to the controller.
   *
   * @param node_manager Pointer to the NodeManager object.
   * @param session_manager Pointer to the SessionManager object.
   * @param crossbar Pointer to the Crossbar object.
   */
  void setup(NodeManager* node_manager,
             Crossbar* crossbar,
             IConnectionManager* connections);

  /**
   * @brief Handle the received ControlMsgNotificationType notification.
   *
   * @param notification_msg_ptr Shared pointer to the
   *        ControlMsgNotificationType message.
   * @remark Since ControllerProxy is a serialized observer, everything has been
   *         post to the strand.
   */
  void handleNotification(NotificationMsgPtrType notification_msg_ptr) override;

 protected:
  /**
   * @brief Handle the control message received from the centralized Controller.
   *
   * @param msg The parsed control message.
   */
  void onControlMsgReceived(const rapidjson::Document& msg) override;

  /**
   * @brief Dispatch the control message to an appropriate component, based on
   *        the message type.
   *
   * @param type The type code of the message.
   * @param msg The parsed JSON object.
   */
  void dispatchControlMsgToComponent(ControlMsgType type,
                                     const rapidjson::Value& msg);

 protected:
  /**
   * @brief A flag variable indicating whether the ControllerProxy has been
   *        configured and connected to the centralized Controller.
   */
  bool configured_;

  /**
   * @brief Storing the nodeID of the local node.
   */
  nodeID_t local_nodeID_;

  /**
   * @brief The member NodeManager object.
   */
  NodeManager* node_manager_;

  /**
   * @brief Pointer to the Crossbar object.
   */
  Crossbar* crossbar_;

  IConnectionManager* connections_;
};


}


#endif
