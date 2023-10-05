
#include <poll.h>
#include <stdlib.h>
#include <stdio.h>

#include "poll_dispatcher.h"
#include "dispatcher.h"

PollDispatcher::PollDispatcher(EventLoop* evloop) : Dispatcher(evloop)
{
    m_maxfd = 0;
    m_fds = new struct pollfd[m_maxNode];
    for (int i = 0; i < m_maxNode; ++i)
    {
        m_fds[i].fd = -1;
        m_fds[i].events = 0;
        m_fds[i].revents = 0;
    }
    m_name = "Poll";
}

PollDispatcher::~PollDispatcher()
{
    delete[]m_fds;
}

int PollDispatcher::add()
{
    short int events = 0;

    if (m_channel->get_events() & (int)FDEvent::READ_EVENT)
    {
        events |= POLLIN;
    }
    if (m_channel->get_events() & (int)FDEvent::WRITE_EVENT)
    {
        events |= POLLOUT;
    }

    int i = 0;
    for (; i < m_maxNode; ++i)
    {
        if (m_fds[i].fd == -1)
        {
            m_fds[i].events = events;
            m_fds[i].fd = m_channel->get_socket();
            m_maxfd = i > m_maxfd ? i : m_maxfd;
            break;
        }
    }
    if (i >= m_maxNode)
    {
        return -1;
    }
    return 0;
}

int PollDispatcher::remove()
{
    int i = 0;
    for (; i < m_maxNode; ++i)
    {
        if (m_fds[i].fd == m_channel->get_socket())
        {
            m_fds[i].events = 0;
            m_fds[i].revents = 0;
            m_fds[i].fd = -1;
            break;
        }
    }
    // 通过 channel 释放对应的 TcpConnection 资源
    m_channel->destroy_callback(const_cast<void*>(m_channel->get_arg()));
    if (i >= m_maxNode)
    {
        return -1;
    }
    return 0;
}

int PollDispatcher::modify()
{
    short int events = 0;

    if (m_channel->get_events() & (int)FDEvent::READ_EVENT)
    {
        events |= POLLIN;
    }

    if (m_channel->get_events() & (int)FDEvent::WRITE_EVENT)
    {
        events |= POLLOUT;
    }

    int i = 0;
    for (; i < m_maxNode; ++i)
    {
        if (m_fds[i].fd == m_channel->get_socket())
        {
            m_fds[i].events = events;
            break;
        }
    }
    if (i >= m_maxNode)
    {
        return -1;
    }
    return 0;
}

int PollDispatcher::dispatch(int timeout)
{
    int count = poll(m_fds, m_maxfd + 1, timeout * 1000);
    if (count == -1)
    {
        perror("poll");
        exit(0);
    }
    for (int i = 0; i <= m_maxfd; ++i)
    {
        if (m_fds[i].fd == -1)
        {
            continue;
        }

        if (m_fds[i].revents & POLLIN)
        {
            m_evLoop->event_activate(m_fds[i].fd, (int)FDEvent::READ_EVENT);
        }
        if (m_fds[i].revents & POLLOUT)
        {
            m_evLoop->event_activate(m_fds[i].fd, (int)FDEvent::WRITE_EVENT);
        }
    }
    return 0;
}
