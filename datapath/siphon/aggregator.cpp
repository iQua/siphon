/**
 * @file SiphonWorker.cpp
 * @brief Define the SiphonWorker class, which contains all components used by
 *        a SiphonWorker node and maintains the workflow.
 * @author Shuhao Liu <shuhao@ece.toronto.edu>
 *
 * @date 2016-01-08
 * @ingroup application
 */

#include "aggregator.hpp"

#include "networking/tcp_connection_manager.hpp"
#include "networking/udp_connection_manager.hpp"


namespace siphon {

Aggregator::Aggregator(boost::asio::io_context &ios) :
    ios_(ios),
    thread_pool_(ios) {
}

void Aggregator::start(const std::string& controller_url) {
  // Start controller first.
  controller_ = std::make_unique<ControllerProxy>(
      ios_, controller_url, util::Config::get()->kControllerPortNumber);
  local_nodeID_ = controller_->local_nodeID();

  // Create components.
  center_ = std::make_unique<NotificationCenterType>(controller_.get());
  minion_pool_ = std::make_unique<MinionPool>();
  minion_pool_->create(256);

  app_manager_ = std::make_unique<AppManager>(
      ios_, minion_pool_.get(), local_nodeID_);
  node_manager_ = std::make_unique<NodeManager>(
      app_manager_.get(), local_nodeID_);
  if (util::Config::get()->kUseTCP) {
    connection_manager_ = std::make_unique<TCPConnectionManager>(
        ios_, minion_pool_.get(), local_nodeID_);
  }
  else {
    connection_manager_ = std::make_unique<UDPConnectionManager>(
        ios_, minion_pool_.get(), local_nodeID_);
  }

  crossbar_ = std::make_unique<Crossbar>(
      ios_, minion_pool_.get(), app_manager_.get(), connection_manager_.get());
  crossbar_->setLocalNodeID(local_nodeID_);

  // Start components
  app_manager_->createPseudoApps(crossbar_.get());
  connection_manager_->init(crossbar_.get());
  center_->registerNotification<Crossbar, ControlMsgNotificationType>(
      crossbar_.get());
  controller_->setup(node_manager_.get(),
                     crossbar_.get(),
                     connection_manager_.get());

  thread_pool_.run();
}

void Aggregator::waitUntilErrorDetected() {
  thread_pool_.waitUntilErrorDetected();
}


}
