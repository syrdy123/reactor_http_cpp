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

//����״̬
enum class ProcessState:char {
	ParseRequestLine,
	ParseRequestHeader,
	ParseRequestBody,
	ParseRequestDone
};

//���� http ����ṹ��
class HttpRequest {
public:
	HttpRequest();
	~HttpRequest();

	//����
	void reset();

	//��ȡ״̬
	inline ProcessState get_state() {
		return m_cur_state;
	}

	//����״̬
	inline void set_state(ProcessState state) {
		m_cur_state = state;
	}

	//�������ͷ
	void add_header(const string key, const string value);

	//���� key ��ö�Ӧ�� value
	string get_header(const string key);

	//����������
	bool parse_http_request_line(Buffer* read_buffer);

	//��������ͷ
	bool parse_http_request_head(Buffer* read_buffer);

	//���� http ����Э��
	bool parse_http_request(Buffer* read_buffer, HttpResponse* response,
		Buffer* send_buffer, int socket);

	//���� http ����
	bool process_http_request(HttpResponse* response);

	//����
	int hexToDec(char c);
	string decodeMsg(string from);

	//����ļ�����
	const string get_file_type(const string& name);

	//�����ļ� �� Ŀ¼
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


