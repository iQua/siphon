#include "gtest/gtest.h"

#include "base/thread_pool.hpp"
#include "base/minion_pool.hpp"
#include "util/logger.hpp"

#include <mutex>


namespace siphon {

using namespace std::chrono;

class TimedLog : public Message {
 public:
  TimedLog() : index_(0) {
    setNext();
  }

  void setNext() {
    if (index_ >= LOG_LENGTH) {
      LOG_ERROR << "# of logs exceeds LOG_LENGTH!";
      throw std::out_of_range("# of logs exceeds LOG_LENGTH!");
    }
    logs_[index_] = high_resolution_clock::now();
    index_++;
  }

  void print() {
    high_resolution_clock::time_point* prev = &(logs_[0]);
    high_resolution_clock::time_point* curr = &(logs_[0]);
    std::ostringstream ss;
    for (int i = 1; i < index_; i++) {
      curr++;
      duration<double> span = duration_cast<duration<double>>(*curr - *prev);
      ss << std::setw(10) << span.count() * 1000;
      prev++;
    }
    LOG_INFO << ss.str();
  }

 private:
  const static int LOG_LENGTH = 6;
  int index_;
  high_resolution_clock::time_point logs_[LOG_LENGTH];
};

class TimedWorkProducer : public IMinionStop {
 public:
  TimedWorkProducer(boost::asio::io_context& ios,
                    int rate,
                    MinionPool* pool,
                    IMinionStop* next_stop,
                    int total = 1000) :
      ios_(ios), interval_(1000 / rate),
      pool_(pool), next_stop_(next_stop),
      count_(total) {
    onTimeout(boost::system::error_code());
  }

  void onTimeout(const boost::system::error_code& e) {
    if (e == boost::asio::error::operation_aborted) {
      // Timer was not cancelled, take necessary action.
      LOG_INFO << "Producer Timer canceled";
      return;
    }
    pool_->request(this);
  }

  IMinionStop* process(Minion* minion) override {
    count_--;
    // Record the time when the data is taken by a Minion
    auto log = std::unique_ptr<Message>(new TimedLog());
    ((TimedLog*) (log.get()))->setNext();
    minion->swapData(log);
    // Schedule next
    if (count_ > 0) {
      timer_.reset(new boost::asio::deadline_timer(ios_));
      timer_->expires_from_now(boost::posix_time::milliseconds(interval_));
      timer_->async_wait(boost::bind(&TimedWorkProducer::onTimeout, this, _1));
    }
    else {
      timer_.reset();
    }

    return next_stop_;
  }

 private:
  boost::asio::io_context& ios_;
  const int interval_;
  MinionPool* pool_;
  IMinionStop* next_stop_;
  int count_;
  std::unique_ptr<boost::asio::deadline_timer> timer_;
};

class MinionDestination : public IMinionStop {
 public:
  MinionDestination(boost::asio::io_context& ios,
                    MinionStopQueueType* q, MinionPool* pool, int rate, int t) :
      ios_(ios), q_(q), pool_(pool), interval_(1000 / rate), total_(t) {
    onTimeout(boost::system::error_code());
  }

  IMinionStop* process(Minion* minion) override {
    using namespace std;
    auto data = std::unique_ptr<Message>(new TimedLog());
    minion->swapData(data);
#ifdef TEST_TIMED_LOG_PRINT
    ((TimedLog*) data.get())->print();
#endif
    std::lock_guard<std::recursive_mutex> lock(mtx_);
    total_--;
    return pool_;
  }

  void onTimeout(const boost::system::error_code& e) {
    if (e == boost::asio::error::operation_aborted) {
      // Timer was not cancelled, take necessary action.
      LOG_INFO << "Producer Timer canceled";
      return;
    }
    // Schedule next
    std::lock_guard<std::recursive_mutex> lock(mtx_);
    if (total_ >= 0) {
      if (total_ == 0) {
        total_--;
      }
      else {
        // Generate a TimedLog
        q_->request(this);
      }
      timer_ = std::make_unique<boost::asio::deadline_timer>(ios_);
      timer_->expires_from_now(boost::posix_time::milliseconds(interval_));
      timer_->async_wait(boost::bind(&MinionDestination::onTimeout, this, _1));
    }
    else {
      timer_.reset();
      ios_.stop();
    }
  }

 private:
  boost::asio::io_context& ios_;
  MinionStopQueueType* q_;
  MinionPool* pool_;
  const int interval_;
  int total_;
  std::unique_ptr<boost::asio::deadline_timer> timer_;
  std::recursive_mutex mtx_;
};

class MinionTest : public ::testing::Test {
 protected:
  MinionTest() : thread_pool_(ios_, 4) {
    // You can do set-up work for each test here.
    minion_pool_.reset(new MinionPool());
    thread_pool_.run();
  }

  virtual ~MinionTest() {
    // You can do clean-up work that doesn't throw exceptions here.
    minion_pool_.reset();
    thread_pool_.stop();
  }

