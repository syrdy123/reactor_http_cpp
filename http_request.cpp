#include "http_request.h"

HttpRequest::HttpRequest()
{
	reset();
}

HttpRequest::~HttpRequest()
{
}

void HttpRequest::reset()
{
	m_cur_state = ProcessState::ParseRequestLine;
	m_method = "";
	m_url = "";
	m_version = "";
	m_request_headers.clear();
}

void HttpRequest::add_header(const string key, const string value)
{
	if (key.empty() || value.empty()) return;

	m_request_headers.insert(make_pair(key, value));
}

string HttpRequest::get_header(const string key)
{
	auto it = m_request_headers.find(key);
	if (it == m_request_headers.end()) return string();

	return it->second;
}

bool HttpRequest::parse_http_request_line(Buffer* read_buffer)
{
	//�����еĽ���λ��
	char* end = read_buffer->find_crlf();


	//�����е���ʼλ��
	char* start = read_buffer->data();

	//�����еĳ���
	int line_size = static_cast<int>(end - start);

	if (line_size) {

		auto method_func = bind(&HttpRequest::set_method, this, placeholders::_1);
		start = split_request_line(start, end, " ", method_func);
		Debug("method = %s...", m_method.data());

		auto url_func = bind(&HttpRequest::set_url, this, placeholders::_1);
		start = split_request_line(start, end, " ", url_func);
		Debug("url = %s...", m_url.data());

		auto version_func = bind(&HttpRequest::set_version, this, placeholders::_1);
		split_request_line(start, end, nullptr, version_func);
		Debug("version = %s...", m_version.data());

#if 0
		//get /xxx/xxx/xxx.txt http/1.1

		//����ʽ
		char* space = memmem(start, line_size, " ", 1);
		assert(space != NULL);
		int method_size = space - start;
		request->method = (char*)malloc(method_size + 1);
		strnpy(request->method, start, method_size);
		request->method[method_size] = '\0';

		//����ľ�̬��Դ
		start = space + 1;
		space = memmem(start, end - start, " ", 1);
		assert(space != NULL);
		int url_size = space - start;
		request->url = (char*)malloc(url_size + 1);
		strnpy(request->url, start, url_size);
		request->url[url_size] = '\0';

		//�汾
		start = space + 1;
		int version_size = end - start;
		request->version = (char*)malloc(version_size + 1);
		strnpy(request->version, start, version_size);
		request->version[version_size] = '\0';

#endif

		//Ϊ��������ͷ��׼��
		read_buffer->read_pos_add(line_size + 2);

		//�޸�״̬
		set_state(ProcessState::ParseRequestHeader);

		return true;
	}

	return false;
}

bool HttpRequest::parse_http_request_head(Buffer* read_buffer)
{
	char* end = read_buffer->find_crlf();

	if (end != nullptr) {

		char* start = read_buffer->data();

		int line_size = static_cast<int>(end - start);

		char* mid = static_cast<char*>(memmem(start, line_size, ": ", 2));

		if (mid != nullptr) {
			//key ��value
			int key_size = static_cast<int>(mid - start);
			int value_size = static_cast<int>(end - mid - 2);

			if (key_size > 0 && value_size > 0) {
				string key(start, key_size);
				string value(mid + 2, value_size);
				add_header(key, value);
			}
			
			read_buffer->read_pos_add(line_size + 2);
	}
		else {
			//����ͷ�Ѿ�����������
			read_buffer->read_pos_add(2);

			//�޸ĵ�ǰ���� http �����״̬
			//������ʱֻ���� get ���� , ���Ժ���������
			set_state(ProcessState::ParseRequestDone);
		}
		return true;
}
	return false;
}

