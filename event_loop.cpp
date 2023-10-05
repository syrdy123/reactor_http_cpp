#include "event_loop.h"
#include "select_dispatcher.h"
#include "poll_dispatcher.h"
#include "epoll_dispatcher.h"


// 写数据
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

    //用 bind 绑定可调用对象 function<int(void*)> 
    auto obj = std::bind(&EventLoop::read_message, this);

    // 指定规则: evLoop->socketPair[0] 发送数据, evLoop->socketPair[1] 接收数据
    Channel* channel = new Channel(m_socket_pair[1], FDEvent::READ_EVENT,
        obj, nullptr, nullptr, this);

    // channel 添加到任务队列
    add_task(channel, ElemType::ADD);

}

EventLoop::~EventLoop()
{
}

int EventLoop::run()
{
    m_is_quit = false;

    // 比较线程ID是否正常
    if (m_thread_id != std::this_thread::get_id())
    {
        return -1;
    }

    // 循环进行事件处理
    while (!m_is_quit)
    {
        m_dispatcher->dispatch();    // 超时时长 2s
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
    // 取出channel
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
    // 加锁, 保护共享资源
    m_mutex.lock();

    // 创建新节点
    ChannelElement* node = new ChannelElement;
    node->channel = channel;
    node->type = type;

    m_task_queue.push(node);
    
    m_mutex.unlock();

    /*
    * 细节:
    *   1. 对于链表节点的添加: 可能是当前线程也可能是其他线程(主线程)
    *       1). 修改fd的事件, 当前子线程发起, 当前子线程处理
    *       2). 添加新的fd, 添加任务节点的操作是由主线程发起的
    *   2. 不能让主线程处理任务队列, 需要由当前的子线程取处理
    */

    if (m_thread_id == std::this_thread::get_id())
    {
        // 当前子线程(基于子线程的角度分析)
        process_task_queue();
    }

    else
    {
        // 主线程 -- 告诉子线程处理任务队列中的任务
        // 1. 子线程在工作 2. 子线程被阻塞了:select, poll, epoll
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
            // 添加
            add(channel);
        }
        else if (node->type == ElemType::DELETE)
        {
            // 删除
            remove(channel);
        }
        else if (node->type == ElemType::MODIFY)
        {
            // 修改
            modify(channel);
        }
        delete node;
    }

    return 0;
}

int EventLoop::add(Channel* channel)
{
    int fd = channel->get_socket();

    // 找到fd对应的数组元素位置, 并存储
    if (m_channel_map.find(fd) == m_channel_map.end())
    {
        m_channel_map.insert(std::make_pair(fd, channel));
        //更新 dispatcher 中的 channel
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
    //更新 channel 
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

    //更新 channel 
    m_dispatcher->update_channel(channel);

    int ret = m_dispatcher->modify();
    return ret;
}

int EventLoop::channel_destroy(Channel* channel)
{
    auto it = m_channel_map.find(channel->get_socket());

    if (it != m_channel_map.end()) {
        //删除映射关系
        m_channel_map.erase(it);

        //关闭 channel 对应的 fd
        close(channel->get_socket());

        //释放channel
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

