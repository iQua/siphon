/**
 * @file NotificationObservers.hpp
 * @brief Define three types of base classes:
 *        * INotification defines the interfaces of a notification poster.
 *        * IObserver is the object that provides a proper notification handler.
 *        * ISerializedObserver is a variation of IObserver which automatically
 *                              serialize the notification handling.
 *
 * @author Shuhao Liu <shuhao@ece.toronto.edu>
 *
 * @ingroup notification
 * @date 2015-11-05
 */

#ifndef NOTIFICATION_OBSERVERS_HPP_
#define NOTIFICATION_OBSERVERS_HPP_

#include <string>
#include <typeinfo>
#include <type_traits>
#include <utility>
#include <shared_mutex>
#include <mutex>

#include <boost/signals2.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/unordered_map.hpp>
#include <boost/noncopyable.hpp>
#include <boost/fusion/container.hpp>
#include <boost/fusion/include/container.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/fusion/include/sequence.hpp>

#include "controller/notification_types.hpp"
#include "util/logger.hpp"


namespace siphon {

using namespace std;

/**
 * @brief The interface class for Notifications. All classes derived from this
 *        class has the ability to emit notifications.
 *
 * @tparam NotificationMsgType The type of notification messages emitted by the
 *         Notification.
 * @remark Typically a Notification should not work on multiple threads. That
 *         is, several Notification instances should be created if multiple
 *         threads are appropriate. As a result, the interface implementation
 *         cares nothing about thread safety.
 */
template <typename NotificationMsgType>
class INotification {
 public:
  /**
   * @brief The smart pointer type of the notification message.
   */
  typedef std::shared_ptr<NotificationMsgType> NotificationMsgPtrType;

 public:
  /**
   * @brief Constructor.
   */
  INotification() = default;

  /**
   * @brief Destructor. Disconnect the connection before freeing the memory.
   */
  virtual ~INotification() {
    if (hasActiveObserver()) observer_conn_.disconnect();
    LOG_WARNING << "Notification<" << typeid(NotificationMsgType).name()
        << "> has been destroyed.";
  }

  /**
   * @brief Utility function to check whether the notification has an active
   *        Observer.
   *
   * @return A bool value indicating whether there is an active observer
   *         registered.
   */
  bool hasActiveObserver() const {
    return observer_conn_.connected();
  }

  /**
   * @brief Register an Observer to the Notification, by connecting its `signal`
   *        to the provided handler.
   *
   * @param func The notification handler, which is a `boost::function`.
   * @return Always true.
   */
  virtual bool registerNotificationHandler(
      boost::function<void (NotificationMsgPtrType)> func) {
    observer_conn_ = observer_signal_.connect(func);
    return true;
  }

  /**
   * @brief Notify the observers with a notification message.
   *
   * @param notification_msg_ptr The smart pointer to the emitted notification
   *                             message.
   */
  void notify(NotificationMsgPtrType notification_msg_ptr) {
    if (!hasActiveObserver()) {
      LOG_ERROR << "Notification<" << typeid(NotificationMsgType).name()
          << "> tried to emit, but hasn't registered yet!";
    }
    observer_signal_(notification_msg_ptr);
  }

 protected:
  /**
   * @brief The `boost::signals2::signal` instance, used to notify the observer.
   */
  boost::signals2::signal<void (NotificationMsgPtrType)> observer_signal_;

