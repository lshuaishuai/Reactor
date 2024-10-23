all:client echoserver client1 bankserver
client:client.cpp
	g++ -g -o client client.cpp

client1:client1.cpp
	g++ -g -o client1 client1.cpp

echoserver:echoserver.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp EventLoop.cpp TcpServer.cpp Accepter.cpp Connection.cpp Buffer.cpp EchoServer.cpp ThreadPool.cpp Timestamp.cpp
	g++ -g -std=c++11 -lpthread -o echoserver echoserver.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp EventLoop.cpp TcpServer.cpp Accepter.cpp Connection.cpp Buffer.cpp EchoServer.cpp ThreadPool.cpp Timestamp.cpp

bankserver:bankserver.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp EventLoop.cpp TcpServer.cpp Accepter.cpp Connection.cpp Buffer.cpp BankServer.cpp ThreadPool.cpp Timestamp.cpp
	g++ -g -std=c++11 -lpthread -o bankserver bankserver.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp EventLoop.cpp TcpServer.cpp Accepter.cpp Connection.cpp Buffer.cpp BankServer.cpp ThreadPool.cpp Timestamp.cpp

clean:
	rm -f client echoserver client1 bankserver