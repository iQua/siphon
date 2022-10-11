/**
 * @file ForwardingTable.hpp
 * @brief Define the forwarding table used in Siphon.
 *
 * @author Shuhao Liu <shuhao@ece.toronto.edu>
 * @date 2015-11-30
 *
 * @ingroup routing
 */

#ifndef FORWARDING_TABLE_H_
#define FORWARDING_TABLE_H_

#include <shared_mutex>

#include <boost/atomic.hpp>
#include <boost/unordered_map.hpp>
#include <boost/function.hpp>

#include "forwarding_table_entry.hpp"


namespace siphon {

/**
 * @brief Forward Declaration.
 */
class ForwardingTable;

/**
 * @brief A factory class to generate different types of ForwardingTableEntries.
 */
class ForwardingTableEntryFactory {
 public:
  /**
   * @brief Constructor.
   *
   * @param ios The reference to the global `boost::asio::io_context` object.
   * @param forwarding_table Pointer to the parent ForwardingTable.
   */
  ForwardingTableEntryFactory(boost::asio::io_context& ios,
                              ForwardingTable* forwarding_table);

  /**
   * @brief Generate a ForwardingTableEntry with the given JSON value.
   *
   * @param sessionID The corresponding sessionID of the generated entry.
   * @param v The JSON value used to generate the entry.
   * @param timeout The expire time of the entry
   *
   * @return The pointer to the newly generated entry.
   */
  ForwardingTableEntry::ptr generateEntry(
      const std::string& sessionID,
      const rapidjson::Value& v,
      uint32_t timeout = 0) const;

  /**
   * @brief Generate a SimpleForwardingTableEntry with the given result.
   *
   * @param sessionID The corresponding sessionID of the generated entry.
   * @param result The static forwarding decision of the entry.
   *
   * @return The pointer to the newly generated entry.
   */
  ForwardingTableEntry::ptr generateEntry(const std::string& sessionID,
      const ForwardingTableEntry::result_t& result) const;

 private:
  /**
   * @brief The reference to the global `boost::asio::io_context` object.
   */
  boost::asio::io_context& ios_;

  /**
   * @brief Pointer to the parent ForwardingTable.
   */
  ForwardingTable* forwarding_table_;
};

/**
 * @brief The ForwardingTable class.
 */
class ForwardingTable {
  friend ForwardingTableEntry;
 public:
  /**
   * @brief A short hand for the underlying data type for the hash map.
   */
  typedef boost::unordered_map<std::string, ForwardingTableEntry::ptr> MapType;

  static constexpr int kForwardingTableMaxSize = 256;

 public:
  /**
   * @brief Constructor.
   *
   * @param ios The reference to the global `boost::asio::io_context` object.
   */
  ForwardingTable(boost::asio::io_context& ios);

  /**
   * @brief Destructor. Everything is handled by smart pointer. Nothing to clean
   *        up.
   */
  ~ForwardingTable() = default;

  /**
   * @brief Create a new ForwardingTableEntry, without inserting to the
   *        ForwardingTable.
   *
   * The given JSON value should be in the following format:
   * \code{.js}
   * { "SessionID": __, "Entry": [ __ ] (, "Timeout": __) }
   * \endcode
   *
   * @param v The JSON value to generate the new entry.
   * @return A pair containing sessionID and the created entry.
   */
  std::pair<std::string, ForwardingTableEntry::ptr>
      newEntry(const rapidjson::Value& v);

  /**
   * @brief Create a new ForwardingTableEntry, without inserting to the
   *        ForwardingTable.
   *
   * @tparam T The result type to help the ForwardingTableEntryFactory to
   *         generate a new entry.
   * @param sessionID The corresponding sessionID of the entry to be generated.
   * @param param The result value to help the ForwardingTableEntryFactory to
   *        generate a new entry.
   *
   * @return The new entry.
   */
  template <typename... T>
  ForwardingTableEntry::ptr newEntry(
      const std::string& sessionID, const T&... param) {
    ForwardingTableEntry::ptr new_entry
        = entry_factory_.generateEntry(sessionID, param...);
    return new_entry;
  }

  /**
   * @brief Insert an entry to the forwarding table.
   *
   * @param sessionID The sessionID of the new entry.
   * @param new_entry The pointer to the new entry.
   */
  void insertEntry(const std::string& sessionID, ForwardingTableEntry::ptr new_entry);

  /**
   * @brief The public interface to query the nexthop information.
   *
   * @param sessionID Queried sessionID.
   *
   * @return The result of the forwarding decision.
   */
  ForwardingTableEntry::result_t getNextHop(const std::string& sessionID);

  /**
   * @brief Get the ForwardingTableEntry for a given session.
   *
   * @param sessionID Queried sessionID.
   *
   * @return The pointer to the corresponding ForwardingTableEntry.
   */
  ForwardingTableEntry::ptr getEntry(const std::string& sessionID);

 protected:
  /**
   * @brief Delete an entry from the forwarding table, referenced by the
   *        given sessionID.
   *
   * @param sessionID The sessionID of the entry to be deleted.
   */
  void deleteOneEntry(const std::string& sessionID);

  /**
   * @brief The callback function if the timer expired. If called, it means that
   *        the entry should be invalidated.
   *
   * @param ec The error code if error occurs.
   * @param sessionID The ID of the expired session.
   */
  void onEntryExpired(const boost::system::error_code& ec,
                      const std::string& sessionID);

 protected:
  /**
   * @brief The forwarding table instance, based on a `boost::unordered_map`.
   */
  MapType table_;

  /**
   * @brief The ForwardingTableEntryFactory instance to generate new entries.
   */
  ForwardingTableEntryFactory entry_factory_;

  /**
   * @brief The spin lock to help protect the thread safety of the forwarding
   *        table.
   */
  std::shared_mutex table_mutex_;
};

}


#endif