bool HttpRequest::parse_http_request(Buffer* read_buffer, HttpResponse* response, Buffer* send_buffer, int socket)
{
	bool flag;

	while (m_cur_state != ProcessState::ParseRequestDone) {

		switch (m_cur_state)
		{
		case ProcessState::ParseRequestLine:
			flag = parse_http_request_line(read_buffer);
			break;

		case ProcessState::ParseRequestHeader:
			flag = parse_http_request_head(read_buffer);
			break;

		case ProcessState::ParseRequestBody:
			break;

		default:
			break;
		}

		if (!flag) {
			//����http����Ĺ����г�������
			return flag;
		}

		//http �����Ѿ���������
		if (m_cur_state == ProcessState::ParseRequestDone) {
			Debug("http request parsing is completed...");

			//1.���ݽ�����������,�Կͻ��˵�������������
			process_http_request(response);


			//2.��֯������Ӧ���͸��ͻ���
			response->prepare_data(send_buffer, socket);

		}
	}
	//״̬��ԭ,���㴦����һ������
	m_cur_state = ProcessState::ParseRequestLine;

	return flag;
}

bool HttpRequest::process_http_request(HttpResponse* response)
{
	if (strcasecmp(m_method.data(), "get") != 0)
	{
		return -1;
	}

	m_url = decodeMsg(m_url);

	// ����ͻ�������ľ�̬��Դ(Ŀ¼�����ļ�)
	const char* file = NULL;

	if (strcmp(m_url.data(), "/") == 0)
	{
		file = "./";
	}
	else
	{
		file = m_url.data() + 1;
	}

	Debug("file = %s", file);

	// ��ȡ�ļ�����
	struct stat st;
	int ret = stat(file, &st);

	if (ret == -1)
	{
		// �ļ������� -- �ظ�404
		//sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);
		//sendFile("404.html", cfd);
		response->set_file_name("404.html");
		response->set_status_code(HttpStatusCode::NotFound);
		// ��Ӧͷ
		response->add_header("Content-type", get_file_type(".html"));
		response->send_data_func = send_file;
		return 0;
	}

	response->set_file_name(file);
	response->set_status_code(HttpStatusCode::OK);

	// �ж��ļ�����
	if (S_ISDIR(st.st_mode))
	{
		// �����Ŀ¼�е����ݷ��͸��ͻ���
		//sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
		//sendDir(file, cfd);
		// ��Ӧͷ
		response->add_header("Content-type", get_file_type(".html"));
		response->send_data_func = send_dir;
	}
	else
	{
		// ���ļ������ݷ��͸��ͻ���
		//sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size);
		//sendFile(file, cfd);
		// ��Ӧͷ
		response->add_header("Content-type", get_file_type(file));
		response->add_header("Content-length", to_string(st.st_size));
		response->send_data_func = send_file;
	}

	return false;
}

int HttpRequest::hexToDec(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;

	return 0;
}

string HttpRequest::decodeMsg(string msg)
{
	const char* from = msg.data();
	string res;

	for (; *from != '\0';++from)
	{
		// isxdigit -> �ж��ַ��ǲ���16���Ƹ�ʽ, ȡֵ�� 0-f
		// Linux%E5%86%85%E6%A0%B8.jpg
		if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
		{
			// ��16���Ƶ��� -> ʮ���� �������ֵ��ֵ�����ַ� int -> char
			// B2 == 178
			// ��3���ַ�, �����һ���ַ�, ����ַ�����ԭʼ����
			char c = static_cast<char>(hexToDec(from[1]) * 16 + hexToDec(from[2]));
			res.push_back(c);

			// ���� from[1] �� from[2] ����ڵ�ǰѭ�����Ѿ��������
			from += 2;
		}
		else
		{
			// �ַ�����, ��ֵ
			res.push_back(*from);
		}

	}
	res.push_back('\0');
	return res;
}

