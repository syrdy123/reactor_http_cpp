#include "thread_pool.h"

void ThreadPool::run()
{
	assert(!m_is_start);

	if (m_main_loop->get_thread_id() != std::this_thread::get_id()) {
		exit(0);
	}

	m_is_start = true;

	if (m_thread_num > 0) {
		for (int i = 0; i < m_thread_num; ++i) {
			WorkerThread* sub_thread = new WorkerThread(i);
			sub_thread->run();
			m_worker_threads.push_back(sub_thread);
		}
	}

}

EventLoop* ThreadPool::take_worker_event_loop()
{
	assert(m_is_start);

	//必须是由主线程 来取出一个子线程的 反应堆实例
	if (m_main_loop->get_thread_id() != std::this_thread::get_id()) {
		exit(0);
	}

	//从线程池中找到一个子线程，取出它的反应堆实例
	EventLoop* event_loop = m_main_loop;

	if (m_thread_num) {
		event_loop = m_worker_threads[m_index]->get_event_loop();
		m_index = (m_index + 1) % m_thread_num;
	}

	return event_loop;
}

ThreadPool::ThreadPool(EventLoop* event_loop, int count)
{
	m_index = 0;
	m_is_start = false;
	m_main_loop = event_loop;
	m_thread_num = count;
	m_worker_threads.clear();
}

ThreadPool::~ThreadPool()
{
	for (auto e : m_worker_threads) {
		delete e;
	}
}
