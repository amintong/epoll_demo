#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring> // 添加这一行


int main() {
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

    // 发送数据给服务器
    const char* message = "Hello from client!";
    ssize_t bytesSent = send(clientSocket, message, strlen(message), 0);
    if (bytesSent == -1) {
        std::cerr << "Failed to send data to server" << std::endl;
        close(clientSocket);
        return 1;
    }

    std::cout << "Data sent to server" << std::endl;

    // 接收服务器的响应
    char buffer[1024];
    ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesRead == -1) {
        std::cerr << "Failed to receive response from server" << std::endl;
        close(clientSocket);
        return 1;
    }

    std::cout << "Received response from server: " << buffer << std::endl;

    // 关闭连接
    close(clientSocket);

    return 0;
}