/**
 * @file ThreadPool.cpp
 * @brief Implement the worker thread pool. The threads in the thread pool will
 *        invoke `io_service.run()` to process the posted handlers.
 *
 * @author Zimu Liu (original author)
 * @author Shuhao Liu <shuhao@ece.toronto.edu>
 *
 * @ingroup fundamentals
 * @date 2015-11-05
 */

#include "base/thread_pool.hpp"

#include <boost/bind.hpp>

#include "util/logger.hpp"


namespace siphon {

using namespace std;

ThreadPool::ThreadPool(boost::asio::io_context& ios, size_t thread_num) :
    boost::thread_group(),
    ios_ref_(ios),
    thread_num_(thread_num),
    work_(boost::asio::make_work_guard(ios)) {
  // Initialize the io_service instance by resetting it.
  ios_ref_.reset();

  // Set the appropriate number of threads
  if (thread_num_ == 0) { // The default setting: # of cpu cores
    thread_num_ = boost::thread::hardware_concurrency();
  }
  if (thread_num_ < MIN_THREAD_NUM) { // Normalize by the min setting
    thread_num_ = MIN_THREAD_NUM;
  }
}

ThreadPool::~ThreadPool() {
  if (!ios_ref_.stopped()) stop();
  join_all();
}

void ThreadPool::run() {
  using namespace boost::asio;
  // Spawning the threads, each of which calls threadAttachToIOServiceSafely().
  for (size_t i = 0; i < thread_num_; i++) {
    create_thread(
        boost::bind(&ThreadPool::threadAttachToIOServiceSafely, this));
  }
}

void ThreadPool::stop() {
  work_.reset();
  ios_ref_.stop();
}

void ThreadPool::waitUntilErrorDetected() {
  join_all();
}

void ThreadPool::cancelAfterSeconds(int seconds) {
  boost::this_thread::sleep_for(boost::chrono::seconds(seconds));
  this->stop();
}

void ThreadPool::threadAttachToIOServiceSafely() {
  boost::system::error_code error;
  try {
    ios_ref_.run(error);
  }
  catch (const boost::system::error_code& error) {
    if (error && error.value() != boost::system::errc::operation_canceled) {
      LOG_FATAL << "FATAL Error: " << error.message();
    }
  }
  stop();
}

}
