/**
 * @file PseudoApp.hpp
 * @brief Define an pseudo application, using the APIs provided by Siphon to
 *        send and receive data.
 *
 * @author Shuhao Liu <shuhao@ece.toronto.edu>
 *
 * @date 2015-12-19
 * @ingroup application
 */

#include "apps/pseudo_app.hpp"

#include <boost/bind.hpp>


namespace siphon {

TokenBucket::TokenBucket(boost::asio::io_context& ios,
                         const double average_rate,
                         const size_t burst_size) :
    ios_(ios), rho_(average_rate), sigma_(burst_size),
    refill_timer_(ios), should_send_ontoken_available_(true) {
  refill_amount_ = 1;
  double freq = average_rate;
//  while (freq > 1 / kTokenBucketMinTimerInterval) {
//    refill_amount_ *= 2;
//    freq /= 2;
//  }
  bucket_depth_ = refill_amount_ + sigma_;
  refill_interval_ = boost::posix_time::microseconds((long) (1.0e6 / freq));
}

void TokenBucket::start() {
  last_refill_time_ = boost::posix_time::microsec_clock::universal_time();
  bucket_level_ = bucket_depth_;
  generateOneToken(boost::system::error_code());
}

void TokenBucket::generateOneToken(const boost::system::error_code& ec) {
  if (ec == boost::asio::error::operation_aborted) return;
  if (ec) LOG_ERROR << "TokenBucket Error: " << ec.message();

  const boost::posix_time::ptime now =
      boost::posix_time::microsec_clock::universal_time();
  std::unique_lock<std::mutex> grd(level_mtx_);
  if (now > last_refill_time_) {
    if (bucket_level_ < bucket_depth_) {
      const boost::posix_time::time_duration elapsed = now - last_refill_time_;
      bucket_level_ += refill_amount_ * (double) elapsed.total_microseconds()
          / refill_interval_.total_microseconds();
      bucket_level_ = (bucket_level_ < bucket_depth_ ?
                       bucket_level_ : bucket_depth_);
    }
  }
  last_refill_time_ = now;

  refill_timer_.expires_from_now(refill_interval_);
  refill_timer_.async_wait(boost::bind(&TokenBucket::generateOneToken, this,
                                       boost::asio::placeholders::error));

  bool should_call = true;
  if (should_send_ontoken_available_.compare_exchange_strong(
          should_call, false)) {
    ios_.post(boost::bind(&TokenBucket::onTokenAvailable, this));
  }
}

bool TokenBucket::consumeOneToken() {
  std::unique_lock<std::mutex> grd(level_mtx_);
  bool available = (bucket_level_ >= 1);
  if (available) bucket_level_--;
  else should_send_ontoken_available_.store(true);
  return available;
}

PseudoAppSource::PseudoAppSource(boost::asio::io_context& ios,
                                 MinionPool* pool,
                                 IMinionStop* next_stop,
                                 const PseudoSessionConfig& config) :
    TokenBucket(ios, config.average_rate, config.burst_size),
    AbstractMessageProducer(pool),
    next_stop_(next_stop),
    sessionID_(config.sessionID),
    seq_(0),
    message_size_(config.message_size) {
}


IMinionStop* PseudoAppSource::process(Minion* minion) {
  minion->getData()->mutable_header()->set_session_id(sessionID_);
  minion->getData()->mutable_header()->set_seq(seq_++);

  const boost::asio::mutable_buffer buf = minion->getData()->allocateBuffer();
  minion->getData()->resetPayload((uint8_t*) buf.data(), message_size_);

  ios_.post(boost::bind(&PseudoAppSource::onTokenAvailable, this));
  return next_stop_;
}

void PseudoAppSource::onTokenAvailable() {
  if (consumeOneToken()) {
    pool_->request(this);
  }
  else {
    should_send_ontoken_available_.store(true);
  }
}

PseudoAppReceiver::PseudoAppReceiver(MinionPool* pool,
                                     const std::string& sessionID) :
    AbstractMessageConsumer(pool),
    sessionID_(sessionID),
    counter_(0),
    last_report_time_(boost::posix_time::microsec_clock::universal_time()) {
}

IMinionStop* PseudoAppReceiver::process(Minion* minion) {
  boost::posix_time::time_duration t =
      boost::posix_time::microsec_clock::universal_time() - last_report_time_;
  counter_ += minion->getData()->header()->payload_size();
  if (t.total_microseconds() >= 5.0e6) {
    double rate = counter_ / (double) t.total_microseconds() * 1.0e6;
    LOG_INFO << "Session# " << sessionID_ << " receiving "
        << rate << " Bps";
    counter_ = 0;
    last_report_time_ = boost::posix_time::microsec_clock::universal_time();
  }

  return pool_;
}

}
