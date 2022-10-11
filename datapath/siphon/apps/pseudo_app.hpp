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

#ifndef PSEUDO_APP_H_
#define PSEUDO_APP_H_

#include "base/minion_pool.hpp"

#include <mutex>
#include <string>

#include <boost/asio.hpp>

#include "util/config.hpp"
#include "util/types.hpp"


namespace siphon {

/**
 * @brief The minimum time interval to refill the TokenBucket.
 */
const static double kTokenBucketMinTimerInterval = 0.1;

/**
 * @brief The TokenBucket used to generate data.
 */
class TokenBucket {
 public:
  /**
   * @brief Constructor.
   *
   * @param ios Reference to the global `boost::asio::io_context` object.
   * @param average_rate The average rate to generate tokens (pkt/s).
   * @param burst_size The size of the bursts (pkt/s).
   */
  TokenBucket(boost::asio::io_context& ios,
              double average_rate = 0,
              size_t burst_size = 0);

  /**
   * @brief Destructor.
   */
  virtual ~TokenBucket() = default;

  /**
   * @brief Start to generate tokens.
   */
  void start();

  /**
   * @brief Consume one token from the TokenBucket.
   *
   * @return true If the token bucket had tokens available, and one token has
   *              been effectively consumed.
   *         false If the token bucket was empty.
   */
  bool consumeOneToken();

 protected:
  /**
   * @brief Try to generate data.
   */
  virtual void onTokenAvailable() = 0;

  /**
   * @brief Function to be called when there is new token generated.
   *
   * @param ec Possible error emitted by the deadline timer.
   */
  void generateOneToken(const boost::system::error_code& ec);

 protected:
  /**
   * @brief Reference to the global `boost::asio::io_context` object.
   */
  boost::asio::io_context& ios_;

  /**
   * @brief Average rate.
   */
  double rho_;

  /**
   * @brief Burst size.
   */
  const size_t sigma_;

  /**
   * @brief The number of tokens to be put into the bucket upon refilling.
   */
  size_t refill_amount_;

  /**
   * @brief The time interval between two actions of refilling.
   */
  boost::posix_time::time_duration refill_interval_;

  /**
   * @brief The timer used to refill the bucket.
   */
  boost::asio::deadline_timer refill_timer_;

  /**
   * @brief Recording the last time to refill the bucket.
   */
  boost::posix_time::ptime last_refill_time_;

  /**
   * @brief Depth of the bucket.
   */
  size_t bucket_depth_;

  /**
   * @brief Record the current level of the tokens in the bucket.
   */
  double bucket_level_;

  /**
   * @brief An indicator, showing whether we should explicity call send
   *        operations when the next token is generated.
   */
  std::atomic<bool> should_send_ontoken_available_;

  /**
   * @brief The mutex to protect the modifications to `bucket_level_`.
   */
  std::mutex level_mtx_;
};

/**
 * @brief A pseudo application data source. The data generation rate is
 *        controlled by the TokenBucket.
 */
class PseudoAppSource : public TokenBucket, public AbstractMessageProducer {
 public:
  /**
   * @brief Constructor.
   *
   * @param ios Reference to the global `boost::asio::io_context` object.
   * @param app_proxy Reference to the AppDataProxy object.
   * @param dsts The desired destinations of this session.
   * @param average_rate The average rate of the data source (KB/s).
   * @param burst_size The burst size of the data source (KB/s).
   */
  PseudoAppSource(boost::asio::io_context& ios,
                  MinionPool* pool,
                  IMinionStop* next_stop,
                  const PseudoSessionConfig& config);

  /**
   * @brief Destructor.
   */
  ~PseudoAppSource() override = default;

  IMinionStop* process(Minion* minion) override;

 protected:
  /**
   * @brief Try to generate data.
   */
  void onTokenAvailable() override;

 protected:
  IMinionStop* next_stop_;

  const std::string sessionID_;

  size_t seq_;

  const size_t message_size_;
};

/**
 * @brief A pseudo data receiver, which can print the data receiving rate.
 */
class PseudoAppReceiver: public AbstractMessageConsumer {
 public:
  /**
   * @brief Constructor. Upon construction, a message querying the desired
   *        sessionID will be sent to the controller.
   *
   * @param app_proxy Reference to the AppDataProxy object.
   * @param src The desired source of this session.
   * @param dsts The desired destination of this session.
   * @param protocol The protocol used for communication.
   */
  PseudoAppReceiver(MinionPool* pool,
                    const string& sessionID);

  /**
   * @brief Destructor.
   */
  ~PseudoAppReceiver() = default;

  IMinionStop* process(Minion* minion) override;

 protected:
  const std::string sessionID_;

  /**
   * @brief The counter which keeps track of the received number of KBs.
   */
  size_t counter_;

  /**
   * @brief Time of last report of data receiving rate.
   */
  boost::posix_time::ptime last_report_time_;
};


}

#endif
