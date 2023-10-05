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




//处理 channel 中节点的方式
enum class ElemType:char{ADD,DELETE,MODIFY};

//定义任务队列的节点
struct ChannelElement {
	ElemType type; //如何处理该节点中的 channel
	Channel* channel;
};

class Dispatcher;

class EventLoop {
public:

	//初始化
	EventLoop();

	EventLoop(const std::string thread_name);

	~EventLoop();

	//启动反应堆模型
	int run();

	//处理激活的文件描述符fd
	int event_activate(int fd, int event);

	//添加任务到任务队列
	int add_task(Channel* channel, ElemType type);

	//处理任务队列中的任务
	int process_task_queue();

	//处理 dispatcher 中的节点
	int add(Channel* channel);

	int remove(Channel* channel);

	int modify(Channel* channel);

	//释放 channel
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

	//任务队列
	std::queue<ChannelElement*> m_task_queue;
	
	//map 记录了 文件描述符fd -> channel 的映射关系
	std::map<int, Channel*> m_channel_map;

	//线程 id , name ,mutex , cond
	std::thread::id m_thread_id;
	std::string m_thread_name;
	std::mutex m_mutex;

	int m_socket_pair[2]; //存储本地通信的 fd,用 socketpair 来初始化

};

