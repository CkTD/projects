# logserver #

## 概述 ##

实现了一个多进程的，事件驱动的，使用redis缓存的日志服务器。

主进程负责监控并同步工作从进程以及数据库。每个从进程都在同一个TCP端口上等待用户的连接并处理请求。从进程使用epoll实现了事件驱动的复用I/O机制。


## 相关技术及方法 ##

### epoll ###

epoll是Linux内核的可扩展I/O事件通知机制。它设计目的旨在取代既有POSIX select与poll系统函数，让需要大量操作文件描述符的程序得以发挥更优异的性能(举例来说：旧有的系统函数所花费的时间复杂度为O(n)，epoll的时间复杂度O(log n))。epoll与FreeBSD的kqueue类似，底层都是由可配置的操作系统内核对象建构而成，并以文件描述符(file descriptor)的形式呈现于用户空间。

#### epoll API ###

```c
int epoll_create1(int flags);
```

创建一个`epoll`对象并且返回文件描述符。`flags`参数可以改变`epoll`的默认行为。它只有一个合法的值，`EPOLL_CLOEXEC`。

```c
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
```

控制`epoll`监视的文件描述父。`op`可以是ADD、MODIFY或者DELETE。

```c
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
```

等待任何被`epoll_ctl`注册的事件，直到有一个发生或者超时。一次返回最多发生的事件到`events`。

### Redis ###

Redis是一个使用ANSI C编写的开源、支持网络、基于内存、可选持久性的键值对存储数据库。
Redis 有以下特点：

- Redis运行在内存中但是可以持久化到磁盘，在内存数据库方面的另一个优点是， 相比在磁盘上相同的复杂的数据结构，在内存中操作起来非常简单，这样Redis可以做很多内部复杂性很强的事情。同时，在磁盘格式方面他们是紧凑的以追加的方式产生的，因为他们并不需要进行随机访问。Redis支持数据的持久化，可以将内存中的数据保持在磁盘中，重启的时候可以再次加载进行使用。
- Redis不仅仅支持简单的key-value类型的数据，同时还提供list，set，zset，hash等数据结构的存储。而且Redis还支持 publish/subscribe, 通知, key 过期等等特性。
- Redis的所有操作都是原子性的，同时Redis还支持对几个操作全并后的原子性执行。
- 性能极高 – Redis能读的速度是110000次/s,写的速度是81000次/s 。

## 系统分析 ##

## 系统设计 ##

程序框架如下图所示
![frame](./frame.png)

## 系统实现 ##

### 主要文件 ###

- ae.c: 基于`epoll`的Eventloop，扩展了`epoll`并增加了时间事件，用于处理一些周期性的事件，比如检查客户超时。这个文件来自redis的源码，并将复杂的时间事件简化为了周期性的调用被注册的函数。
- anet.c: 基本的网络函数库，简化对套接字基本操作。来自redis。
- hiredis/ 一个redis客户端的C库
- daemon.c: 用于创建守护进程，让服务器可以守护进程工作。
- config.c: 用于解析参数，初始化服务器的配置。
- serverlog.c: 服务器的日志记录模块。
- db.c: redis数据库操作相关函数
- server.c: 实现Master，进行初始化，创建Woker，redis进程，经数据库缓存写入磁盘等。
- woker.c: 处理用户连接请求，将用户的日志数据发送给数据库。
- test.py: 用于模拟大量客户端同时请求服务的python测试脚本

## 系统测试 ##