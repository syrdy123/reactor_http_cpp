#pragma once

#include "event_loop.h"
#include "channel.h"
#include "buffer.h"
#include "http_request.h"
#include "http_response.h"
#include "log.h"

#define MSG_SEND_AUTO

class TcpConnection{
public:
	//≥ı ºªØ
	TcpConnection(int fd, EventLoop* event_loop);
	~TcpConnection();

	static int process_read(void* arg);
	static int process_write(void* arg);
	static int destroy(void* arg);

private:
	EventLoop* m_event_loop;
	Channel* m_channel;
	Buffer* m_read_buffer;
	Buffer* m_write_buffer;
	string m_name;

	//http
	HttpRequest* m_request;
	HttpResponse* m_response;
};



