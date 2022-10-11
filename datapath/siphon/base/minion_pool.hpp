#ifndef MINION_POOL_HPP_
#define MINION_POOL_HPP_

#include "base/minion.hpp"

#include <atomic>

#include <boost/lockfree/stack.hpp>
#include <boost/lockfree/queue.hpp>


namespace siphon {

using namespace std;
using namespace boost::lockfree;

#ifndef SIPHON_MINION_POOL_SIZE
#define SIPHON_MINION_POOL_SIZE 256
#endif

template <typename T>
class ConcurrentQueue {
 private:
  typedef queue<T*, capacity<SIPHON_MINION_POOL_SIZE>> QueueType;

 public:
  ConcurrentQueue() = default;

  T* pop() {
    T* t = nullptr;
    bool succeeded = q_.pop(t);
    if (succeeded) {
      return t;
    }
    else {
      return nullptr;
    }
  }

  T* spinPop() {
    T* t = nullptr;
    while (!t) {
      if (q_.pop(t)) return t;
    }
    return t;
  }

  void push(T* t) {
    q_.push(t);
  }

 private:
  QueueType q_;
};

class MinionStopQueueType : public IRequestableMinionStop {
 public:
  void request(IMinionStop* requester) override;

  IMinionStop* process(Minion* minion) override;

 protected:
  ConcurrentQueue<Minion> minion_q_;

  ConcurrentQueue<IMinionStop> requester_q_;

  std::atomic_int32_t waiting_minions_{0};
};

class MinionPool : public MinionStopQueueType {
 public:
  MinionPool();

  ~MinionPool() = default;

  void create(size_t size);

  IMinionStop* process(Minion* minion) override;

 private:
  unique_ptr<Minion>* minions_;
};

class AbstractMessageProducer : public IMinionStop {
 public:
  AbstractMessageProducer(MinionPool* pool) : pool_(pool) {
  }

  virtual ~AbstractMessageProducer() = default;

 protected:
  MinionPool* pool_;
};


class AbstractMessageConsumer : public IMinionStop {
 public:
  AbstractMessageConsumer(MinionPool* pool) : pool_(pool) {
  }

  virtual ~AbstractMessageConsumer() = default;

 protected:
  MinionPool* pool_;
};


}

#endif