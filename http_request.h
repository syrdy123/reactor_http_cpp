#pragma once
//#define _GNU_SOURCE 

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <map>
#include<functional>
#include <string>

#include "buffer.h"
#include "http_response.h"
#include "log.h"


using namespace std;

//解析状态
enum class ProcessState:char {
	ParseRequestLine,
	ParseRequestHeader,
	ParseRequestBody,
	ParseRequestDone
};

//定义 http 请求结构体
class HttpRequest {
public:
	HttpRequest();
	~HttpRequest();

	//重置
	void reset();

	//获取状态
	inline ProcessState get_state() {
		return m_cur_state;
	}

	//设置状态
	inline void set_state(ProcessState state) {
		m_cur_state = state;
	}

	//添加请求头
	void add_header(const string key, const string value);

	//根据 key 获得对应的 value
	string get_header(const string key);

	//解析请求行
	bool parse_http_request_line(Buffer* read_buffer);

	//解析请求头
	bool parse_http_request_head(Buffer* read_buffer);

	//解析 http 请求协议
	bool parse_http_request(Buffer* read_buffer, HttpResponse* response,
		Buffer* send_buffer, int socket);

	//处理 http 请求
	bool process_http_request(HttpResponse* response);

	//解码
	int hexToDec(char c);
	string decodeMsg(string from);

	//获得文件类型
	const string get_file_type(const string& name);

	//发送文件 或 目录
	static int send_dir(const string& dirName, Buffer* send_buffer, int cfd);
	static int send_file(const string& fileName, Buffer* send_buffer, int cfd);

private:
	char* split_request_line(const char* start, const char* end, const char* sub, function<void(const string&)> call_back);
	inline void set_method(const string& method) {
		m_method = method;
	}

	inline void set_url(const string& url) {
		m_url = url;
	}

	inline void set_version(const string& version) {
		m_version = version;
	}

private:
	string m_method;
	string m_url;
	string m_version;

	map<string, string> m_request_headers;
	ProcessState m_cur_state;
};


