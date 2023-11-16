#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <pthread.h>
#include <climits>

int epollFd = -1;

// 线程函数
void* threadFunction(void* arg) {
     // 创建客户端套接字
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        pthread_exit(nullptr);
    }

    // 连接服务器
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    if (inet_pton(AF_INET, "127.0.0.1", &(serverAddress.sin_addr)) <= 0) {
        std::cerr << "Invalid address" << std::endl;
        close(clientSocket);
        pthread_exit(nullptr);
    }

    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Failed to connect to server" << std::endl;
        close(clientSocket);
        pthread_exit(nullptr);
    }

    std::cout << "Connected to server" << std::endl;

    // 将客户端套接字添加到 epoll 实例中
    epoll_event event{};
    event.events = EPOLLIN;
    event.data.fd = clientSocket;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &event) == -1) {
        std::cerr << "Failed to add client socket to epoll" << std::endl;
        close(clientSocket);
        pthread_exit(nullptr);
    }

    // 发送数据给服务器
    // 不断循环的发送数据，间隔1秒
    // 获取thread唯一id
    pthread_t threadId = pthread_self();
    std::cout << "Thread id: " << threadId << std::endl;
    // cnt记录发送次数
    int cnt = 0;

    while (true) {
        usleep(100);  // 间隔1秒
        char message[10240];
        sprintf(message, "Hello from client! Thread: %lu client_cnt: %d\n", threadId, cnt);
        // std::cout<<"client message " << message << std::endl;
        // 发送数据给服务器
        ssize_t bytesSent = send(clientSocket, message, strlen(message), 0);
        if (bytesSent == -1) {
            std::cerr << "Failed to send data to server" << std::endl;
            close(clientSocket);
        }
        ++cnt;
        // 如果发送次数超过了int的最大值，退出线程
        if (cnt >= INT_MAX) {
            std::cout << "Exit thread" << std::endl;
            close(clientSocket);
            pthread_exit(nullptr);
        }
    }
    
    pthread_exit(nullptr);
}


int main() {

    // 主IO线程
    // 创建 epoll 实例
    epollFd = epoll_create1(0);
    if (epollFd == -1) {
        std::cerr << "Failed to create epoll instance" << std::endl;
        return 1;
    }

    // 创建多个线程
    const int numThreads = 10;  // 线程数量

    pthread_t threads[numThreads];
    for (int i = 0; i < numThreads; ++i) {
        if (pthread_create(&threads[i], nullptr, threadFunction, nullptr) != 0) {
            std::cerr << "Failed to create thread" << std::endl;
            return 1;
        }
    }

    // epoll_wait 循环等待事件发生
    const int maxEvents = 10;
    epoll_event events[maxEvents];
    while (true) {
        // 等待事件发生
        int numEvents = epoll_wait(epollFd, events, maxEvents, -1);
        if (numEvents == -1) {
            std::cerr << "Failed to wait for events" << std::endl;
            break;
        }
        std::cout << "Processed " << numEvents << " events" << std::endl;

        // 处理事件
        for (int i = 0; i < numEvents; ++i) {
            int clientSocket = events[i].data.fd;
            // 有数据可读
            char buffer[10240];
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
            std::cout << "Received response from server: "<<std::endl;
            std::cout << buffer << std::endl;   
        }
    }


    // 等待线程结束
    for (int i = 0; i < numThreads; ++i) {
        pthread_join(threads[i], nullptr);
    }
    

    // 关闭连接
    close(epollFd);

    return 0;
}