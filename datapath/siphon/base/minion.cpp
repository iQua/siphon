#include "base/minion.hpp"


namespace siphon {

Minion::Minion(IMinionStop* first_stop) :
    next_stop_(first_stop),
    data_(std::make_unique<Message>()) {
}

void Minion::recycle() {
  if (data_) data_->recycle();
  else {
    data_ = std::make_unique<Message>();
    LOG_WARNING << "Re-create Message in Minion when recycling. Not desired!";
  }
  extra_data_.clear();
}

void Minion::start() {
  while (next_stop_ != nullptr) {
    next_stop_ = next_stop_->process(this);
  }
}

}
