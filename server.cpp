#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <csignal>



int serverSocket = -1;  // 全局变量用于存储套接字
int epollFd = -1;  // 全局变量用于存储套接字

// SIGINT 信号处理函数
void handleSignal(int signal) {
    if (signal == SIGINT) {
        std::cout << "Received SIGINT signal. Cleaning up and exiting..." << std::endl;
        // 在这里使用全局变量 globalSocket 进行操作
        if (serverSocket != -1)
        {
            // 打印日志
            // 关闭套接字
            std::cout << "Closing socket..." << std::endl;
            // 关闭套接字
            close(serverSocket);
            
        }
        // 关闭 epoll 句子的文件描述符
        if (epollFd != -1)
        {
            // 打印日志
            // 关闭 epoll 句子的文件描述符
            std::cout << "Closing epoll fd..." << std::endl;
            // 关闭 epoll 句子的文件描述符    
            close(epollFd);
        }

        // 退出程序
        exit(0);
    }
}


int main()
{
    // 接收 SIGINT 信号
    // 注册所有信号的处理函数
    signal(SIGINT, handleSignal);
    


    // 创建服务器套接字
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    // 设置 SO_REUSEADDR 选项
    int reuse = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        std::cerr << "Failed to set SO_REUSEADDR option" << std::endl;
        close(serverSocket);
        return 1;
    }

    // 绑定服务器地址和端口
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8080);
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        std::cerr << "Failed to bind socket" << std::endl;
        close(serverSocket);
        return 1;
    }

    // 监听连接
    if (listen(serverSocket, 5) == -1)
    {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(serverSocket);
        return 1;
    }

    std::cout << "Server started. Listening on port 8080..." << std::endl;

    // 创建 epoll 实例
    epollFd = epoll_create1(0);
    if (epollFd == -1)
    {
        std::cerr << "Failed to create epoll instance" << std::endl;
        close(serverSocket);
        return 1;
    }

    // 将服务器套接字添加到 epoll 实例中
    epoll_event event{};
    event.events = EPOLLIN;
    event.data.fd = serverSocket;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverSocket, &event) == -1)
    {
        std::cerr << "Failed to add server socket to epoll" << std::endl;
        close(serverSocket);
        close(epollFd);
        return 1;
    }

    const int maxEvents = 10;
    epoll_event events[maxEvents];

    while (true)
    {
        // 等待事件发生
        int numEvents = epoll_wait(epollFd, events, maxEvents, -1);
        if (numEvents == -1)
        {
            std::cerr << "Failed to wait for events" << std::endl;
            break;
        }

        // 处理事件
        for (int i = 0; i < numEvents; ++i)
        {
            if (events[i].data.fd == serverSocket)
            {
                // 有新的连接请求
                sockaddr_in clientAddress{};
                socklen_t clientAddressLength = sizeof(clientAddress);
                int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientAddressLength);
                if (clientSocket == -1)
                {
                    std::cerr << "Failed to accept client connection" << std::endl;
                    continue;
                }

                std::cout << "Client connected" << std::endl;

                // 将客户端套接字添加到 epoll 实例中
                event.events = EPOLLIN;
                event.data.fd = clientSocket;
                if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &event) == -1)
                {
                    std::cerr << "Failed to add client socket to epoll" << std::endl;
                    close(clientSocket);
                    continue;
                }
            }
            else
            {
                // 有数据可读
                int clientSocket = events[i].data.fd;
                char buffer[10240];
                ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
                if (bytesRead <= 0)
                {
                    // 客户端断开连接或发生错误
                    if (bytesRead == 0)
                    {
                        std::cout << "Client disconnected" << std::endl;
                    }
                    else
                    {
                        std::cerr << "Failed to receive data from client" << std::endl;
                    }
                    close(clientSocket);
                    epoll_ctl(epollFd, EPOLL_CTL_DEL, clientSocket, nullptr);
                    continue;
                }

                std::cout << "Received data from client: " << buffer << std::endl;

                // 发送响应给客户端
                // 把buffer按\n进行分割，然后依次发送
                int start = 0;
                int end = 0;
                for (end = 0; end < strlen(buffer); end++)
                {
                    if (buffer[end] == '\n' || end == strlen(buffer)-1) 
                    {
                        ssize_t bytesSent = send(clientSocket, &buffer[start], end-start+1, 0);
                        if (bytesSent == -1)
                        {
                            std::cerr << "Failed to send data to client" << std::endl;
                            close(clientSocket);
                            epoll_ctl(epollFd, EPOLL_CTL_DEL, clientSocket, nullptr);
                            break;
                        }else{
                            start = end+1;
                        }
                        // usleep(1);
                    }   
                }                                        
            }
        }
    }

    // 关闭连接
    close(serverSocket);
    close(epollFd);

    return 0;
}