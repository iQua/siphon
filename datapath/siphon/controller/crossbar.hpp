/**
 * @file Crossbar.hpp
 * @brief Crossbar is the key component in Siphon to forward packet to the
 *        its proper destination connection queue.
 *
 * @author Shuhao Liu <shuhao@ece.toronto.edu>
 * @date 2015-12-04
 *
 * @ingroup routing
 */

#ifndef CROSSBAR_H_
#define CROSSBAR_H_

#include <boost/unordered_map.hpp>

#include "base/minion_pool.hpp"
#include "base/mpsc_queue.hpp"
#include "controller/forwarding_table.hpp"
#include "controller/notification_observers.hpp"
#include "controller/notification_types.hpp"
#include "networking/connection_interfaces.hpp"


namespace siphon {

/**
 * @brief To correctly process the packets, all operations must be SYNCHRONIZED.
 */
class PendingPacketArchive {
 public:
  /**
   * @brief A short hand for the type of queue which is used to archive pending
   *        packets.
   */
  typedef MPSCQueue<Minion*> QueueType;

  /**
   * @brief The type of the underlying map to reference the pending packets.
   */
  typedef boost::unordered_map<std::string, boost::shared_ptr<QueueType>>
      MapType;

 public:
  /**
   * @brief Constructor. Reserve the space to improve efficiency.
   */
  PendingPacketArchive();

  /**
   * @brief Archive the given message.
   *
   * @param sessionID The sessionID that the new message belongs to.
   * @param packet The new message to be archived.
   *
   * @return true If this is a new entry, which means this is the first received
   *              packet of the given session.
   *         false If the entry exists.
   */
  bool archiveMinion(const std::string& sessionID, Minion* minion);

  /**
   * @brief Get the pending packet queue for a given session.
   *
   * @param sessionID The sessionID to be searched.
   * @return The pointer to the queue. If not found, nullptr is returned.
   */
  MapType::mapped_type getPendingPacketQueue(const std::string& sessionID);

  /**
   * @brief Remove the pending packet queue for a given session from the map.
   *
   * @param sessionID The sessionID to be removed.
   * @return The pointer to the queue being removed. If not found, nullptr is
   *         returned.
   *
   * @remark The returned queue may not be empty even though having been
   *         removed.
   */
  MapType::mapped_type removeSessionFromArchive(const std::string& sessionID);

  /**
   * @brief Reset the pending packet queue for a given session, without deleting
   *        the entry from the map.
   *
   * @param sessionID The sessionID to be reset.
   * @return The pointer to the queue being reset. If not found, nullptr is
   *         returned.
   *
   * @remark The returned queue may not be empty even though having been reset.
   */
  MapType::mapped_type resetSessionFromArchive(const std::string& sessionID);

 protected:
  /**
   * @brief The map to reference the pending packet queues.
   */
  MapType pending_minion_map_;

  /**
   * @brief The spinlock to protect the map container.
   */
  std::shared_mutex map_lock_;
};

/**
 * @brief The Crossbar class, which is used to dispatch the input packets to the
 *        output port. It consumes PacketInMsg, and emit PacketOutMsgs
 *        correspondingly.
 */
class Crossbar : public IMinionStop,
    public INotification<ControlMsgNotificationType> {
 public:
  /**
   * @brief Constructor.
   *
   * @param ios Reference to the global `boost::asio::io_context` object.
   */
  Crossbar(boost::asio::io_context& ios,
      MinionPool* pool,
      IMinionStop* local_app,
      IConnectionManager* conn_manager);

  /**
   * @brief Destructor.
   */
  ~Crossbar() override = default;

  /**
   * @brief Install a new ForwardingTableEntry to the ForwardingTable.
   *
   * The given JSON value should be in the following format:
   * \code{.js}
   * { "SessionID": __, "Entry": [ __ ] (, "Timeout": __) }
   * \endcode
   *
   * @param v The JSON message used to construct the ForwardingTableEntry.
   */
  void installForwardingTableEntry(const rapidjson::Value& v);

  virtual IMinionStop* process(Minion* minion) override;

  void setLocalNodeID(nodeID_t nodeID) {
    local_nodeID_ = nodeID;
  }

  void setLocalApp(IMinionStop* local_app) {
    local_app_ = local_app;
  }

  void setConnectionManager(IConnectionManager* conn_manager) {
    conn_manager_ = conn_manager;
  }

 protected:
  /**
   * @brief Process a single packet that is already archived.
   *
   * @param packet The pointer to the PacketInMsg.
   * @param entry The ForwardingTableEntry to forward the packet.
   */
  void handleArchivedPacket(MPSCQueue<Minion*>::Node* packet_node,
                            ForwardingTableEntry::ptr entry);

  /**
   * @brief Send the controller a message, querying for the routing decision for
   *        a particular session.
   *
   * @param sessionID The sessionID of the session to query about.
   */
  void queryForwardingTableEntry(const std::string& sessionID);

 private:
  /**
   * @brief The ForwardingTable member.
   */
  ForwardingTable forwarding_table_;

  /**
   * @brief The PendingPacketArchive to store the packets that cannot be sent
   *        directly.
   */
  PendingPacketArchive packet_archive_;

  MinionPool* pool_;

  IMinionStop* local_app_;

  IConnectionManager* conn_manager_;

  nodeID_t local_nodeID_;
};

}

#endif
