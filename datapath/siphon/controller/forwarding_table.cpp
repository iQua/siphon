/**
 * @file ForwardingTable.cpp
 * @brief Define the forwarding table used in Siphon.
 *
 * @author Shuhao Liu <shuhao@ece.toronto.edu>
 * @date 2015-12-03
 *
 * @ingroup routing
 */

#include "controller/forwarding_table.hpp"

#include <mutex>

#include "boost/bind.hpp"

#include "base/message.hpp"
#include "util/message_util.hpp"
#include "util/logger.hpp"


namespace siphon {

ForwardingTableEntryFactory::ForwardingTableEntryFactory(
    boost::asio::io_context& ios,
    ForwardingTable* forwarding_table) :
    ios_(ios), forwarding_table_(forwarding_table) {
  srand((unsigned) time(NULL));
}

ForwardingTableEntry::ptr ForwardingTableEntryFactory::generateEntry(
    const std::string& sessionID,
    const rapidjson::Value& entry_v,
    uint32_t timeout) const {
  // Check the Entry type
  uint8_t entry_type = 0; // simple (0), splitter (1), generic (2)
  assert(entry_v.IsArray());
  if (!entry_v.Size()) { // Empty array
    throw std::invalid_argument("ForwardingTableEntry candidate #dest is 0.");
    return 0;
  }
  if (!entry_v[0].IsNumber()) { // Not simple
    entry_type = 1;
    if (!entry_v[0].IsObject()) { // Not splitter
      entry_type = 2;
    }
  }

  // Construct the Entry based on the message
  ForwardingTableEntry::ptr new_entry;
  switch (entry_type) {
    case 0: {
      // simple
      new_entry = ForwardingTableEntry::ptr(
          new SimpleForwardingTableEntry(
              entry_v, ios_, sessionID, forwarding_table_, timeout));
      break;
    }
    case 1: {
      // splitter
      new_entry = ForwardingTableEntry::ptr(
          new SplitterForwardingTableEntry(
              entry_v, ios_, sessionID, forwarding_table_, timeout));
      break;
    }
    case 2: {
      // generic
      new_entry = ForwardingTableEntry::ptr(
          new GenericForwardingTableEntry(
              entry_v, ios_, sessionID, forwarding_table_, timeout));
      break;
    }
  }

  return new_entry;
}

ForwardingTableEntry::ptr ForwardingTableEntryFactory::generateEntry(
    const std::string& sessionID,
    const ForwardingTableEntry::result_t& result) const {
  return ForwardingTableEntry::ptr(
      new SimpleForwardingTableEntry(result, ios_, sessionID,
                                     forwarding_table_));
}

ForwardingTable::ForwardingTable(boost::asio::io_context& ios) :
    entry_factory_(ios, this) {
  table_.reserve(kForwardingTableMaxSize);
}

std::pair<std::string, ForwardingTableEntry::ptr> ForwardingTable::newEntry(
    const rapidjson::Value& v) {
  // Assert the message
  assert(v.IsObject());
  if (!v.HasMember("SessionID") || !v.HasMember("Entry")) {
    throw std::invalid_argument("Factory must take a message having SessionID \
                                and an EntryType!");
    return {};
  }
  // Get the sessionID_
  assert(v["SessionID"].IsString());
  std::string sessionID = v["SessionID"].GetString();

  // Get the expire time
  uint32_t timeout = 0;
  if (v.HasMember("Timeout") && v["Timeout"].IsUint()) {
    timeout = v["Timeout"].GetUint();
  }

  // Generate entry and insert to map
  ForwardingTableEntry::ptr new_entry
      = entry_factory_.generateEntry(sessionID, v["Entry"], timeout);

  return {sessionID, new_entry};
}

void ForwardingTable::insertEntry(
    const std::string& sessionID, ForwardingTableEntry::ptr new_entry) {
  if (!new_entry) return;
  {
    std::shared_lock<std::shared_mutex> shared_guard(table_mutex_);
    MapType::iterator i = table_.find(sessionID);
    if (i != table_.end()) {
      // Entry found... it is an update, we just swap the pointer
      boost::atomic_exchange(&(i->second), new_entry);
      return;
    }
    // Else we have to insert it to the map, which modifies the container
  }
  {
    // To modify the container, we need the writer lock
    std::unique_lock<std::shared_mutex> guard(table_mutex_);
    table_[sessionID] = new_entry;
  }

  return;
}

ForwardingTableEntry::result_t ForwardingTable::getNextHop(
    const std::string& sessionID) {
  if (util::IsSpecialSession(sessionID)) {
    // Special sessions, return 0
    LOG_ERROR << "Session#" << sessionID
        << " is a special Session! No nexthop...";
    return {};
  }

  // Find the entry in the table_
  ForwardingTableEntry::ptr entry = 0;
  {
    std::shared_lock<std::shared_mutex> read_guard(table_mutex_);
    MapType::iterator i = table_.find(sessionID);
    if (i == table_.end()) {
      // Not found
      return {};
    }
    else {
      // Found
      entry = boost::atomic_load(&(i->second));
    }
  }
  // We will no longer access the table container, so release the lock
  if (entry) return entry->getNextHop();
  else return {};
}

ForwardingTableEntry::ptr ForwardingTable::getEntry(
    const std::string& sessionID) {
  // Find the entry in the table_
  ForwardingTableEntry::ptr entry = 0;
  {
    std::shared_lock<std::shared_mutex> read_guard(table_mutex_);
    MapType::iterator i = table_.find(sessionID);
    if (i == table_.end()) {
      // Not found
      return nullptr;
    }
    else {
      // Found
      return boost::atomic_load(&i->second);
    }
  }
}

void ForwardingTable::deleteOneEntry(const std::string& sessionID) {
  std::unique_lock<std::shared_mutex> guard(table_mutex_);
  table_.erase(sessionID);
  LOG_INFO << "ForwardingTableEntry for session#" << sessionID << " expired.";
}

void ForwardingTable::onEntryExpired(const boost::system::error_code& ec,
                                     const std::string& sessionID) {
  // Canceled
  if (ec == boost::asio::error::operation_aborted) return;
  // Timed out
  if (ec) throw ec;
  deleteOneEntry(sessionID);
}


}
