#pragma once

#include <vector>

#include <assert.h>

#include "event_loop.h"
#include "worker_thread.h"

class ThreadPool {
public:
	ThreadPool(EventLoop* event_loop,int count);
	~ThreadPool();

	//启动
	void run();

	//取出线程池中某一个子线程的反应堆实例
	EventLoop* take_worker_event_loop();

private:
	EventLoop* m_main_loop; //主线程的反应堆模型

	bool m_is_start;
	int m_thread_num;
	std::vector<WorkerThread*> m_worker_threads;
	int m_index;

};

