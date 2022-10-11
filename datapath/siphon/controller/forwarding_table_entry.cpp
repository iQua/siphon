/**
 * @file ForwardingTableEntry.cpp
 * @brief Define the forwarding table entry types used in Siphon.
 * @author Shuhao Liu <shuhao@ece.toronto.edu>
 *
 * @ingroup routing
 * @date 2015-11-30
 */

#include "controller/forwarding_table_entry.hpp"

#include <stdexcept>
#include <cstdlib>

#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>

#include "controller/forwarding_table.hpp"
#include "util/logger.hpp"


namespace siphon {

ForwardingTableEntry::ForwardingTableEntry(boost::asio::io_context& ios,
                                           const std::string& sessionID,
                                           ForwardingTable* forwarding_table,
                                           uint32_t expire_time) :
    timer_(ios), sessionID_(sessionID), forwarding_table_(forwarding_table),
    expire_time_(expire_time),
    dur_to_expire_(boost::posix_time::seconds(expire_time)) {
  if (expire_time == 0) return;
  // Set a timer to expire
  timer_.expires_from_now(dur_to_expire_);
  timer_.async_wait(boost::bind(&ForwardingTable::onEntryExpired,
                                forwarding_table_,
                                boost::asio::placeholders::error,
                                sessionID_));
}

ForwardingTableEntry::~ForwardingTableEntry() {
  timer_.cancel();
}

ForwardingTableEntry::result_t ForwardingTableEntry::getNextHop() {
  if (expire_time_) {
    // Hit: Reset timer
    timer_.cancel();
    timer_.expires_from_now(dur_to_expire_);
    timer_.async_wait(boost::bind(&ForwardingTable::onEntryExpired,
                                  forwarding_table_,
                                  boost::asio::placeholders::error,
                                  sessionID_));
  }
  return nextHop();
}

SimpleForwardingTableEntry::SimpleForwardingTableEntry(
    const rapidjson::Value& v,
    boost::asio::io_context& ios,
    const std::string& sessionID,
    ForwardingTable* forwarding_table,
    uint32_t expire_time) :
    ForwardingTableEntry(ios, sessionID, forwarding_table, expire_time) {
  // Assert the JSON message
  if (!v.IsArray()) {
    throw std::invalid_argument("Entry should be an JSON Array");
  }

  // Construct from the JSON document
  for (rapidjson::SizeType i = 0; i < v.Size(); ++i) {
    if (!v[i].IsUint()) {
      throw std::invalid_argument("NodeID should be usigned int!");
    }
    nexthops_.insert(v[i].GetUint());
  }
}

SimpleForwardingTableEntry::SimpleForwardingTableEntry(
    const result_t& result,
    boost::asio::io_context& ios,
    const std::string& sessionID,
    ForwardingTable* forwarding_table,
    uint32_t expire_time) :
    ForwardingTableEntry(ios, sessionID, forwarding_table, expire_time),
    nexthops_(result) {
  if (result.empty()) {
    throw std::invalid_argument("Nexthop should not be empty!");
  }
}

ForwardingTableEntry::result_t SimpleForwardingTableEntry::nextHop() const {
  return nexthops_;
}

SplitterForwardingTableEntry::SplitterForwardingTableEntry(
    const rapidjson::Value& v,
    boost::asio::io_context& ios,
    const std::string& sessionID,
    ForwardingTable* forwarding_table,
    uint32_t expire_time) :
    ForwardingTableEntry(ios, sessionID, forwarding_table, expire_time) {
  // Assert the JSON message
  if (!v.IsArray()) {
    throw std::invalid_argument("Entry should be an JSON Array");
  }
  for (rapidjson::SizeType i = 0; i < v.Size(); ++i) {
    if (!v[i].HasMember("NextHop") || !v[i].HasMember("Weight")) {
      throw std::invalid_argument("NextHop missing for SimpleEntry " +
                                  boost::lexical_cast<std::string>(sessionID));
      return;
    }
    if (!v[i]["NextHop"].IsUint()) {
      throw std::invalid_argument("NodeID should be usigned int!");
    }
    if (!v[i]["Weight"].IsDouble()) {
      throw std::invalid_argument("Weight should be double!");
    }
  }

  // Construct from the JSON document
  total_weight_ = 0;
  double weight;
  for (rapidjson::SizeType i = 0; i < v.Size(); ++i) {
    weight = v[i]["Weight"].GetDouble();
    total_weight_ += weight;
    weighted_nexthop_.insert({weight, v[i]["NextHop"].GetUint()});
  }
}

ForwardingTableEntry::result_t SplitterForwardingTableEntry::nextHop() const {
  double rd = (double) std::rand() / RAND_MAX * total_weight_;
  MapType::const_iterator i;
  // Iterate until the random generated weight is lower than a threshold.
  for (i = weighted_nexthop_.begin(); i != weighted_nexthop_.end(); ++i) {
    if (i->first >= rd) break;
    else {
      rd -= i->first;
    }
  }
  return {i->second};
}

GenericForwardingTableEntry::GenericForwardingTableEntry(
    const rapidjson::Value& v,
    boost::asio::io_context& ios,
    const std::string& sessionID,
    ForwardingTable* forwarding_table,
    uint32_t expire_time) :
    ForwardingTableEntry(ios, sessionID, forwarding_table, expire_time) {
  if (!v.IsArray()) {
    throw std::invalid_argument("Entry should be an JSON Array");
  }
  for (rapidjson::SizeType i = 0; i < v.Size(); ++i) {
    const rapidjson::Value& subv = v[i];
    // Assert the sub JSON message
    if (!subv.IsArray()) {
      throw std::invalid_argument("Subentry should be an JSON Array");
    }
    for (rapidjson::SizeType i = 0; i < subv.Size(); ++i) {
      if (!subv[i].HasMember("NextHop") || !subv[i].HasMember("Weight")) {
        throw std::invalid_argument("NextHop missing for SimpleEntry " +
                                    boost::lexical_cast<std::string>(sessionID));
        return;
      }
      if (!subv[i]["NextHop"].IsUint()) {
        throw std::invalid_argument("NodeID should be usigned int!");
      }
      if (!subv[i]["Weight"].IsDouble()) {
        throw std::invalid_argument("Weight should be double!");
      }
    }

    // Construct from the JSON document
    MapType subweighted_map;
    double subtotal_weight = 0;
    double weight;
    for (rapidjson::SizeType i = 0; i < subv.Size(); ++i) {
      weight = subv[i]["Weight"].GetDouble();
      subtotal_weight += weight;
      subweighted_map.insert({weight, subv[i]["NextHop"].GetUint()});
    }

    weighted_nexthops_.emplace_back(subtotal_weight, subweighted_map);
  }

  weighted_nexthops_.shrink_to_fit();
}

ForwardingTableEntry::result_t GenericForwardingTableEntry::nextHop() const {
  result_t result;
  // For each replication, do
  for (std::vector<std::pair<double, MapType>>::const_iterator
       j = weighted_nexthops_.begin(); j != weighted_nexthops_.end(); ++j) {
    const MapType& weighted_nexthop = j->second;
    double rd = std::rand() * (j->first);
    MapType::const_iterator i;
    // Iterate until the random generated weight is lower than a threshold.
    for (i = weighted_nexthop.begin(); i != weighted_nexthop.end(); ++i) {
      if (i->first >= rd) break;
      else {
        rd -= i->first;
      }
    }
    result.insert(i->second);
  }

  return result;
}

}
