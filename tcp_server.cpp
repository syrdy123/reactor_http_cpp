#include "tcp_server.h"



int TcpServer::accept_connection(void* arg) {
	TcpServer* server = static_cast<TcpServer*>(arg);
	int cfd = accept(server->m_lfd, NULL, NULL);

	//从线程池中取出一个子线程的反应堆实例，来处理这个用于通信的 cfd
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
	//用于监听的文件描述符
	m_lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_lfd == -1) {
		perror("socket");
		return;
	}

	//设置端口复用
	int opt = 1;
	int ret = setsockopt(m_lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
	if (ret == -1) {
		perror("setsocket");
		return;
	}

	//绑定
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
	//启动线程池
	m_thread_pool->run();

	//添加检测事件
	Channel* channel = new Channel(m_lfd, FDEvent::READ_EVENT, accept_connection, nullptr, nullptr, this);
	m_main_loop->add_task(channel, ElemType::ADD);

	//启动反应堆模型
	m_main_loop->run();
}
