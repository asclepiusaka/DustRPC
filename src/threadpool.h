#pragma once
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>
#include "SyncQueue.h"


class threadpool {
public:
	threadpool(size_t);
	templates<typename F, typename... Args>
	auto enqueue(F&& f, Args&&... args) -> std::future<decltype(f(args...))>;
	~threadpool();
private:
	// keep track of spawned threads.
	std::vector<std::thread> workers;

	// task queue
	SyncQueue<std::function<void()>> tasks;

	bool shutdown;
	std::mutex queue_mutex;
	std::condition_variable conditional_lock;
};

threadpool::threadpool(size_t threads) : shutdown(false) {
	for (size_t i = 0; i < threads; ++i) {
		workers.emplace_back(
			[this]
			{
				std::function<void()> task;
				bool dequeued;

				while(!this->shutdown) {
					{
						std::unique_lock<std::mutex> lock(this->queue_mutex);
						if (this->tasks.empty()) {
							this->conditional_lock.wait(lock);
						}
						dequeued = this->tasks.dequeue(task);
					}
					if (dequeued) {
						task();
					}
				}
			}
		);
	}
}


template<typename F, typename... Args>
auto threadpool::enqueue(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
    std::function<decltype(f(args...))()> task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(task);
    std::function<void()> wrapper_task = [task_ptr]() {
      (*task_ptr)(); 
    };
    tasks_enqueue(wrapper_task);
    conditional_lock.notify_one();
    return task_ptr->get_future();
}

threadpool::~threadpool() {
	{
		{
			std::unique_lock<std::mutex> lock(queue_mutex);
			shutdown = true;
		}
		conditional_lock.notify_all();
		for (int i = 0; i < workers.size(); i++) {
			workers[i].join();
		}
	}
}