  /**
   * @brief The `connection` instance preserving the status the the `signal`.
   */
  boost::signals2::connection observer_conn_;
};

/**
 * @brief A class that manages a group of Notifications. An instance of
 *        NotificationGroup are organized in a hash map. Notification instances
 *        are created and deleted with NotificationGroup instances.
 *
 * @tparam KeyType The type of the unique key to reference the Notifications.
 * @tparam T The type of the notification instance. It must be a derived class
 *         of `INotification<NotificationMsgTypes...>`.
 * @tparam NotificationMsgType/NotificationMsgTypes The type of the notification
 *         message to be emitted.
 *
 * @remark Since Notification instances are likely to be held in different
 *         threads, thread-safety should be a concern while reading/writing the
 *         hash map. In this case, we use a SharedSpinLock to protect these
 *         operations. Concurrent reads are allowed, but reading operations have
 *         exclusive use of the hash map container.
 */
template <typename KeyType, typename T, typename NotificationMsgType,
          typename... NotificationMsgTypes>
class NotificationGroup :
    public boost::unordered_map<KeyType, std::shared_ptr<T>>,
    public boost::noncopyable {
 public:
   /**
    * @brief The underlying hash map type.
    */
  typedef boost::unordered_map<KeyType, std::shared_ptr<T>> BaseMapType;

 protected:
  /**
   * @brief A helper type to perform trait check.
   */
  template <bool...> struct bool_pack;

  /**
   * @brief A helper type to perform trait check.
   */
  template <bool... v>
  using all_true = std::is_same<bool_pack<true, v...>, bool_pack<v..., true>>;

 public:
  /**
   * @brief Constructor. Check whether T is a derived class of INotification at
   *        compile time.
   */
  NotificationGroup() : has_handler_(false) {
    static_assert(all_true<
        std::is_base_of<INotification<NotificationMsgType>, T>{},
        std::is_base_of<INotification<NotificationMsgTypes>, T>{}...>{},
        "NotificationGroup must be created out of some INotification types!");
  }

  /**
   * @brief Destructor. Since the NotificationGroup owns all managed
   *        Notification instances, it will delete all instances.
   */
  ~NotificationGroup() {
    std::unique_lock<std::shared_mutex> guard(mutex_);
    for (typename BaseMapType::iterator i = BaseMapType::begin();
         i!= BaseMapType::end(); ++i) {
      i->second.reset();
    }
  }

  /**
   * @brief Register all required notification handlers to the group.
   *
   * @param f/fs A list of notification handlers. The order of the handlers must
   *        be the same as NotificationMsgType/NotificationMsgTypes.
   *
   * @return Always true.
   */
  virtual bool registerNotificationHandlers(
      boost::function<void (std::shared_ptr<NotificationMsgType>)> f,
      boost::function<void (std::shared_ptr<NotificationMsgTypes>)>... fs) {
    std::vector<bool> temp {registerNotificationHandler(f),
                            registerNotificationHandler(fs)...};
    has_handler_ = true;
    return true;
  }

  /**
   * @brief Find the Notification with the given key in the group. If it does
   *        not exist yet, create it with the given parameters.
   *
   * @tparam Params A list of parameters types. Be consistent with one of the
   *         constructor of `T`.
   * @param key The key to reference the newly created Notification instance.
   * @param vars A list of parameters used to instantiate the Notification.
   * @return The pointer to the found or newly created notification instance.
   *
   * @remark Prerequisite: The NotificationGroup has been registered to the
   *         NotificationCenter, such that handlers have been passed to the
   *         NotificationGroup.
   */
  template <typename... Params>
  std::shared_ptr<T> getNotificationInGroup(KeyType& key, Params&... vars) {
    assert(has_handler_);
    typename BaseMapType::const_iterator itr;
    std::shared_ptr<T> new_notification;
    {
      std::unique_lock<std::shared_mutex> guard(mutex_);
      itr = this->find(key);
      if (itr == this->end()) {
        new_notification = std::make_shared<T>(vars...);
        (*this)[key] = new_notification;
      }
      else return itr->second;
    }
    // Register all required handlers
    std::vector<bool> temp {
      new_notification->
          INotification<NotificationMsgType>::registerNotificationHandler(
              boost::fusion::at_key<NotificationMsgType>(handler_map_)),
      new_notification->
          INotification<NotificationMsgTypes>::registerNotificationHandler(
              boost::fusion::at_key<NotificationMsgTypes>(handler_map_))...
    };
    return new_notification;
  }

  /**
   * @brief Create a new Notification instance with the parameters provided.
   *        Then add it to the hash map, referenced by the key. Then, register
   *        all required handlers to it.
   *
   * @tparam Params A list of parameters types. Be consistent with one of the
   *         constructor of `T`.
   * @param key The key to reference the newly created Notification instance.
   * @param vars A list of parameters used to instantiate the Notification.
   * @return The pointer to the newly created notification instance.
   *
   * @remark Prerequisite: The NotificationGroup has been registered to the
   *         NotificationCenter, such that handlers have been passed to the
   *         NotificationGroup.
   */
  template <typename... Params>
  std::shared_ptr<T> createNewNotification(KeyType& key, Params&... vars) {
    assert(has_handler_);
    std::shared_ptr<T> new_notification = std::make_shared<T>(vars...);
    {
      std::unique_lock<std::shared_mutex> guard(mutex_);  // Protected write
      (*this)[key] = new_notification;
    }
    // Register all required handlers
    std::vector<bool> temp {
      new_notification->
          INotification<NotificationMsgType>::registerNotificationHandler(
              boost::fusion::at_key<NotificationMsgType>(handler_map_)),
      new_notification->
          INotification<NotificationMsgTypes>::registerNotificationHandler(
              boost::fusion::at_key<NotificationMsgTypes>(handler_map_))...
    };
    return new_notification;
  }

  /**
   * @brief Add an existing Notification instance to the NotificationGroup.
   *        Then add it to the hash map, referenced by the key. Then, register
   *        all required handlers to it.
   *
   * @param key The key to reference the newly created Notification instance.
   * @param new_notification The shared pointer to the existing Notification.
   * @return The pointer to the newly created notification instance.
   *
   * @remark Prerequisite: The NotificationGroup has been registered to the
   *         NotificationCenter, such that handlers have been passed to the
   *         NotificationGroup.
   */
  std::shared_ptr<T> addNewNotification(KeyType& key,
      std::shared_ptr<T> new_notification) {
    assert(has_handler_);
    {
      std::unique_lock<std::shared_mutex> guard(mutex_);  // Protected write
      (*this)[key] = new_notification;
    }
    // Register all required handlers
    std::vector<bool> temp {
      new_notification->
          INotification<NotificationMsgType>::registerNotificationHandler(
              boost::fusion::at_key<NotificationMsgType>(handler_map_)),
      new_notification->
          INotification<NotificationMsgTypes>::registerNotificationHandler(
              boost::fusion::at_key<NotificationMsgTypes>(handler_map_))...
    };
    return new_notification;
  }

  /**
   * @brief Look up the map and find the Notification instance with the given
   *        key.
   *
   * @param key The key to reference the desired Notification instance.
   *
   * @return The pointer to the desired instance. Return a null pointer if not
   *         found.
   */
  std::shared_ptr<T> findNotificationInGroup(const KeyType& key) {
    typename BaseMapType::const_iterator itr;
    {
      std::shared_lock<std::shared_mutex> guard(mutex_);
      itr = this->find(key); // Protect the read, allowing shared ownership
    }
    if (itr == this->end()) return nullptr;
    else return itr->second;
  }

  /**
   * @brief Delete the Notification instance with the given key.
   *
   * @param key The key to reference the Notification instance to be deleted.
   *
   * @return true if the desired element is successfully released and erased.
   *         false if the desired element is not found.
   */
  bool deleteNotificationInGroup(const KeyType& key) {
    typename BaseMapType::iterator itr;
    std::unique_lock<std::shared_mutex> guard(mutex_);
    itr = this->find(key);
    if (itr == this->end()) return false;
    else {
      itr->second.reset();
      this->erase(itr);
      return true;
    }
  }

 protected:
  /**
   * @brief Register an Observer handler to the Notification group.
   *
   * @param f The notification handler, which is a `boost::function`.
   */
  template <typename CurrentMsgType>
  bool registerNotificationHandler(
      boost::function<void (std::shared_ptr<CurrentMsgType>)> f) {
    static_assert(std::is_base_of<INotification<CurrentMsgType>, T>::value,
                 "A matching handler should be provided.");
    boost::fusion::at_key<CurrentMsgType>(handler_map_) = f;
    return true;
  }

 protected:
  /**
   * @brief The map of `boost::function` type to store the handlers for all
   *        notifications, referenced by their NotificationMsgType.
   */
  boost::fusion::map<
      boost::fusion::pair<NotificationMsgType,
          boost::function<void (std::shared_ptr<NotificationMsgType>)>>,
      boost::fusion::pair<NotificationMsgTypes,
          boost::function<void (std::shared_ptr<NotificationMsgTypes>)>>...>
      handler_map_;

  /**
   * @brief A flag indicating whether there is a handler being registered.
   */
  bool has_handler_;

  /**
   * @brief The SharedSpinLock object to protect read to and writes from the
   *        hash map.
   */
  mutable std::shared_mutex mutex_;
};


/**
 * @brief The interface class for Observers that DO NOT require notifications to
 *        be processed serially. All classes derived from this class has the
 *        ability to process notifications ***asynchronously***.
 *
 * @tparam NotificationMsgType The type of notification messages processed by
 *                             the Observer.
 * @remark This Observer interface does not guarantee thread safety.
 */
template <typename NotificationMsgType>
class IObserver {
 public:
  /**
   * @brief The smart pointer type of the notification message.
   */
  typedef std::shared_ptr<NotificationMsgType> NotificationMsgPtrType;

