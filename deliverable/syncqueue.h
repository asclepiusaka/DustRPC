// Synchronized task queue using mutex.
#pragma once
#include <mutex>
#include <queue>

template <typename T>
class SyncQueue {
public:
    SyncQueue() {};
    SyncQueue(SyncQueue& other) {};
    ~SyncQueue() {};

    bool empty();
    int size();
    void enqueue(T& t);
    bool dequeue(T& t);
private:
    std::queue<T> queue;
    std::mutex mutex;
};

template <typename T>
bool SyncQueue<T>::empty() {
    //unique_lock will be released automatically when out of scope.
    std::unique_lock<std::mutex> lock(mutex);
    return queue.empty();
}

template <typename T>
int SyncQueue<T>::size() {
    std::unique_lock<std::mutex> lock(mutex);
    return queue.size();
}

template <typename T>
void SyncQueue<T>::enqueue(T& t) {
    std::unique_lock<std::mutex> lock(mutex);
    queue.push(t);

}

template <typename T>
bool SyncQueue<T>::dequeue(T& t) {
    std::unique_lock<std::mutex> lock(mutex);
    if (queue.empty()) {
        return false;
    }
    t = std::move(queue.front());
    queue.pop();
    return true;
}