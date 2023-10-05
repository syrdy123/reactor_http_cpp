#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>

#include "event_loop.h"
#include "thread_pool.h"
#include "tcp_connection.h"
#include "log.h"


class TcpServer {
public:
	//��ʼ��
	TcpServer(unsigned short port, int thread_count);
	~TcpServer();

	//��ʼ������
	void set_listen();

	//���� tcpserver
	void run();

private:
	static int accept_connection(void*);

private:
	EventLoop* m_main_loop;
	ThreadPool* m_thread_pool;

	int m_thread_count;
	int m_lfd;
	unsigned short m_port;
};

