/**
 * @file AtomicQueues.hpp
 * @brief Define several lock-free queue data structure.
 *
 * @author Shuhao Liu <shuhao@ece.toronto.edu>
 *
 * @date 2016-01-18
 * @ingroup fundamentals
 */

#ifndef SIPHON_MPSC_QUEUE_
#define SIPHON_MPSC_QUEUE_

#include <boost/atomic.hpp>
#include <boost/function.hpp>


namespace siphon {

/**
 * @brief A Lock-free queue. It allows multiple concurrent producers and
 *        a single consumer. Such queue can work as a multiplexer, which is used
 *        to serialize messages.
 *
 * @tparam T The data type to be contained in the queue.
 */
template<typename T>
class MPSCQueue {
 public:
  /**
   * @brief The underlying implementation of the queue is a singly-linked list.
   *        Define the node in the list.
   */
  struct Node {
    T data; /* The data itself */
    Node* next; /* Pointer to the next node */
  };

  /**
   * @brief Constructor.
   */
  MPSCQueue() : head_(0) {
  }

  /**
   * @brief Push new element to the queue. The operation is lock-free and
   *        wait-free, and multiple threads can push concurrently.
   *
   * @param data The data to be enqueued.
   */
  void push(const T& data) {
    Node* n = new Node;
    n->data = data;
    Node* stale_head = head_.load(boost::memory_order_relaxed);
    do {
      n->next = stale_head;
    } while (!head_.compare_exchange_weak(stale_head, n,
                                          boost::memory_order_release));
  }

  /**
   * @brief Pop all nodes in the queue as a linked list. Empty the queue.
   *
   * @return The pointer to the first node.
   */
  Node* popAll() {
    Node* last = popAllReverse();
    Node* first = 0;
    while (last) {
      Node* tmp = last;
      last = last->next;
      tmp->next = first;
      first = tmp;
    }
    return first;
  }

  /**
   * @brief Pop all nodes in the queue as a linked list, in the reverse order.
   *        Empty the queue afterwards.
   * @return The pointer to the first node.
   *
   * @remark Only one thread can take this operation at one time.
   */
  Node* popAllReverse() {
#ifdef __APPLE__
    return head_.exchange(0, boost::memory_order_consume);
#else
    return head_.exchange(0);
#endif
  }

  /**
   * @brief Pop all nodes in the queue in the enqueued order. Empty the queue
   *        afterwards. Then process each element with the given function.
   *
   * @param func The function to process each element.
   */
  void processAll(const boost::function<void (T)>& func) {
    Node* x = popAll();
    while (x) {
      func(x->data);
      Node* temp = x;
      x = x->next;
      delete temp;
    }
  }

  /**
   * @brief Pop all nodes in the queue in the reverse order. Empty the queue
   *        afterwards. Then process each element with the given function.
   *
   * @param func The function to process each element.
   */
  void processAllReverse(const boost::function<void (T)>& func) {
    Node* x = popAllReverse();
    while (x) {
      func(x->data);
      Node* temp = x;
      x = x->next;
      delete temp;
    }
  }

private:
  /**
   * @brief The atomic pointer to the head of the queue.
   */
  boost::atomic<Node*> head_;
};

};


#endif
