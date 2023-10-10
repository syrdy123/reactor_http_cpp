# reactor_http_cpp

## 一、项目介绍

这是一个用C++实现的高性能web服务器，采用了Reactor多反应堆模型，底层分别对 select,poll,epoll 做了封装。

## 二、技术点

1. 用到了自己编写的线程池ThreadPoll
2. 编写了一个支持多线程并发读写的读写缓冲区Buffer
3. 使用了Reactor多反应堆模型,底层分别封装了select,poll,epoll
4. 能够解析Http Get请求,并返回Http响应
