#pragma once

#include <string>

#include "channel.h"
#include "event_loop.h"

using namespace std;

class EventLoop;
class Dispatcher
{
public:
    Dispatcher(EventLoop* evloop);
    virtual ~Dispatcher();
    // 添加
    virtual int add();
    // 删除
    virtual int remove();
    // 修改
    virtual int modify();
    // 事件监测
    virtual int dispatch(int timeout = 2); // 单位: s
    inline void update_channel(Channel* channel)
    {
        m_channel = channel;
    }

    protected:

    string m_name = string();
    Channel* m_channel;
    EventLoop* m_evLoop;
};