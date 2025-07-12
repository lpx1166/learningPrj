#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <map>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <fstream>

static int PORT = 8111;
static int MAX_CLIENT_COUNT = 3;
static int BUFFER_SIZE = 1024;

struct ClientInfo
{
    int socket;
    std::string username;
};

std::vector<ClientInfo> clients;
std::mutex clients_mutex;
std::map<std::string, std::string> users;

void broadcast_message(const std::string& strMsg, int exclude_fd) {
    for (auto& client : clients) {
        if (client.socket != exclude_fd) {
            send(client.socket, strMsg.c_str(), strMsg.size(), 0);
        }
    }
}


void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            std::cerr << "received is empty." << std::endl;
            break;
        }


        std::string strMsg(buffer);

        size_t pos = strMsg.find(':');
        std::string strCmd("");
        if (pos != std::string::npos) {
            strCmd = strMsg.substr(0, pos);
            strMsg = strMsg.substr(pos+1);
        }
        if (strCmd == "private" || strCmd == "p") {
            // 私聊
            std::cout << "private..." << std::endl;
        }
        else if (strCmd == "group" || strCmd == "g") {
            // 群发
            broadcast_message(strMsg, client_fd);

            // 保存历史消息
            std::ofstream history("server_history.txt", std::ios::app);
            if (history.is_open()) {
                // todo 加密
                history << strMsg << std::endl;
                history.close(); // 这里不会多线程竞争吗？
            }
        }
        else if (strCmd == "login") {
            // 登录
            {
                ClientInfo new_client{client_fd, strMsg};
                clients_mutex.lock();
                clients.push_back(new_client);
                clients_mutex.unlock();
            }

            std::string rsp = "welcome " + strMsg + " to charRoom";
            send(client_fd, rsp.c_str(), rsp.size(), 0);
            broadcast_message(rsp, client_fd);
            // 保存历史消息
            std::ofstream history("server_history.txt", std::ios::app);
            if (history.is_open()) {
                // todo 加密
                history << strMsg << std::endl;
                history.close(); // 这里不会多线程竞争吗？
            }
        }
        else {
            // ...
            std::cout << "errCMD:" << strCmd << ", msg:" << strMsg << std::endl; 
        }
    }
}

int main()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_fd, MAX_CLIENT_COUNT);

    std::cout << "server started on port: " << PORT << std::endl;

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_addr_len);
        std::thread(handle_client, client_fd).detach();
    }

    close(server_fd);
    return 0;
}
