
#include <sys/epoll.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "epoll_dispatcher.h"
#include "dispatcher.h"

EpollDispatcher::EpollDispatcher(EventLoop* evloop) : Dispatcher(evloop)
{
    m_epfd = epoll_create(10);
    if (m_epfd == -1)
    {
        perror("epoll_create");
        exit(0);
    }

    m_events = new struct epoll_event[m_maxNode];
    m_name = "Epoll";
}

EpollDispatcher::~EpollDispatcher()
{
    close(m_epfd);
    delete[]m_events;
}

int EpollDispatcher::add()
{
    int ret = epollCtl(EPOLL_CTL_ADD);
    if (ret == -1)
    {
        perror("epoll_crl add");
        exit(0);
    }
    return ret;
}

int EpollDispatcher::remove()
{
    int ret = epollCtl(EPOLL_CTL_DEL);

    if (ret == -1)
    {
        perror("epoll_crl delete");
        exit(0);
    }
    // 通过 channel 释放对应的 TcpConnection 资源
    m_channel->destroy_callback(const_cast<void*>(m_channel->get_arg()));

    return ret;
}

int EpollDispatcher::modify()
{
    int ret = epollCtl(EPOLL_CTL_MOD);
    if (ret == -1)
    {
        perror("epoll_crl modify");
        exit(0);
    }
    return ret;
}

int EpollDispatcher::dispatch(int timeout)
{
    int count = epoll_wait(m_epfd, m_events, m_maxNode, timeout * 1000);
    for (int i = 0; i < count; ++i)
    {
        int events = m_events[i].events;
        int fd = m_events[i].data.fd;
        if (events & EPOLLERR || events & EPOLLHUP)
        {
            // 对方断开了连接, 删除 fd
            // epollRemove(Channel, evLoop);
            continue;
        }
        if (events & EPOLLIN)
        {
            m_evLoop->event_activate(fd, (int)FDEvent::READ_EVENT);
        }
        if (events & EPOLLOUT)
        {
            m_evLoop->event_activate(fd, (int)FDEvent::WRITE_EVENT);
        }
    }
    return 0;
}

int EpollDispatcher::epollCtl(int op)
{
    struct epoll_event ev;
    ev.data.fd = m_channel->get_socket();
    short int events = 0;

    if (m_channel->get_events() & (int)FDEvent::READ_EVENT)
    {
        events |= EPOLLIN;
    }

    if (m_channel->get_events() & (int)FDEvent::WRITE_EVENT)
    {
        events |= EPOLLOUT;
    }

    ev.events = events;
    int ret = epoll_ctl(m_epfd, op, m_channel->get_socket(), &ev);
    return ret;
}
