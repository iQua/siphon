/**
 * @file NotificationCenter.hpp
 * @brief The entity that allows notification broadcast in Siphon. Different
 *        Notification message types are also defined.
 *
 * A NotificationCenter will create the connections between all notification
 * emitters and their corresponding notification handlers. Each notification has
 * a specific notification message type, also defined in this file. The
 * NotificationCenter is constructed with a list of Observers. Each observer is
 * set to process a specific type of notification message. Then, the
 * Notification objects can register themselves to the NotificationCenter,
 * getting connected to the corresponding Observer (a N-to-1 mapping).
 *
 * @author Shuhao Liu <shuhao@ece.toronto.edu>
 *
 * @ingroup notification
 * @date 2015-11-05
 */

#ifndef NOTIFICATION_CENTER_H_
#define NOTIFICATION_CENTER_H_

#include <vector>

#include "controller/notification_observers.hpp"


namespace siphon {

/**
 * @brief NotificationCenter class is a template class. It is extensible with
 *        any number of notification message types.
 *
 * @tparam NotificationMsgTypes A series of notification message types, each of
 *         which corresponds to an arbitrary notification observer (handler).
 *
 * @remark Everything is initialized during construction. No write after it is
 *         initialized, so it is thread-safe.
 */
template <typename... NotificationMsgTypes>
class NotificationCenter {
 public:
   /**
    * @brief Construct the NotificationCenter with a list of pointers to the
    *        Observers, each of which is responsible for handling a type of
    *        notifications (with a type of notification message).
    *
    * @param observers The list of pointers to the Observers. The listing
    *        order should be the same as the list of type names in the template
    *        arguement.
    */
  NotificationCenter(IObserver<NotificationMsgTypes>*... observers) :
      observer_map_(boost::fusion::pair<NotificationMsgTypes,
                    IObserver<NotificationMsgTypes>*>(observers)...) {
  }

  /**
   * @brief Destructor. Nothing to clean, because it holds no status.
   */
  ~NotificationCenter() = default;

  /**
   * @brief Connecting a Notification to its corresponding Observer.
   *
   * @tparam T The type of the actual Notification. It might be a multiple
   *         inheritance of several Notifications.
   * @tparam MsgType/MsgTypes The message types that can be emitted by T.
   * @param emitter The pointer to the Notification to be registered.
   *
   * @remark T is allowed to be a multiple inheritance of multiple Notification
   *         types. With this function, all corresponding handlers can be
   *         registered to T, as long as *ALL* message types are *explicitly*
   *         given.
   */
  template <typename T, typename MsgType, typename... MsgTypes>
  void registerNotification(T* emitter) const {
    static_assert(std::is_base_of<INotification<MsgType>, T>::value,
                  "Notification type does not match.");
    // Find the corresponding observer with the notification message type.
    IObserver<MsgType>* corr_observer
        = boost::fusion::at_key<MsgType>(observer_map_);
    // Register the notification handler to the emitter.
    emitter->INotification<MsgType>::registerNotificationHandler(
        corr_observer->getNotificationHandler());
    registerNotification<T, MsgTypes...>(emitter);
  }

  /**
   * @brief Register a NotificationGroup to the NotificationCenter. Handlers
   *        required will be passed to the NotificationGroup.
   *
   * @param group The pointer to the NotificationGroup instance.
   *
   * @remark All template types will be inferred by the compiler.
   */
  template <typename KeyType, typename T, typename MsgType,
            typename... MsgTypes>
  void registerNotificationGroup(
      NotificationGroup<KeyType, T, MsgType, MsgTypes...>* group) const {
    // Find the corresponding observer with the notification message type.
    // Register the notification handler to the emitter.
    group->registerNotificationHandlers(
        (boost::fusion::at_key<MsgType>(observer_map_))
            ->getNotificationHandler(),
        (boost::fusion::at_key<MsgTypes>(observer_map_))
            ->getNotificationHandler()...);
  }

 protected:
  /**
   * @brief A helper function to help `registerNotification()` function finish
   *        properly.
   */
  template <typename T>
  void registerNotification(T* emitter) const {
  }

 protected:
  /**
   * @brief A boost Fusion map which holds the mapping relationship between the
   *        message type to the corresponding Observer.
   *
   * Boost Fusion map is a heterogeneous data structure. The key of each mapping
   * pair is type-only, while the value is a pointer to the corresponding
   * Observer.
   *
   */
  boost::fusion::map<boost::fusion::pair<NotificationMsgTypes,
      IObserver<NotificationMsgTypes>*>...> observer_map_;

};


}


#endif
