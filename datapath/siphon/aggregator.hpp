/**
 * @file SiphonWorker.hpp
 * @brief Define the SiphonWorker class, which contains all components used by
 *        a SiphonWorker node and maintains the workflow.
 * @author Shuhao Liu <shuhao@ece.toronto.edu>
 *
 * @date 2016-01-08
 * @ingroup application
 */

#ifndef SIPHON_WORKER_H_
#define SIPHON_WORKER_H_

#include "apps/app_manager.hpp"
#include "base/message.hpp"
#include "base/minion_pool.hpp"
#include "base/thread_pool.hpp"
#include "controller/controller_proxy.hpp"
#include "controller/crossbar.hpp"
#include "controller/node_manager.hpp"
#include "controller/notification_center.hpp"
#include "networking/connection_interfaces.hpp"
#include "util/config.hpp"
#include "util/logger.hpp"


namespace siphon {

class Aggregator {
 private:
  typedef NotificationCenter<ControlMsgNotificationType> NotificationCenterType;

 public:
  Aggregator(boost::asio::io_context& ios);

  ~Aggregator() = default;

  void start(const std::string& controller_url);

  void waitUntilErrorDetected();

 protected:
  boost::asio::io_context& ios_;

  nodeID_t local_nodeID_;

  ThreadPool thread_pool_;

  std::unique_ptr<ControllerProxy> controller_;

  std::unique_ptr<NotificationCenterType> center_;

  std::unique_ptr<MinionPool> minion_pool_;

  std::unique_ptr<AppManager> app_manager_;

  std::unique_ptr<NodeManager> node_manager_;

  std::unique_ptr<IConnectionManager> connection_manager_;

  std::unique_ptr<Crossbar> crossbar_;
};

}

#endif
