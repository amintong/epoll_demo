#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

int main() {

    // 创建 epoll 实例
    int epollFd = epoll_create1(0);
    if (epollFd == -1) {
        std::cerr << "Failed to create epoll instance" << std::endl;
        return 1;
    }

    // 创建客户端套接字
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    // 连接服务器
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    if (inet_pton(AF_INET, "127.0.0.1", &(serverAddress.sin_addr)) <= 0) {
        std::cerr << "Invalid address" << std::endl;
        close(clientSocket);
        return 1;
    }

    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Failed to connect to server" << std::endl;
        close(clientSocket);
        return 1;
    }

    std::cout << "Connected to server" << std::endl;



    // 将客户端套接字添加到 epoll 实例中
    epoll_event event{};
    event.events = EPOLLIN;
    event.data.fd = clientSocket;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &event) == -1) {
        std::cerr << "Failed to add client socket to epoll" << std::endl;
        close(clientSocket);
        close(epollFd);
        return 1;
    }


     // 发送数据给服务器
    const char* message = "Hello from client!";
    ssize_t bytesSent = send(clientSocket, message, strlen(message), 0);
    if (bytesSent == -1) {
        std::cerr << "Failed to send data to server" << std::endl;
        close(clientSocket);
    }

    const int maxEvents = 10;
    epoll_event events[maxEvents];

    while (true) {
        // 等待事件发生
        int numEvents = epoll_wait(epollFd, events, maxEvents, -1);
        if (numEvents == -1) {
            std::cerr << "Failed to wait for events" << std::endl;
            break;
        }

        // 处理事件
        for (int i = 0; i < numEvents; ++i) {
            if (events[i].data.fd == clientSocket) {
                // 有数据可读
                char buffer[1024];
                ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
                if (bytesRead <= 0) {
                    // 服务器断开连接或发生错误
                    if (bytesRead == 0) {
                        std::cout << "Server disconnected" << std::endl;
                    } else {
                        std::cerr << "Failed to receive data from server" << std::endl;
                    }
                    close(clientSocket);
                    close(epollFd);
                    return 1;
                }

                std::cout << "Received response from server: " << buffer << std::endl;
            }
        }
    }

    // 关闭连接
    close(clientSocket);
    close(epollFd);

    return 0;
}