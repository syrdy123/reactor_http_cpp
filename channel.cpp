#include "channel.h"

Channel::Channel(int fd, FDEvent events, handle_function read_func, handle_function write_func, handle_function destroy_func, void* arg)
{
	m_fd = fd;
	m_events = static_cast<int>(events);
	m_arg = arg;
	read_callback = read_func;
	write_callback = write_func;
	destroy_callback = destroy_func;

}


void Channel::write_event_enable( bool flag)
{
	if (flag) {
		m_events |= static_cast<int>(FDEvent::WRITE_EVENT);
	}
	else {
		m_events &= ~static_cast<int>(FDEvent::WRITE_EVENT);
	}
}

bool Channel::is_write_event_enable()
{
	return m_events & static_cast<int>(FDEvent::WRITE_EVENT);
}

