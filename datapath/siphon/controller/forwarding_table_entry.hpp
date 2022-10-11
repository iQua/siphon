/**
 * @file ForwardingTableEntry.hpp
 * @brief Define the forwarding table entry types used in Siphon.
 * @author Shuhao Liu <shuhao@ece.toronto.edu>
 *
 * @ingroup routing
 * @date 2015-11-30
 */

#ifndef FORWARDING_TABLE_ENTRY_H_
#define FORWARDING_TABLE_ENTRY_H_

#include <map>
#include <string>
#include <unordered_set>

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include "rapidjson/document.h"

#include "util/types.hpp"


namespace siphon {

// Forward Declaration
class ForwardingTable;

/**
 * @brief Define the base class for a ForwardingTableEntry.
 */
class ForwardingTableEntry {
  friend ForwardingTable;
 public:
  /**
   * @brief A short hand for the shared pointer type.
   */
  typedef boost::shared_ptr<ForwardingTableEntry> ptr;

  /**
   * @brief A short hand for the data type of the forwarding decisions.
   */
  typedef std::unordered_set<nodeID_t> result_t;

 public:
  /**
   * @brief Constructor.
   *
   * @param ios The reference to the global `boost::asio::io_context` object.
   * @param sessionID The sessionID of the corresponding flow.
   * @param forwarding_table The pointer to the parent ForwardingTable.
   * @param expire_time The timeout for the ForwardingTableEntry.
   */
  ForwardingTableEntry(boost::asio::io_context& ios,
                       const std::string& sessionID,
                       ForwardingTable* forwarding_table,
                       uint32_t expire_time = 0);

  /**
   * @brief Destructor. Cancel the pending jobs in timer.
   */
  ~ForwardingTableEntry();

  /**
   * @brief The public interface for others to call, return the routing
   *        decision.
   *
   * @return The routing decision of the ForwardingTableEntry.
   */
  result_t getNextHop();

 protected:
  /**
   * @brief The interface to compute the next hop information. To be overriden
   *        by the derived classes.
   *
   * @return The routing decision.
   */
  virtual result_t nextHop() const = 0;

 protected:
  /**
   * @brief The deadline timer object.
   */
  boost::asio::deadline_timer timer_;

  /**
   * @brief To store the sessionID information.
   */
  std::string sessionID_;

  /**
   * @brief Pointer to the parent ForwardingTable.
   */
  ForwardingTable* forwarding_table_;

  /**
   * @brief Store the time to expire of this entry in seconds.
   */
  const uint32_t expire_time_;

  /**
   * @brief Store the time duration to expire.
   */
  const boost::posix_time::time_duration dur_to_expire_;
};

/**
 * @brief The most common ForwardingTableEntry which simply forwards a single
 *        packet to one or more destinations. It can be used for both unicast
 *        and multicast.
 */
class SimpleForwardingTableEntry : public ForwardingTableEntry {
 public:
  /**
   * @brief Constructor.
   *
   * SimpleForwardingEntry is constructed with the JSON message of the format:
   * \code{.js}
   * [dst0, dst1, dst2...]
   * \endcode
   *
   * @param v The parsed `rapidjson::Value`, containing the necessary
   *        information to construct the entry.
   * @param ios The reference to the global `boost::asio::io_context` object.
   * @param sessionID The sessionID of the corresponding flow.
   * @param forwarding_table The pointer to the parent ForwardingTable.
   * @param expire_time The timeout for the ForwardingTableEntry.
   */
  SimpleForwardingTableEntry(const rapidjson::Value& v,
                             boost::asio::io_context& ios,
                             const std::string& sessionID,
                             ForwardingTable* forwarding_table,
                             uint32_t expire_time = 0);

