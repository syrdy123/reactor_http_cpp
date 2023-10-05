#include "tcp_connection.h"





TcpConnection::TcpConnection(int fd, EventLoop* event_loop)
{
	m_event_loop = event_loop;


	m_read_buffer = new Buffer(10240);
	m_write_buffer = new Buffer(10240);
	//http
	m_request = new HttpRequest;
	m_response = new HttpResponse;

	m_name = "Connection-" + to_string(fd);

	m_channel = new Channel (fd, FDEvent::READ_EVENT, process_read, process_write, destroy, this);
	m_event_loop->add_task(m_channel, ElemType::ADD);
}

TcpConnection::~TcpConnection()
{
	if (m_read_buffer && m_read_buffer->readable_size() == 0 &&
		m_write_buffer && m_write_buffer->readable_size() == 0) {

		m_event_loop->channel_destroy(m_channel);

		delete m_read_buffer;
		delete m_write_buffer;

		delete m_request;
		delete m_response;
	}

	Debug("Disconnect,free source , connection-name : %s", m_name.data());
}

int TcpConnection::process_read(void* arg) {
	TcpConnection* conn = static_cast<TcpConnection*>(arg);
	
	int socket = conn->m_channel->get_socket();
	//读数据
	int count = conn->m_read_buffer->socket_read(socket);

	Debug("The received http request data : %s", conn->m_read_buffer->data());

	if (count > 0) {
		//解析 http 请求

#ifdef MSG_SEND_AUTO
		//激活写事件
		conn->m_channel->write_event_enable(true);

		//修改反应堆模型中该节点对应的事件
		conn->m_event_loop->add_task(conn->m_channel, ElemType::MODIFY);
#endif 

		bool flag = conn->m_request->parse_http_request(conn->m_read_buffer, conn->m_response, conn->m_write_buffer, socket);
		if (!flag) {
			//解析失败 , 回复一个简单的html
			string error_msg = "Http/1.1 400 Bad Request\r\n\r\n";
			conn->m_write_buffer->append_string(error_msg);

		}
	}
	//发送数据为空,就断开连接
	else {
#ifdef MSG_SEND_AUTO
		//断开连接 从反应堆模型中删除对应的文件描述符
		conn->m_event_loop->add_task(conn->m_channel, ElemType::DELETE);

#endif
	}

#ifndef MSG_SEND_AUTO
	//断开连接 从反应堆模型中删除对应的文件描述符
	conn->m_event_loop->add_task(conn->m_channel, ElemType::DELETE);

#endif

	return 0;
}

int TcpConnection::process_write(void* arg) {

	TcpConnection* conn = static_cast<TcpConnection*>(arg);
	int socket = conn->m_channel->get_socket();

	int count = conn->m_write_buffer->send_data(socket);

	if (count) {
		//write_buffer 数据已经发完了
		if (conn->m_write_buffer->readable_size() == 0) {
			//1.不再检测写事件了 , 修改 channel 中的事件
			conn->m_channel->write_event_enable(false);

			//2.修改反应堆模型中节点对应的事件 , 读写 ——> 读
			conn->m_event_loop->add_task(conn->m_channel, ElemType::MODIFY);

			//3.删除该节点
			conn->m_event_loop->add_task(conn->m_channel, ElemType::DELETE);
		}
	}
	return 0;
}

int TcpConnection::destroy(void* arg)
{
	TcpConnection* conn = static_cast<TcpConnection*>(arg);

	if (conn != nullptr) {
		delete conn;
	}

	return 0;
}
