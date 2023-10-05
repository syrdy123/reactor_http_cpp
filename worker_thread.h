#pragma once

#include <thread>
#include <condition_variable>

#include "event_loop.h"

//子线程的结构体
class WorkerThread {
public:
	WorkerThread(int index);
	~WorkerThread();

	inline EventLoop* get_event_loop() {
		return m_event_loop;
	}

	//子线程的工作函数
	void sub_thread_work();

	//启动
	void run();

private:
	std::thread::id m_thread_id;
	std::thread* m_thread;
	std::string m_thread_name;
	std::mutex m_mutex; //互斥锁
	std::condition_variable m_condition; //条件变量

	EventLoop* m_event_loop; //反应堆模型

};

