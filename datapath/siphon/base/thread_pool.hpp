/**
 * @file IOSThreadPool.hpp
 * @brief Define the worker thread pool. The threads in the thread pool will
 *        invoke `io_service.run()` to process the posted handlers.
 *
 * @author Zimu Liu (original author)
 * @author Shuhao Liu <shuhao@ece.toronto.edu>
 *
 * @ingroup fundamentals
 * @date 2015-11-05
 */

#ifndef IOS_THREAD_POOL_HPP_
#define IOS_THREAD_POOL_HPP_

#include <memory>

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/utility.hpp>


namespace siphon {

using namespace std;

/**
 * The minimum number of worker threads in the thread pool.
 */
#define MIN_THREAD_NUM 1

/**
 * @brief IOSThreadPool is a wrapper of multiple worker threads that are
 *        invoking `io_service.run()`.
 *
 * Inherited from `boost::thread_group` class, it is a non-copyable class.
 * User of the class should initiate the class with a valid `io_service`
 * object, and call member function `run()` explicitly.
 *
 */
class ThreadPool: private boost::thread_group {
 public:
  /**
   * @brief Constructor.
   *
   * @param ios The reference to the global `io_service` instance.
   * @param thread_num The number of threads created in the pool. The default
   *        value is 0, indicating the number is equal to the hardware
   *        concurrency.
   */
  ThreadPool(boost::asio::io_context& ios,
             size_t thread_num = 0);

  /**
   * @brief Destructor.
   */
  ~ThreadPool();

  /**
   * @brief Spawn and run the worker threads.
   *
   * If the hardware concurrency is less than the `MIN_THREAD_NUM`, it will
   * be overriden by the macro setting.
   */
  void run();

  /**
   * @brief Force to interrupt all threads and call `join()`.
   */
  void stop();

  /**
   * @brief Block the calling thread, while letting the workers run. Threads
   * will not be joined until error occurs.
   */
  void waitUntilErrorDetected();

  void cancelAfterSeconds(int seconds);

 private:
  /**
   * @brief The function to explicitly call `io_service.run()`.
   *
   * The function ran on each thread. It makes each thread a worker of
   * `io_service`.
   */
  void threadAttachToIOServiceSafely();

 private:
  /**
   * @brief Reference to the `io_service` object.
   */
  boost::asio::io_context& ios_ref_;

  /**
   * @brief The number of threads in the thread pool.
   *
   * The number is a const value which is set in `run()`.
   */
  size_t thread_num_;

  /**
   * @brief Some work to guarantee thread will not exit.
   */
  boost::asio::executor_work_guard<
      boost::asio::io_context::executor_type> work_;
};

}

#endif
