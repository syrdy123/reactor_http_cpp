#pragma once

#include <functional>

//���庯��ָ��
//using handle_function =  int(*)(void*);

//�����ļ��������Ķ�д�¼�
enum class FDEvent {
	TIMEOUT = 0x01,
	READ_EVENT = 0x02,
	WRITE_EVENT = 0x04
};

class Channel {
public:
	//�ɵ��ö���
	using handle_function = std::function<int(void*)>;

	Channel(int fd, FDEvent events, handle_function read_func, handle_function write_func, handle_function destroy_func, void* arg);

	//�ص�����
	handle_function read_callback;
	handle_function write_callback;
	handle_function destroy_callback;

	//���� fd ��д�¼� (���ñ���� ���� �������)
	void write_event_enable(bool flag);

	//�ж��Ƿ���Ҫ����ļ���������д�¼�
	bool is_write_event_enable();

	//��ȡ˽�г�Ա����
	inline int get_events() {
		return m_events;
	}

	inline int get_socket() {
		return m_fd;
	}

	inline const void* get_arg() {
		return m_arg;
	}

private:

	//�ļ�������
	int m_fd;
	//�¼�
	int m_events;
	//�ص������Ĳ���
	void* m_arg;
};


