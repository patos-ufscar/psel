#ifndef SERVER_SOCKET_H
#define SERVER_SOCKET_H

#include <string>

class ServerSocket{
private:
    std::string port;
    std::string doc_root;
    int server_fd;

    void handleClient(int client_fd);

public:
    ServerSocket(const std::string& port, const std::string& doc_root);
    void start();
};

#endif