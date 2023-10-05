#include "worker_thread.h"


WorkerThread::WorkerThread(int index)
{
	m_event_loop = nullptr;
	m_thread = nullptr;
	m_thread_id = std::thread::id();
	m_thread_name = "sub_thread-" + std::to_string(index);
}

WorkerThread::~WorkerThread()
{
	if (m_thread != nullptr) {
		delete m_thread;
	}
}

void WorkerThread::sub_thread_work()
{

	m_mutex.lock();

	m_event_loop = new EventLoop(m_thread_name);

	m_mutex.unlock();

	//event_loop 被初始化完毕之后,就要唤醒阻塞的主线程,让主线程继续向下执行
	m_condition.notify_one();

	m_event_loop->run();
}

void WorkerThread::run()
{


	//创建子线程
	m_thread = new std::thread(&WorkerThread::sub_thread_work, this);

	//阻塞主线程 , 等到子线程把 event_loop 初始化完毕 , 再获取锁向下继续执行

	std::unique_lock<std::mutex> locker(m_mutex);


	while (m_event_loop == NULL) {
		m_condition.wait(locker);
	}

}
