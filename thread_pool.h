#pragma once

#include <vector>

#include <assert.h>

#include "event_loop.h"
#include "worker_thread.h"

class ThreadPool {
public:
	ThreadPool(EventLoop* event_loop,int count);
	~ThreadPool();

	//����
	void run();

	//ȡ���̳߳���ĳһ�����̵߳ķ�Ӧ��ʵ��
	EventLoop* take_worker_event_loop();

private:
	EventLoop* m_main_loop; //���̵߳ķ�Ӧ��ģ��

	bool m_is_start;
	int m_thread_num;
	std::vector<WorkerThread*> m_worker_threads;
	int m_index;

};

