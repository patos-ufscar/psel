#include "ServerSocket.h"
#include "HttpRequest.h"
#include "FileHandler.h"
#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <thread>

using namespace std;

ServerSocket::ServerSocket(const string& port, const string& doc_root) 
    : port(port), doc_root(doc_root), server_fd(-1) {}

void ServerSocket::handleClient(int client_fd){
    char buffer[4096];
    memset(buffer, 0, 4096);

    int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    
    if(bytes_received > 0){
        string raw_request(buffer);
        cout << "Requisição recebida...\n";

        HttpRequest req = parseHttpRequest(raw_request);
        
        FileHandler fileHandler(doc_root);
        HttpResponse res = fileHandler.handle(req);
        
        string response_str = res.toString();
        send(client_fd, response_str.c_str(), response_str.size(), 0);
    }
    close(client_fd);
}

void ServerSocket::start(){
    struct addrinfo hints, *servinfo, *p;
    int status;
    int yes = 1;

    // config basica, IPv4, TCP, localhost
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if((status = getaddrinfo(NULL, port.c_str(), &hints, &servinfo)) != 0){
        cerr << "getaddrinfo erro: " << gai_strerror(status) << endl;
        return;
    }

    for(p = servinfo; p != NULL; p = p->ai_next){
        if((server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) continue;
        if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) exit(1);
        if(bind(server_fd, p->ai_addr, p->ai_addrlen) == -1){
            close(server_fd);
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo);

    if(p == NULL){
        cerr << "server: falha ao fazer o bind" << endl;
        exit(1);
    }

    if(listen(server_fd, 10) == -1) exit(1);

    cout << "Servidor escutando na porta " << port << "..." << endl;

    struct sockaddr_storage their_addr;
    socklen_t sin_size;

    while(true){
        sin_size = sizeof their_addr;
        int client_fd = accept(server_fd, (struct sockaddr *)&their_addr, &sin_size);
        if(client_fd == -1) continue;

        // atende a conexão em uma nova thread
        thread(&ServerSocket::handleClient, this, client_fd).detach();
    }

    close(server_fd);
}