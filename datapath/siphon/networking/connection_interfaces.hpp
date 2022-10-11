//
// Created by Shuhao Liu on 2018-08-19.
//

#ifndef SIPHON_CONNECTION_INTERFACES_HPP
#define SIPHON_CONNECTION_INTERFACES_HPP

#include <string>

#include "base/minion_pool.hpp"
#include "util/types.hpp"


namespace siphon {

using boost::asio::ip::address_v4;

// To send, ISender --> ISender::q_ --> ISenderSocketWrapper.
class ISenderSocketWrapper : public AbstractMessageConsumer {
 public:
  ISenderSocketWrapper(MinionPool* pool, MinionStopQueueType* queue) :
      AbstractMessageConsumer(pool), q_(queue) {}
  virtual ~ISenderSocketWrapper() override = default;

  void startSending() {
    q_->request(this);
  }

 protected:
  virtual void onDataSent(const boost::system::error_code& ec,
                          size_t bytes_transferred) = 0;

 protected:
  MinionStopQueueType* q_;
};

class ISender : public IMinionStop {
 public:
  explicit ISender(MinionPool* pool) : pool_(pool) {}
  virtual ~ISender() = default;

  virtual void startSending() = 0;

  virtual void onAck(Message* ack) = 0;

 protected:
  MinionPool* pool_;

  MinionStopQueueType q_;

  std::unique_ptr<ISenderSocketWrapper> socket_;
};

class IReceiver : public AbstractMessageProducer {
 public:
  explicit IReceiver(MinionPool* pool) : AbstractMessageProducer(pool) {}
  virtual ~IReceiver() override = default;

  virtual void startReceiving() = 0;

 protected:
  virtual void onDataReceived(
      const boost::system::error_code& ec,
      size_t bytes_transferred) = 0;
};

class IConnectionManager {
 public:
  virtual ~IConnectionManager() = default;
  virtual void init(IMinionStop* crossbar) = 0;
  virtual bool shouldInitiateConnectionTo(nodeID_t nodeID) = 0;
  virtual ISender* create(const std::string& url, nodeID_t nodeID) = 0;
  virtual ISender* getSender(nodeID_t nodeID) = 0;
  virtual IReceiver* getReceiver(nodeID_t nodeID) = 0;
  virtual void remove(nodeID_t nodeID) = 0;
};

}

#endif //SIPHON_UDPCONNECTIONINTERFACES_HPP
