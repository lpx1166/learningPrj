#include <iostream>
#include <thread>
#include <mutex>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <fstream>
#include <string>

static std::string SERVER_IP = "127.0.0.1";
static int PORT = 8111;
static int BUFFER_SIZE = 1024;

std::mutex cout_mutex;
std::string username;

void receive_message(int client_fd) {
    char buffer[BUFFER_SIZE];
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            std::cerr << "recv empty." << std::endl;
            break;
        }
        std::string strMsg(buffer);

        // 保存历史消息
        std::ofstream history("client_history_" + username + ".txt", std::ios::app);
        if (history.is_open()) {
            history << strMsg << std::endl;
            history.close();
        }

        // todo 可能需要锁住输出，std::cout 不是线程安全的
        std::cout << strMsg << std::endl;
    }
    close(client_fd);
}

int main()
{
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP.c_str(), &server_addr.sin_addr);

    connect(client_fd, (sockaddr*)&server_addr, sizeof(server_addr));

    std::cout << "enter username:";
    std::cin >> username;

    std::string loginMsg = "login:" + username;
    send(client_fd, loginMsg.c_str(), loginMsg.size(), 0);
    char buff[BUFFER_SIZE];
    int bytes_received = recv(client_fd, buff, BUFFER_SIZE, 0);
    if (bytes_received <= 0) {
        std::cerr << "login failed. " << std::endl;
        return -1;
    }
    std::cout << "server rsp:" << buff << std::endl;

    std::thread receive_thread(receive_message, client_fd);

    while (true) {
        std::string strMsg;
        std::getline(std::cin >> std::ws, strMsg);
        send(client_fd, strMsg.c_str(), strMsg.size(), 0);
    }

    receive_thread.join();
    close(client_fd);
    return 0;
}