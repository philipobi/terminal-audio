#include <queue>
#include <mutex>
#include <condition_variable>
#include "queue.h"

template<class T>
Queue<T>::Queue(): q(), m(), c() {}

template<class T>
void Queue<T>::push(T t){
    std::lock_guard<std::mutex> lck(m);
    q.push(t);
    c.notify_one();
}

template<class T>
T Queue<T>::pop(){
    std::unique_lock<std::mutex> lck(m);
    while(q.empty()) c.wait(lck);
    T item = q.front();
    q.pop();
    return item; 
}
