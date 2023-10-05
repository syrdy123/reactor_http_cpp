#pragma once

#include <functional>

//定义函数指针
//using handle_function =  int(*)(void*);

//定义文件描述符的读写事件
enum class FDEvent {
	TIMEOUT = 0x01,
	READ_EVENT = 0x02,
	WRITE_EVENT = 0x04
};

class Channel {
public:
	//可调用对象
	using handle_function = std::function<int(void*)>;

	Channel(int fd, FDEvent events, handle_function read_func, handle_function write_func, handle_function destroy_func, void* arg);

	//回调函数
	handle_function read_callback;
	handle_function write_callback;
	handle_function destroy_callback;

	//设置 fd 的写事件 (设置被检查 或者 不被检测)
	void write_event_enable(bool flag);

	//判断是否需要检测文件描述符的写事件
	bool is_write_event_enable();

	//获取私有成员变量
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

	//文件描述符
	int m_fd;
	//事件
	int m_events;
	//回调函数的参数
	void* m_arg;
};


