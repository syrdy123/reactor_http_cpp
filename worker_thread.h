#pragma once

#include <thread>
#include <condition_variable>

#include "event_loop.h"

//���̵߳Ľṹ��
class WorkerThread {
public:
	WorkerThread(int index);
	~WorkerThread();

	inline EventLoop* get_event_loop() {
		return m_event_loop;
	}

	//���̵߳Ĺ�������
	void sub_thread_work();

	//����
	void run();

private:
	std::thread::id m_thread_id;
	std::thread* m_thread;
	std::string m_thread_name;
	std::mutex m_mutex; //������
	std::condition_variable m_condition; //��������

	EventLoop* m_event_loop; //��Ӧ��ģ��

};

