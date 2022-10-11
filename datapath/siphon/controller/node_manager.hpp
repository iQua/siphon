/**
 * @file InfoManager.hpp
 * @brief Define the managers that are used to maintain status information, by
 *        the ControllerProxy
 *
 * @author Shuhao Liu <shuhao@ece.toronto.edu>
 *
 * @date 2015-12-28
 * @ingroup controller
 */

#ifndef NODE_MANAGER_HPP_
#define NODE_MANAGER_HPP_

#include <string>
#include <unordered_set>
#include <unordered_map>

#include "apps/app_manager.hpp"
#include "controller/notification_types.hpp"
#include "controller/notification_observers.hpp"
#include "util/types.hpp"


namespace siphon {

/**
 * @brief The manager entity which maintains the nodeID information and their
 *        corresponding URLs.
 *
 * @remark Since almost all nodeID information will be available at start up
 *         time, and everything will be handled by ControllerProxy whose
 *         `handleNotification()` method is dispatched through a strand, we have
 *         to worry nothing about the concurrency issue.
 */
class NodeManager {
 public:
  /**
   * @brief The type of the internal map used to maintain the URL information.
   */
  typedef std::unordered_map<nodeID_t, std::string> MapType;

  /**
   * @brief The type of the internal data structure used to maintain the online
   *        status of the nodes.
   */
  typedef std::unordered_set<nodeID_t> SetType;

  /**
   * @brief A short hand for the tuple type used to represent the status of
   *        a node.
   */
  typedef std::pair<bool, std::string> StatusType;

 public:
  /**
   * @brief Constructor.
   *
   * @param local_nodeID NodeID of the current node.
   */
  NodeManager(AppManager* apps, nodeID_t local_nodeID);

  /**
   * @brief Accessor of the online node set.
   *
   * @return The online node set.
   */
  const SetType& getOnlineNodeSet() const;

  /**
   * @brief Add the URL information of a new node.
   *
   * @param nodeID NodeID of the new node.
   * @param url The URL of the new node.
   *
   * @return true If it is indeed a new node.
   *         false If it is an existing node and its URL information has been
   *               updated with the given one.
   */
  bool addNodeInfo(nodeID_t nodeID, const std::string& url);

  /**
   * @brief Get the current status of the node.
   *
   * @param nodeID The nodeID of the node that draws interest.
   *
   * @return A tuple, whose first element indicating the online status, and the
   *         second element contains its URL.
   */
  StatusType getNodeStatus(nodeID_t nodeID);

  /**
   * @brief Get a node online.
   *
   * @param nodeID The nodeID of the new online node.
   */
  void newOnlineNode(nodeID_t nodeID);

  /**
   * @brief Get a node online.
   *
   * @param msg The JSON message contains the new online nodes.
   */
  const MapType newOnlineNodes(const rapidjson::Value& msg);

  /**
   * @brief Get a node offline.
   *
   * @param nodeID The nodeID of the node that just turned offline.
   */
  void nodeOffline(nodeID_t nodeID);

  /**
   * @brief Get a node offline.
   *
   * @param msg The JSON message contains the new offline nodes.
   */
  nodeID_t nodeOffline(const rapidjson::Value& msg);

 protected:
  /**
   * @brief Current nodeID.
   */
  const nodeID_t local_nodeID_;

  /**
   * @brief The map used to maintain URL information of the nodes.
   */
  MapType node_url_map_;

  /**
   * @brief The set used to maintain the online status of the nodes.
   */
  SetType online_node_set_;

  AppManager* apps_;
  MapType dst_node_to_sessionID_;
};


}

#endif
