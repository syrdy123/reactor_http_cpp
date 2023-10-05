#pragma once
//#define _GNU_SOURCE 

#include <sys/uio.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <string>

#include "log.h"

class Buffer{
public:
	Buffer(int capacity);
	~Buffer();

	//扩容
	void resize(int size);

	//得到剩余可以读的内存容量
	inline int readable_size() {
		return m_write_pos - m_read_pos;
	}

	//得到剩余可以写的内存容量
	inline int writable_size() {
		return m_capacity - m_write_pos;
	}

	//写数据
	//1.直接写
	//2.套接字接收数据
	int append_string(const char* data, int size);
	int append_string(const char* data);
	int append_string(const std::string& data);

	int socket_read(int fd);

	//根据 /r/n 取出一行数据
	char* find_crlf();

	//发送数据
	int send_data(int socket);

	inline char* data() {
		return m_data + m_read_pos;
	}

	inline int read_pos_add(int size) {
		m_read_pos += size;
		return m_read_pos;
	}

private:

	char* m_data;
	int m_capacity;
	int m_read_pos;
	int m_write_pos;
};
