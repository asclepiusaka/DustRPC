// Synchronized task queue using mutex.
#pragma once
#include <mutex>
#include <queue>

template <typename T>
class SyncQueue {
public:
	SyncQueue();
	~SyncQUeue();
	bool empty();
	int size();
	void enqueue(T& t);
	bool dequeue();
private:
	std::queue<T> queue;
	std::mutex mutex;
};

SyncQueue::empty() {
	//unique_lock will be released automatically when out of scope.
	std::unique_lock<std::mutex> lock(mutex);
	return queue.empty();
}
SyncQueue::size() {
	std::unique_lock,std::mutex> lock(mutex);
	return queue.size();
}
SyncQueue::enqueue(T& t) {
	std::unique_lock<std::mutex> lock(mutex);
	queue.push(t);

}
SyncQueue::dequeue(T& t) {
	std::unique_lock<std::mutex> lock(mutex);
	if (queue.empty()) {
		return false;
	}
	t = std::move(queue.front());
	queue.pop();
	return true;
}