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

	//����
	void resize(int size);

	//�õ�ʣ����Զ����ڴ�����
	inline int readable_size() {
		return m_write_pos - m_read_pos;
	}

	//�õ�ʣ�����д���ڴ�����
	inline int writable_size() {
		return m_capacity - m_write_pos;
	}

	//д����
	//1.ֱ��д
	//2.�׽��ֽ�������
	int append_string(const char* data, int size);
	int append_string(const char* data);
	int append_string(const std::string& data);

	int socket_read(int fd);

	//���� /r/n ȡ��һ������
	char* find_crlf();

	//��������
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