 public:
  /**
   * @brief Constructor.
   *
   * @param ios A reference to the global `boost::asio::io_context` objected.
   */
  IObserver(boost::asio::io_context& ios) : ios_(ios) {
  }

  /**
   * @brief Destructor.
   */
  virtual ~IObserver() {
  }

  /**
   * @brief Get the notification handler, usually called by the notification
   *        manager.
   *
   * @return The `boost::function` that has the correct handler signature.
   */
  boost::function<void (NotificationMsgPtrType)> getNotificationHandler() {
    return boost::bind(
        &IObserver<NotificationMsgType>::wrappedHandler, this, _1);
  }

 protected:
  /**
   * @brief The real notification handler to be implemented. Here we define how
   *        to deal with the notification messages. It should be thread-safe.
   *
   * @param notification_msg_ptr The smart pointer to the emitted notification
   *        message.
   */
  virtual void handleNotification(NotificationMsgPtrType notification_msg_ptr)
      = 0;

  /**
   * @brief A psudo notification handler, only to be overriden by
   *        ISerializedObserver. The reason of using this function is that it
   *        wraps the real notification handler to the `io_service` object, so
   *        that the notification handlers can be executed asynchronously.
   *
   * @param notification_msg_ptr
   */
  virtual void wrappedHandler(NotificationMsgPtrType notification_msg_ptr) {
    ios_.post(boost::bind(
              &IObserver<NotificationMsgType>::handleNotification,
              this,
              notification_msg_ptr));
  }

