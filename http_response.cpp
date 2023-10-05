#include "http_response.h"

HttpResponse::HttpResponse()
{
    m_status_code = HttpStatusCode::Unknown;
    m_headers.clear();
    m_file_name = "";
    send_data_func = nullptr;
}

HttpResponse::~HttpResponse()
{
}

void HttpResponse::add_header(const string& key, const string& value)
{
    if (key.empty() || value.empty()) {
        return;
    }
    m_headers.insert(make_pair(key, value));
}

void HttpResponse::prepare_data(Buffer* send_buffer, int socket)
{
    // ״̬��
    char tmp[1024] = { 0 };
    int code = static_cast<int>(m_status_code);
    Debug("status_code = %d", code);

    sprintf(tmp, "HTTP/1.1 %d %s\r\n", code, m_msg_info.at(code).data());
    send_buffer->append_string(tmp);


    // ��Ӧͷ
    for (auto it = m_headers.begin();it != m_headers.end();++it)
    {
        sprintf(tmp, "%s: %s\r\n", it->first.data(), it->second.data());
        send_buffer->append_string(tmp);
    }

    // ����
    send_buffer->append_string("\r\n");

#ifndef MSG_SEND_AUTO

    send_buffer->send_data(socket);

#endif

    // �ظ�������
    send_data_func(m_file_name, send_buffer, socket);
}
