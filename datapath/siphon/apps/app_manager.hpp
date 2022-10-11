//
// Created by Shuhao Liu on 2018-11-19.
//

#ifndef SIPHON_APP_MANAGER_HPP
#define SIPHON_APP_MANAGER_HPP

#include <shared_mutex>
#include <string>
#include <unordered_map>

#include "base/minion_pool.hpp"
#include "apps/pseudo_app.hpp"


namespace siphon {

class AppManager : public IMinionStop {
 public:
  using SourceMapType =
      std::unordered_map<std::string, std::unique_ptr<AbstractMessageProducer>>;
  using SinkMapType =
  std::unordered_map<std::string, std::unique_ptr<AbstractMessageConsumer>>;

  AppManager(boost::asio::io_context& ios,
             MinionPool* pool,
             nodeID_t local_nodeID);

  ~AppManager() = default;

  IMinionStop* process(Minion* minion) override;

  void createPseudoApps(IMinionStop* crossbar);

  AbstractMessageConsumer* getSinkApp(const std::string& sessionID) const;
  AbstractMessageProducer* getSourceApp(const std::string& sessionID) const;

 private:
  boost::asio::io_context& ios_;
  MinionPool* pool_;
  IMinionStop* crossbar_;
  const nodeID_t local_nodeID_;

  SourceMapType sources_;
  SinkMapType sinks_;

  mutable std::shared_mutex map_mtx_;
};

}

#endif //SIPHON_APP_MANAGER_HPP
