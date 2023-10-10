#include "tcp_server.h"



int TcpServer::accept_connection(void* arg) {
	TcpServer* server = static_cast<TcpServer*>(arg);
	int cfd = accept(server->m_lfd, NULL, NULL);

	//���̳߳���ȡ��һ�����̵߳ķ�Ӧ��ʵ�����������������ͨ�ŵ� cfd
	EventLoop* event_loop = server->m_thread_pool->take_worker_event_loop();

	new TcpConnection(cfd,event_loop);

	return 0;
}


TcpServer::TcpServer(unsigned short port, int thread_count):m_port(port),m_thread_count(thread_count)
{
	set_listen();

	m_main_loop = new EventLoop;
	m_thread_pool = new ThreadPool(m_main_loop,m_thread_count);
}

TcpServer::~TcpServer()
{
}

void TcpServer::set_listen()
{
	//���ڼ������ļ�������
	m_lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_lfd == -1) {
		perror("socket");
		return;
	}

	//���ö˿ڸ���
	int opt = 1;
	int ret = setsockopt(m_lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
	if (ret == -1) {
		perror("setsocket");
		return;
	}

	//��
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(m_port);
	addr.sin_addr.s_addr = INADDR_ANY;
	ret = bind(m_lfd, (struct sockaddr*)(&addr), sizeof addr);

	if (ret == -1) {
		perror("bind");
		return;
	}

	ret = listen(m_lfd, 128);
	if (ret == -1) {
		perror("listen");
		return;
	}
}

void TcpServer::run()
{
	//�����̳߳�
	m_thread_pool->run();

	//��Ӽ���¼�
	Channel* channel = new Channel(m_lfd, FDEvent::READ_EVENT, accept_connection, nullptr, nullptr, this);
	m_main_loop->add_task(channel, ElemType::ADD);

	//������Ӧ��ģ��
	m_main_loop->run();
}