 protected:
  /**
   * @brief The io_service reference.
   */
  boost::asio::io_context& ios_;
};


/**
 * @brief The interface class for Observers that require notifications to be
 *        processed serially. All classes derived from this class has the
 *        ability to process notifications ***asynchronously*** and
 *        ***serially***.
 *
 * @tparam NotificationMsgType The type of notification messages processed by
 *         the Observer.
 * @remark Observer usually takes notifications emitted by several Notifications
 *         on several threads, so the handler might require thread safety. For
 *         this reason, we implement another varation where each Observer owns
 *         a `asio::strand` to serialize the notification to be processed.
 */
template <typename NotificationMsgType>
class ISerializedObserver : public IObserver<NotificationMsgType>{
 public:
  /**
   * @brief The smart pointer type of the notification message.
   */
  typedef std::shared_ptr<NotificationMsgType> NotificationMsgPtrType;

 public:
  /**
   * @brief Constructor.
   *
   * @param ios A reference to the global `boost::asio::io_context` objected,
   *            used to initialize the strand.
   */
  ISerializedObserver(boost::asio::io_context& ios) :
      IObserver<NotificationMsgType>(ios),
      handler_strand_(ios) {
  }

  /**
   * @brief Destructor.
   */
  virtual ~ISerializedObserver() {
  }

 protected:
  /**
   * @brief The real notification handler to be implemented. Here we define how
   *        to deal with the notification messages, *without* considering
   *        race-conditions because everything is executed in serial.
   *
   * @param notification_msg_ptr The smart pointer to the emitted notification
   *        message.
   */
  virtual void handleNotification(NotificationMsgPtrType notification_msg_ptr)
      = 0;

  /**
   * @brief A psudo notification handler, which will not be overriden. The
   *        reason of using this function is that it wraps the real notification
   *        handler to the strand, so that the notification handlers can be
   *        executed in serial.
   *
   * @param notification_msg_ptr
   */
  virtual void wrappedHandler(NotificationMsgPtrType notification_msg_ptr) {
    handler_strand_.post(boost::bind(
            &ISerializedObserver<NotificationMsgType>::handleNotification,
            this,
            notification_msg_ptr));
  }

 protected:
  /**
   * @brief The strand instance.
   */
  boost::asio::io_context::strand handler_strand_;
};


}

#endif

