#ifndef MINION_HPP_
#define MINION_HPP_

#include <memory>
#include <vector>
#include <list>

#include "base/message.hpp"
#include "util/logger.hpp"


namespace siphon {

using namespace std;

class IMinionStop;

class Minion {
  friend class MinionPool;
 public:
  /**
   * Wake up this minion, which will then proceed with the provided MinionStop.
   *
   * Important Note: it is possible that anything called after this statement
   * will be delayed for execution, because the Minion coroutine loop might
   * continuously occupy the thread (and this is the desired behaviour).
   *
   * @param proceed_stop
   */
  void wakeup(IMinionStop* proceed_stop) {
    next_stop_ = proceed_stop;
    start();
  }

  Message* getData() {
    return data_.get();
  }

  list<unique_ptr<Message>>* getExtraData() {
    return &extra_data_;
  }

  void swapData(unique_ptr<Message>& peer) {
    data_.swap(peer);
  }

  void recycle();

  /**
   * Non-copyable
   */
  Minion(const Minion&) = delete;

  /**
   * Non-assignable
   */
  Minion& operator=(const Minion&) = delete;

 protected:
  /**
   * Constructor. Left protected to avoid being created by objects other than a
   * MinionPool.
   * @param first_stop The first stop of the Minion. Usually the MinionPool.
   */
  explicit Minion(IMinionStop* first_stop);

  virtual void start();

 protected:
  IMinionStop* next_stop_;

  unique_ptr<Message> data_;

  list<unique_ptr<Message>> extra_data_;
};

class IMinionStop {
 public:
  /**
   * Called by Minion instances to process data at the stop.
   * Important Node: This function will be executed fully in coroutines.
   * Anything in the call stack should *not* invoke Minion::wakeup().
   *
   * @param minion The minion carrying the incoming data.
   * @return Next MinionStop to process the Minion. If cannot process the data
   *         held in the Minion, return a nullptr.
   */
  virtual IMinionStop* process(Minion* minion) = 0;
};

class IRequestableMinionStop : public IMinionStop {
  virtual void request(IMinionStop* requester) = 0;
};

}

#endif