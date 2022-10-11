//
// Created by Shuhao Liu on 2018-11-19.
//

#include "apps/app_manager.hpp"

#include <mutex>

#include "util/config.hpp"


namespace siphon {

AppManager::AppManager(boost::asio::io_context& ios,
                       MinionPool* pool,
                       nodeID_t local_nodeID) :
    ios_(ios), pool_(pool), crossbar_(nullptr), local_nodeID_(local_nodeID) {
}

IMinionStop* AppManager::process(Minion* minion) {
  assert(crossbar_ != nullptr);
  const std::string& sessionID = minion->getData()->header()->session_id();
  std::shared_lock<std::shared_mutex> lock(map_mtx_);
  auto sink_iterator = sinks_.find(sessionID);
  if (sink_iterator == sinks_.end()) {
    LOG_WARNING << "No PseudoAppReceiver found, message dropped for sessionID: "
                << sessionID;
    return pool_;
  }
  return sink_iterator->second.get();
}

void AppManager::createPseudoApps(IMinionStop* crossbar) {
  crossbar_ = crossbar;
  const std::list<PseudoSessionConfig>& sessions =
      util::Config::get()->pseudo_session_configs_;
  std::unique_lock<std::shared_mutex> lock(map_mtx_);
  for (const auto& session : sessions) {
    if (session.src == local_nodeID_) {
      sources_[session.sessionID] = std::make_unique<PseudoAppSource>(
          ios_, pool_, crossbar_, session);
    }

    if (session.dst == local_nodeID_) {
      sinks_[session.sessionID] = std::make_unique<PseudoAppReceiver>(
          pool_, session.sessionID);
    }
  }
}

AbstractMessageConsumer* AppManager::getSinkApp(
    const std::string& sessionID) const {
  std::shared_lock<std::shared_mutex> lock(map_mtx_);
  auto sink_iterator = sinks_.find(sessionID);
  if (sink_iterator == sinks_.end()) {
    LOG_WARNING << "No SinkApp found for sessionID: " << sessionID;
    return nullptr;
  }
  return sink_iterator->second.get();
}

AbstractMessageProducer* AppManager::getSourceApp(
    const std::string& sessionID) const {
  std::shared_lock<std::shared_mutex> lock(map_mtx_);
  auto source_iterator = sources_.find(sessionID);
  if (source_iterator == sources_.end()) {
    LOG_WARNING << "No Source found for sessionID: " << sessionID;
    return nullptr;
  }
  return source_iterator->second.get();
}

}
