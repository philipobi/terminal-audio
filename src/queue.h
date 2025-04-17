#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>

template<class T>
class Queue
{
    std::queue<T> q;
    mutable std::mutex m;
    std::condition_variable c;

public:
    Queue();
    void push(T t);
    T pop();
};

#include "queue.tpp"