# Reactor高性能服务器
    本项目是从零实现了一个C/C++版本的基于TCP协议的Reactor网络服务器，
用epoll来处理大量并发连接的网络请求。服务器启动时，就创建线程池，创
建线程池后，若是任务队列无任务则每个子线程会阻塞在条件变量上；且每个
线程都有一个事件循环(one loop per thread)，用来处理epolll中就绪的事件。
主线程通过I/O多路复用(我选用的是epoll)接口来监听客户端的请求事件，收到
新的链接请求后，Accepter通过accept()处理连接请求，再创建Connection对
象，也会对所有的connectuon对象进行统一管理(放入容器中)；并将其交给子进
程，子进程的epoll来监听新的fd，负责读写事件；然后回调Handle函数，进行
连接后的相关操作。至此服务端与客户端已经连接上。此时将connection的fd让
epoll监听后，子线程是卡在epoll_wait上的，然后客户端给服务端发送消息时
，子线程epoll监听到有读事件就绪，epoll_wait返回成功，回调Connection::
onmessage来收发数据，若是当前时间-lasttime超过某个阈值，表示当前conne
ction为空闲连接，会断开；收到数据后，解析出一条完整的报文(这里为了保证
接发报文的完整性防止战报粘包问题，还创建了缓冲区类)，若是这个请求是CPU
密集型计算任务，可以将其交给工作线程(工作线程也用线程池)去计算；收发数
据都是由IO线程来做。由于任务为业务需求，所以将他放到上层去处理；而read
和send都是由IO线程处理。由于处理业务的任务是工作线程做的，那么如何将
send数据的操作交会给子线程(IO线程)做呢？用eventfd，将子线程的时间循环
在创建是就用epoll监听了eventfd，会将send的任务发到任务队列中，并唤醒
事件循环，也就是向eventfd中写入数据(数据无所谓是什么，只要写入即可)，
此时事件循环被eventfd唤醒，再执行发送操作；若是没有工作线程，则也就没
有这个操作了，直接发送即可。为了防止由空闲的connection浪费服务器资源，
还设置了定时器timerfd，也由epoll监听，每次闹钟响后判断TCP连接是否超时
(空闲：超过自己设定时间阈值未发送请求)，若是超时则断开此TCP连接。为了
优雅的退出该Reactor服务器，使用信号来停止服务器，捕获2号、15号信号，
当收到这两个信号后执行Stop函数，停止工作线程、从事件循环、IO线程、主
事件循环、主线程。
