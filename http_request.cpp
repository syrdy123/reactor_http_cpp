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
	//请求行的结束位置
	char* end = read_buffer->find_crlf();


	//请求行的起始位置
	char* start = read_buffer->data();

	//请求行的长度
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

		//请求方式
		char* space = memmem(start, line_size, " ", 1);
		assert(space != NULL);
		int method_size = space - start;
		request->method = (char*)malloc(method_size + 1);
		strnpy(request->method, start, method_size);
		request->method[method_size] = '\0';

		//请求的静态资源
		start = space + 1;
		space = memmem(start, end - start, " ", 1);
		assert(space != NULL);
		int url_size = space - start;
		request->url = (char*)malloc(url_size + 1);
		strnpy(request->url, start, url_size);
		request->url[url_size] = '\0';

		//版本
		start = space + 1;
		int version_size = end - start;
		request->version = (char*)malloc(version_size + 1);
		strnpy(request->version, start, version_size);
		request->version[version_size] = '\0';

#endif

		//为解析请求头做准备
		read_buffer->read_pos_add(line_size + 2);

		//修改状态
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
			//key ：value
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
			//请求头已经被解析完了
			read_buffer->read_pos_add(2);

			//修改当前解析 http 请求的状态
			//由于暂时只解析 get 请求 , 所以忽略请求体
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
			//解析http请求的过程中出问题了
			return flag;
		}

		//http 请求已经解析完了
		if (m_cur_state == ProcessState::ParseRequestDone) {
			Debug("http request parsing is completed...");

			//1.根据解析出的数据,对客户端的请求作出处理
			process_http_request(response);


			//2.组织数据响应发送给客户端
			response->prepare_data(send_buffer, socket);

		}
	}
	//状态还原,方便处理下一条请求
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

	// 处理客户端请求的静态资源(目录或者文件)
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

	// 获取文件属性
	struct stat st;
	int ret = stat(file, &st);

	if (ret == -1)
	{
		// 文件不存在 -- 回复404
		//sendHeadMsg(cfd, 404, "Not Found", getFileType(".html"), -1);
		//sendFile("404.html", cfd);
		response->set_file_name("404.html");
		response->set_status_code(HttpStatusCode::NotFound);
		// 响应头
		response->add_header("Content-type", get_file_type(".html"));
		response->send_data_func = send_file;
		return 0;
	}

	response->set_file_name(file);
	response->set_status_code(HttpStatusCode::OK);

	// 判断文件类型
	if (S_ISDIR(st.st_mode))
	{
		// 把这个目录中的内容发送给客户端
		//sendHeadMsg(cfd, 200, "OK", getFileType(".html"), -1);
		//sendDir(file, cfd);
		// 响应头
		response->add_header("Content-type", get_file_type(".html"));
		response->send_data_func = send_dir;
	}
	else
	{
		// 把文件的内容发送给客户端
		//sendHeadMsg(cfd, 200, "OK", getFileType(file), st.st_size);
		//sendFile(file, cfd);
		// 响应头
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
		// isxdigit -> 判断字符是不是16进制格式, 取值在 0-f
		// Linux%E5%86%85%E6%A0%B8.jpg
		if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
		{
			// 将16进制的数 -> 十进制 将这个数值赋值给了字符 int -> char
			// B2 == 178
			// 将3个字符, 变成了一个字符, 这个字符就是原始数据
			char c = static_cast<char>(hexToDec(from[1]) * 16 + hexToDec(from[2]));
			res.push_back(c);

			// 跳过 from[1] 和 from[2] 因此在当前循环中已经处理过了
			from += 2;
		}
		else
		{
			// 字符拷贝, 赋值
			res.push_back(*from);
		}

	}
	res.push_back('\0');
	return res;
}

const string HttpRequest::get_file_type(const string& name)
{
	// a.jpg a.mp4 a.html
	// 自右向左查找‘.’字符, 如不存在返回NULL
	const char* dot = strrchr(name.data(), '.');
	if (dot == NULL)
		return "text/plain; charset=utf-8";	// 纯文本
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
		// 取出文件名 namelist 指向的是一个指针数组 struct dirent* tmp[]
		char* name = namelist[i]->d_name;

		struct stat st;
		char subPath[1024] = { 0 };
		sprintf(subPath, "%s/%s", dirName.data(), name);
		stat(subPath, &st);

		if (S_ISDIR(st.st_mode))
		{
			// a标签 <a href="">name</a>
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
	// 1. 打开文件
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

			usleep(10); // 这非常重要
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
			printf("没数据...\n");
		}
	}
#endif
	close(fd);
	return 0;
}

//拆分请求行
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