  /**
   * @brief An alternative constructor that directly takes a result.
   *
   * @param result The result of `result_t` type.
   * @param ios The reference to the global `boost::asio::io_context` object.
   * @param sessionID The sessionID of the corresponding flow.
   * @param forwarding_table The pointer to the parent ForwardingTable.
   * @param expire_time The timeout for the ForwardingTableEntry.
   */
  SimpleForwardingTableEntry(const result_t& result,
                             boost::asio::io_context& ios,
                             const std::string& sessionID,
                             ForwardingTable* forwarding_table,
                             uint32_t expire_time = 0);

 protected:
  /**
   * @brief The interface to compute the next hop information.
   *
   * @return The routing decision.
   */
  virtual result_t nextHop() const;

 protected:
  /**
   * @brief The constant forwarding decision.
   */
  result_t nexthops_;
};

/**
 * @brief A ForwardingTableEntry with the capability of probablisitic routing.
 *        Flows using this entry will be splitted to multiple path.
 */
class SplitterForwardingTableEntry : public ForwardingTableEntry {
 protected:
  /**
   * @brief A short hand for the type of map data structure, which is used to
   *        make probablistic dicisions.
   */
  typedef std::multimap<double, nodeID_t, std::greater<double>> MapType;

 public:
  /**
   * @brief Constructor.
   *
   * SplitterForwardingEntry is constructed with the JSON message of the format:
   * \code{.js}
   * [{"NextHop": dst0, "Weight": __ }, {"NextHop": dst1, "Weight": __ }...]
   * \endcode
   *
   * @param v The parsed `rapidjson::Value`, containing the necessary
   *        information to construct the entry.
   * @param ios The reference to the global `boost::asio::io_context` object.
   * @param sessionID The sessionID of the corresponding flow.
   * @param forwarding_table The pointer to the parent ForwardingTable.
   * @param expire_time The timeout for the ForwardingTableEntry.
   */
  SplitterForwardingTableEntry(const rapidjson::Value& v,
                               boost::asio::io_context& ios,
                               const std::string& sessionID,
                               ForwardingTable* forwarding_table,
                               uint32_t expire_time = 0);

 protected:
  /**
   * @brief The interface to compute the next hop information.
   *
   * @return The routing decision.
   */
  virtual result_t nextHop() const;

 protected:
  /**
   * @brief The map used to help making probablistic routing decisions.
   */
  MapType weighted_nexthop_;

  /**
   * @brief Keep a record of total weight in the message.
   */
  double total_weight_;
};

/**
 * @brief The generic ForwardingTableEntry class which supports both packet
 *        multicast and multipath routing.
 */
class GenericForwardingTableEntry : public ForwardingTableEntry {
 protected:
  /**
   * @brief A short hand for the type of map data structure, which is used to
   *        make probablistic dicisions.
   */
  typedef std::multimap<double, nodeID_t, std::greater<double>> MapType;

 public:
  /**
   * @brief Constructor.
   *
   * GenericForwardingEntry is constructed with the JSON message of the format:
   * \code{.js}
   * [
   *   [{"NextHop": dst0-0, "Weight": _ }, {"NextHop": dst0-1...}...], // Copy0
   *   [{"NextHop": dst1-0, "Weight": _ }, {"NextHop": dst1-1...}...], // Copy1
   *   ...
   * ]
   * \endcode
   *
   * @param v The parsed `rapidjson::Value`, containing the necessary
   *        information to construct the entry.
   * @param ios The reference to the global `boost::asio::io_context` object.
   * @param sessionID The sessionID of the corresponding flow.
   * @param forwarding_table The pointer to the parent ForwardingTable.
   * @param expire_time The timeout for the ForwardingTableEntry.
   */
  GenericForwardingTableEntry(const rapidjson::Value& v,
                              boost::asio::io_context& ios,
                              const std::string& sessionID,
                              ForwardingTable* forwarding_table,
                              uint32_t expire_time = 0);

 protected:
  /**
   * @brief The interface to compute the next hop information.
   *
   * @return The routing decision.
   */
  virtual result_t nextHop() const;

 protected:
  /**
   * @brief A set of maps helping to make probablistic routing decisions.
   */
  std::vector<std::pair<double, MapType>> weighted_nexthops_;
};


}

#endif
