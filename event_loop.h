#pragma once

#include <thread>
#include <queue>
#include <map>
#include <mutex>

#include <assert.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "channel.h"
#include "dispatcher.h"




//���� channel �нڵ�ķ�ʽ
enum class ElemType:char{ADD,DELETE,MODIFY};

//����������еĽڵ�
struct ChannelElement {
	ElemType type; //��δ���ýڵ��е� channel
	Channel* channel;
};

class Dispatcher;

class EventLoop {
public:

	//��ʼ��
	EventLoop();

	EventLoop(const std::string thread_name);

	~EventLoop();

	//������Ӧ��ģ��
	int run();

	//��������ļ�������fd
	int event_activate(int fd, int event);

	//��������������
	int add_task(Channel* channel, ElemType type);

	//������������е�����
	int process_task_queue();

	//���� dispatcher �еĽڵ�
	int add(Channel* channel);

	int remove(Channel* channel);

	int modify(Channel* channel);

	//�ͷ� channel
	int channel_destroy(Channel* channel);

	inline std::thread::id get_thread_id() {
		return m_thread_id;
	}

private:
	int read_message();
	void taskWakeup();

private:
	bool m_is_quit;

	Dispatcher* m_dispatcher;

	//�������
	std::queue<ChannelElement*> m_task_queue;
	
	//map ��¼�� �ļ�������fd -> channel ��ӳ���ϵ
	std::map<int, Channel*> m_channel_map;

	//�߳� id , name ,mutex , cond
	std::thread::id m_thread_id;
	std::string m_thread_name;
	std::mutex m_mutex;

	int m_socket_pair[2]; //�洢����ͨ�ŵ� fd,�� socketpair ����ʼ��

};

