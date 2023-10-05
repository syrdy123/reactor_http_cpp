#include "event_loop.h"
#include "select_dispatcher.h"
#include "poll_dispatcher.h"
#include "epoll_dispatcher.h"


// д����
void EventLoop::taskWakeup()
{
    const char* msg = "i'm coming!";
    write(m_socket_pair[0], msg, strlen(msg));
}

EventLoop::EventLoop():EventLoop(std::string())
{
}

EventLoop::EventLoop(const std::string thread_name)
{
    m_is_quit = true;
    m_thread_id = std::this_thread::get_id();
    m_thread_name = (thread_name == std::string()) ? "MainThread" : thread_name;

    m_dispatcher = new SelectDispatcher(this);


    // map
    m_channel_map.clear();

    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, m_socket_pair);

    if (ret == -1)
    {
        perror("socketpair");
        exit(0);
    }

    //�� bind �󶨿ɵ��ö��� function<int(void*)> 
    auto obj = std::bind(&EventLoop::read_message, this);

    // ָ������: evLoop->socketPair[0] ��������, evLoop->socketPair[1] ��������
    Channel* channel = new Channel(m_socket_pair[1], FDEvent::READ_EVENT,
        obj, nullptr, nullptr, this);

    // channel ��ӵ��������
    add_task(channel, ElemType::ADD);

}

EventLoop::~EventLoop()
{
}

int EventLoop::run()
{
    m_is_quit = false;

    // �Ƚ��߳�ID�Ƿ�����
    if (m_thread_id != std::this_thread::get_id())
    {
        return -1;
    }

    // ѭ�������¼�����
    while (!m_is_quit)
    {
        m_dispatcher->dispatch();    // ��ʱʱ�� 2s
        process_task_queue();
    }

    return 0;
}

int EventLoop::event_activate(int fd, int event)
{
    if (fd < 0)
    {
        return -1;
    }
    // ȡ��channel
    Channel* channel = m_channel_map[fd];

    if (channel == nullptr) {
        return -1;
    }

    assert(channel->get_socket() == fd);

    if ((event & static_cast<int>(FDEvent::READ_EVENT)) && channel->read_callback)
    {
        channel->read_callback(const_cast<void*>(channel->get_arg()));
    }
    if ((event & static_cast<int>(FDEvent::WRITE_EVENT)) && channel->write_callback)
    {
        channel->write_callback(const_cast<void*>(channel->get_arg()));
    }
    return 0;
}

int EventLoop::add_task(Channel* channel, ElemType type)
{
    // ����, ����������Դ
    m_mutex.lock();

    // �����½ڵ�
    ChannelElement* node = new ChannelElement;
    node->channel = channel;
    node->type = type;

    m_task_queue.push(node);
    
    m_mutex.unlock();

    /*
    * ϸ��:
    *   1. ��������ڵ�����: �����ǵ�ǰ�߳�Ҳ�����������߳�(���߳�)
    *       1). �޸�fd���¼�, ��ǰ���̷߳���, ��ǰ���̴߳���
    *       2). ����µ�fd, �������ڵ�Ĳ����������̷߳����
    *   2. ���������̴߳����������, ��Ҫ�ɵ�ǰ�����߳�ȡ����
    */

    if (m_thread_id == std::this_thread::get_id())
    {
        // ��ǰ���߳�(�������̵߳ĽǶȷ���)
        process_task_queue();
    }

    else
    {
        // ���߳� -- �������̴߳�����������е�����
        // 1. ���߳��ڹ��� 2. ���̱߳�������:select, poll, epoll
        taskWakeup();
    }

    return 0;
}

int EventLoop::process_task_queue()
{

    while (!m_task_queue.empty())
    {
        m_mutex.lock();

        ChannelElement* node = m_task_queue.front();
        m_task_queue.pop();

        m_mutex.unlock();

        Channel* channel = node->channel;
        if (node->type == ElemType::ADD)
        {
            // ���
            add(channel);
        }
        else if (node->type == ElemType::DELETE)
        {
            // ɾ��
            remove(channel);
        }
        else if (node->type == ElemType::MODIFY)
        {
            // �޸�
            modify(channel);
        }
        delete node;
    }

    return 0;
}

int EventLoop::add(Channel* channel)
{
    int fd = channel->get_socket();

    // �ҵ�fd��Ӧ������Ԫ��λ��, ���洢
    if (m_channel_map.find(fd) == m_channel_map.end())
    {
        m_channel_map.insert(std::make_pair(fd, channel));
        //���� dispatcher �е� channel
        m_dispatcher->update_channel(channel);

        m_dispatcher->add();
    }
    return 0;
}

int EventLoop::remove(Channel* channel)
{
    int fd = channel->get_socket();

    auto it = m_channel_map.find(fd);

    if (it == m_channel_map.end())
    {
        return -1;
    }
    //���� channel 
    m_dispatcher->update_channel(channel);

    int ret = m_dispatcher->remove();
    return ret;
}


int EventLoop::modify(Channel* channel)
{
    int fd = channel->get_socket();

    if (m_channel_map.find(fd) == m_channel_map.end())
    {
        return -1;
    }

    //���� channel 
    m_dispatcher->update_channel(channel);

    int ret = m_dispatcher->modify();
    return ret;
}

int EventLoop::channel_destroy(Channel* channel)
{
    auto it = m_channel_map.find(channel->get_socket());

    if (it != m_channel_map.end()) {
        //ɾ��ӳ���ϵ
        m_channel_map.erase(it);

        //�ر� channel ��Ӧ�� fd
        close(channel->get_socket());

        //�ͷ�channel
        delete channel;
    }

    return 0;
}

int EventLoop::read_message()
{
    char buf[256];
    read(m_socket_pair[1], buf, sizeof(buf));
    return 0;
}

