#pragma once
//#define _GNU_SOURCE 


#include <string.h>
#include <strings.h>
#include <stdlib.h>

#include <map>
#include <functional>

#include "buffer.h"

using namespace std;

enum class HttpStatusCode {
	Unknown,
	OK = 200,
	MovedPermanently = 301,
	MovdeTemporarily = 302,
	BadRequest = 400,
	NotFound = 404,
	BadGateway = 502
};


class HttpResponse {
public:
	function<void(const string&, Buffer*, int)> send_data_func;

	HttpResponse();
	~HttpResponse();

	//�����Ӧͷ
	void add_header(const string& key, const string& value);

	//��֯�ظ�������
	void prepare_data(Buffer* send_buffer, int socket);

	inline void set_file_name(const string& file_name) {
		m_file_name = file_name;
	}

	inline void set_status_code(HttpStatusCode status_code) {
		m_status_code = status_code;
	}

private:
	//״̬�� : ״̬�� , ״̬����
	HttpStatusCode m_status_code;
	string m_file_name;

	//��Ӧͷ -- ��ֵ��
	map<string, string> m_headers;

	const map<int, string> m_msg_info{
		{200, "OK"},
		{ 301,"MovedPermanently" },
		{ 302,"MovdeTemporarily" },
		{ 400,"BadRequest" },
		{ 404,"NotFound" },
		{ 502,"BadGateway" }
	};
};