const string HttpRequest::get_file_type(const string& name)
{
	// a.jpg a.mp4 a.html
	// ����������ҡ�.���ַ�, �粻���ڷ���NULL
	const char* dot = strrchr(name.data(), '.');
	if (dot == NULL)
		return "text/plain; charset=utf-8";	// ���ı�
	if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
		return "text/html; charset=utf-8";
	if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
		return "image/jpeg";
	if (strcmp(dot, ".gif") == 0)
		return "image/gif";
	if (strcmp(dot, ".png") == 0)
		return "image/png";
	if (strcmp(dot, ".css") == 0)
		return "text/css";
	if (strcmp(dot, ".au") == 0)
		return "audio/basic";
	if (strcmp(dot, ".wav") == 0)
		return "audio/wav";
	if (strcmp(dot, ".avi") == 0)
		return "video/x-msvideo";
	if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
		return "video/quicktime";
	if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
		return "video/mpeg";
	if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
		return "model/vrml";
	if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
		return "audio/midi";
	if (strcmp(dot, ".mp3") == 0)
		return "audio/mpeg";
	if (strcmp(dot, ".ogg") == 0)
		return "application/ogg";
	if (strcmp(dot, ".pac") == 0)
		return "application/x-ns-proxy-autoconfig";

	return "text/plain; charset=utf-8";
}

int HttpRequest::send_dir(const string& dirName, Buffer* send_buffer, int cfd)
{
	char buf[4096] = { 0 };
	sprintf(buf, "<html><head><title>%s</title></head><body><table>", dirName);
	struct dirent** namelist;
	int num = scandir(dirName.data(), &namelist, NULL, alphasort);

	for (int i = 0; i < num; ++i)
	{
		// ȡ���ļ��� namelist ָ�����һ��ָ������ struct dirent* tmp[]
		char* name = namelist[i]->d_name;

		struct stat st;
		char subPath[1024] = { 0 };
		sprintf(subPath, "%s/%s", dirName.data(), name);
		stat(subPath, &st);

		if (S_ISDIR(st.st_mode))
		{
			// a��ǩ <a href="">name</a>
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>",
				name, name, st.st_size);
		}
		else
		{
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
				name, name, st.st_size);
		}

		send_buffer->append_string(buf);

#ifndef MSG_SEND_AUTO

		send_buffer->send_data(cfd);

#endif

		memset(buf, 0, sizeof(buf));
		free(namelist[i]);
	}
	sprintf(buf, "</table></body></html>");
	send_buffer->append_string(buf);

#ifndef MSG_SEND_AUTO

	send_buffer->send_data(cfd);

#endif

	free(namelist);

	return 0;
}

int HttpRequest::send_file(const string& fileName, Buffer* send_buffer, int cfd)
{
	// 1. ���ļ�
	Debug("fileName = %s...", fileName.data());

	int fd = open(fileName.data(), O_RDONLY);
	assert(fd > 0);
#if 1 
	while (1)
	{
		char buf[1024];

		ssize_t len = read(fd, buf, sizeof buf);

		if (len > 0)
		{
			send_buffer->append_string(buf, static_cast<int>(len));

#ifndef MSG_SEND_AUTO
			send_buffer->send_data(cfd);
#endif

			usleep(10); // ��ǳ���Ҫ
		}
		else if (len == 0)
		{
			break;
		}
		else
		{
			close(fd);
			perror("read");
		}
	}
#else
	off_t offset = 0;
	int size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	while (offset < size)
	{
		int ret = sendfile(cfd, fd, &offset, size - offset);
		printf("ret value: %d\n", ret);
		if (ret == -1 && errno == EAGAIN)
		{
			printf("û����...\n");
		}
	}
#endif
	close(fd);
	return 0;
}

//���������
char* HttpRequest::split_request_line(const char* start, const char* end, const char* sub, function<void(const string&)> call_back) {
	char* space = const_cast<char*>(end);

	if (sub != nullptr)
	{
		space = static_cast<char*>(memmem(start, end - start, sub, strlen(sub)));
		assert(space != nullptr);
	}

	int length = static_cast<int>(space - start);

	call_back(string(start, length));

	return space + 1;
}