 protected:
  // Objects declared here can be used by all tests in the test case for Foo.
  boost::asio::io_context ios_;
  std::unique_ptr<MinionPool> minion_pool_;
  ThreadPool thread_pool_;
};

TEST_F(MinionTest, SingleProducerSingleMinionOverflow) {
  minion_pool_->create(1);
  MinionStopQueueType queue;
  int total = 200;
  MinionDestination destination(ios_, &queue, minion_pool_.get(), 100, total);
  TimedWorkProducer producer(ios_, 1000, minion_pool_.get(), &queue, total);
  high_resolution_clock::time_point start = high_resolution_clock::now();
  thread_pool_.waitUntilErrorDetected();
  high_resolution_clock::time_point end = high_resolution_clock::now();
  duration<double> span = duration_cast<duration<double>>(end - start);
  std::cout << "Time spent: " << span.count() << " s." << std::endl;
  EXPECT_TRUE(span.count() > total/100);
  EXPECT_TRUE(true);
}


TEST_F(MinionTest, SingleProducerSingleMinionUnderflow) {
  minion_pool_->create(1);
  MinionStopQueueType queue;
  int total = 200;
  MinionDestination destination(ios_, &queue, minion_pool_.get(), 1000, total);
  TimedWorkProducer producer(ios_, 100, minion_pool_.get(), &queue, total);
  high_resolution_clock::time_point start = high_resolution_clock::now();
  thread_pool_.waitUntilErrorDetected();
  high_resolution_clock::time_point end = high_resolution_clock::now();
  duration<double> span = duration_cast<duration<double>>(end - start);
  std::cout << "Time spent: " << span.count() << " s." << std::endl;
  EXPECT_TRUE(span.count() > total/100);
  EXPECT_TRUE(true);
}


TEST_F(MinionTest, SingleProducerManyMinionOverflow) {
  minion_pool_->create(100);
  MinionStopQueueType queue;
  int total = 1000;
  MinionDestination destination(ios_, &queue, minion_pool_.get(), 100, total);
  TimedWorkProducer producer(ios_, 1000, minion_pool_.get(), &queue, total);
  high_resolution_clock::time_point start = high_resolution_clock::now();
  thread_pool_.waitUntilErrorDetected();
  high_resolution_clock::time_point end = high_resolution_clock::now();
  duration<double> span = duration_cast<duration<double>>(end - start);
  std::cout << "Time spent: " << span.count() << " s." << std::endl;
  EXPECT_TRUE(span.count() > total/100);
  EXPECT_TRUE(true);
}


TEST_F(MinionTest, ManyProducerSingleMinionOverflow) {
  minion_pool_->create(1);
  MinionStopQueueType queue;
  int total = 1000;
  MinionDestination destination(ios_, &queue, minion_pool_.get(), 100, total);
  TimedWorkProducer* producers[10];
  for (int i = 0; i < 10; i++) {
    producers[i] = new TimedWorkProducer(
        ios_, 1000, minion_pool_.get(), &queue, total / 10);
  }
  high_resolution_clock::time_point start = high_resolution_clock::now();
  thread_pool_.waitUntilErrorDetected();
  high_resolution_clock::time_point end = high_resolution_clock::now();
  duration<double> span = duration_cast<duration<double>>(end - start);
  std::cout << "Time spent: " << span.count() << " s." << std::endl;
  EXPECT_TRUE(span.count() > total/100);
  EXPECT_TRUE(true);
  for (int i = 0; i < 10; i++) {
    delete producers[i];
  }
}


TEST_F(MinionTest, ManyProducerManyMinionOverflow) {
  minion_pool_->create(100);
  MinionStopQueueType queue;
  int total = 1000;
  MinionDestination destination(ios_, &queue, minion_pool_.get(), 100, total);
  TimedWorkProducer* producers[10];
  for (int i = 0; i < 10; i++) {
    producers[i] = new TimedWorkProducer(
        ios_, 1000, minion_pool_.get(), &queue, total / 10);
  }
  high_resolution_clock::time_point start = high_resolution_clock::now();
  thread_pool_.waitUntilErrorDetected();
  high_resolution_clock::time_point end = high_resolution_clock::now();
  duration<double> span = duration_cast<duration<double>>(end - start);
  std::cout << "Time spent: " << span.count() << " s." << std::endl;
  EXPECT_TRUE(span.count() > total/100);
  EXPECT_TRUE(true);
  for (int i = 0; i < 10; i++) {
    delete producers[i];
  }
}

TEST_F(MinionTest, ManyProducerManyMinionUnderflow) {
  minion_pool_->create(100);
  MinionStopQueueType queue;
  int total = 2000;
  MinionDestination destination(ios_, &queue, minion_pool_.get(), 1000, total);
  high_resolution_clock::time_point start = high_resolution_clock::now();
  TimedWorkProducer* producers[10];
  for (int i = 0; i < 10; i++) {
    producers[i] = new TimedWorkProducer(
        ios_, 40, minion_pool_.get(), &queue, total / 10);
  }
  thread_pool_.waitUntilErrorDetected();
  high_resolution_clock::time_point end = high_resolution_clock::now();
  duration<double> span = duration_cast<duration<double>>(end - start);
  std::cout << "Time spent: " << span.count() << " s."<< std::endl;
  EXPECT_TRUE(span.count() > total / 40 / 10);
  EXPECT_TRUE(true);
  for (int i = 0; i < 10; i++) {
    delete producers[i];
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

}
