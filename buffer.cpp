#include "buffer.h"


Buffer::Buffer(int capacity):m_capacity(capacity),m_read_pos(0),m_write_pos(0)
{
	m_data = (char*)malloc(static_cast<size_t>(capacity));

	bzero(m_data,static_cast<size_t>(capacity));
}

Buffer::~Buffer()
{
	if (m_data != nullptr) {
		free(m_data);
	}
}

void Buffer::resize(int size)
{
	//1.�ڴ湻��
	if (writable_size() >= size) return;

	//2.�ڴ���Ҫ�ϲ�����ʹ��
	//ʣ��Ŀ���д���ڴ� + �Ѷ����ڴ� >= size
	else if (writable_size() + m_read_pos >= size) {
		//��û�������ֽ���
		int unreaded_size = readable_size();

		memcpy(m_data, m_data + m_read_pos, static_cast<int>(unreaded_size));

		m_read_pos = 0;
		m_write_pos = unreaded_size;

	}

	//3.�ڴ治���� -- ����
	else {
		char* temp = static_cast<char*>(realloc(m_data, static_cast<int>(m_capacity + size)));

		if (temp == NULL) {
			printf("buffer ����ʧ��...\n");
			return;
		}

		//��ʼ��
		memset(temp + m_capacity, 0, static_cast<int>(size));

		//��������
		m_data = static_cast<char*>(temp);
		m_capacity += size;
	}
}

int Buffer::append_string(const char* data, int size)
{
	if (m_data == nullptr || size < 1) return -1;

	//����
	resize(size);

	//����(д)����
	memcpy(m_data + m_write_pos, data, static_cast<size_t>(size));
	m_write_pos += size;

	return 0;
}

int Buffer::append_string(const char* data)
{
	int size = static_cast<int>(strlen(data));
	int ret = append_string(data, size);

	return ret;
}

int Buffer::append_string(const std::string& data)
{
	int ret = append_string(data.data());
	return ret;
}

int Buffer::socket_read(int fd)
{
	struct iovec vec[2];

	//��ʼ������
	vec[0].iov_base = m_data + m_write_pos;
	int size = writable_size();
	vec[0].iov_len = size;

	char* temp_buf = static_cast<char*>(malloc(40960));

	vec[1].iov_base = m_data + m_write_pos;
	vec[1].iov_len = 40960;

	int res = static_cast<int>(readv(fd, vec, 2));

	if (res == -1) {
		return -1;
	}
	else if (res <= size) {
		m_write_pos += res;
		//buffer_append_data(buffer, temp_buf, (size_t)res - writable_size);
	}
	else {
		m_write_pos = m_capacity;
		append_string(temp_buf, res - size);
	}

	free(temp_buf);

	return res;
}

char* Buffer::find_crlf()
{
	char* ptr = static_cast<char*>(memmem(m_data + m_read_pos, readable_size(), "\r\n", 2));
	return ptr;
}

int Buffer::send_data(int socket)
{
	int readable = readable_size();

	if (readable > 0) {
		int count = static_cast<int>(send(socket, m_data + m_read_pos, readable, MSG_NOSIGNAL));
		if (count > 0) {
			m_read_pos += count;
			usleep(1);
		}
		return count;
	}
	return 0;
}
