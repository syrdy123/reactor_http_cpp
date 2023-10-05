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

	//event_loop ����ʼ�����֮��,��Ҫ�������������߳�,�����̼߳�������ִ��
	m_condition.notify_one();

	m_event_loop->run();
}

void WorkerThread::run()
{


	//�������߳�
	m_thread = new std::thread(&WorkerThread::sub_thread_work, this);

	//�������߳� , �ȵ����̰߳� event_loop ��ʼ����� , �ٻ�ȡ�����¼���ִ��

	std::unique_lock<std::mutex> locker(m_mutex);


	while (m_event_loop == NULL) {
		m_condition.wait(locker);
	}

}
