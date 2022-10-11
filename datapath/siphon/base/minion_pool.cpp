#include "base/minion_pool.hpp"


namespace siphon {

void MinionStopQueueType::request(IMinionStop* requester) {
  if (waiting_minions_.fetch_sub(1) > 0) {
    // There is a minion in queue ready for this request, spinPop
    Minion* minion = minion_q_.spinPop();
    minion->wakeup(requester);
  }
  else {
    requester_q_.push(requester);
  }
}

IMinionStop* MinionStopQueueType::process(Minion* minion) {
  if (waiting_minions_.fetch_add(1) < 0) {
    return requester_q_.spinPop();
  }
  else {
    minion_q_.push(minion);
    return nullptr;
  }
}

MinionPool::MinionPool() : minions_(nullptr) {
}

void MinionPool::create(size_t size) {
  assert(size <= SIPHON_MINION_POOL_SIZE);
  delete[] minions_;
  minions_ = new unique_ptr<Minion>[size];
  for (size_t i = 0; i < size; i++) {
    minions_[i].reset(new Minion((IMinionStop*) this));
    // The created minion will be pushed to minion_manager_ by waking up
    minions_[i]->wakeup(this);
  }
}

IMinionStop* MinionPool::process(Minion* minion) {
  minion->recycle();
  return MinionStopQueueType::process(minion);
}

}