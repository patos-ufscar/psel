#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

// nesse repositório amamos Linux e Windows igualmente...
#ifdef __linux__
    #include <arpa/inet.h>
    #define SOCKET_CLEANUP() 0
    #define CLOSE_SOCKET(x) close(x)
    #define READ_SOCKET(x, y, z) read(x, y, z)
    #define WRITE_SOCKET(x, y, z) write(x, y, z)
#elif _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #define SOCKET_CLEANUP() WSACleanup()
    #define CLOSE_SOCKET(x) closesocket(x)
    #define READ_SOCKET(x, y, z) recv(x, y, z, 0)
    #define WRITE_SOCKET(x, y, z) send(x, y, z, 0)
#else
    #error "sistema operacional nao suportado"
#endif

#define MAX_CONNECTIONS 5
#define PORT 8081
#define BUFFER_SIZE 2048
#define FILE_NAME "file.html"

int main()
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    char *not_found_response = "HTTP/1.1 404 Not Found\r\n\r\n<h1>404 File Not Found on server</h1>";

// Windows...
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("falha na inicializacao do Winsock.\n");
        return 1;
    }
#endif

    // IPV4 e TCP
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("falha na criacao do socket");
        exit(EXIT_FAILURE);
    }

    /* 
    * Configuração do socket
    * AF_INET: IPv4
    * INADDR_ANY: escutar em todas as interfaces de rede
    * htons(PORT): porta do servidor (aqui era o parametro que passamos para o contrutor)
    * toda essa parte era feita automáticamente pela biblioteca de Socket do Java
    * mas como estamos em C, a história é outra...
    */
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);


    /*
    * bind() associa o socket a um endereço e porta específicos
    * seria o equivalente em Java do construtor do ServerSocket em execução
    * o que preenchemos anteriormente é o parametro passado para o construtor
    * e aqui é o bind propriamente dito
    */
    int bindResult = bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    if (bindResult < 0)
    {
        perror("erro ao fazer bind");
        CLOSE_SOCKET(server_fd);
        SOCKET_CLEANUP();
        return 1;
    }

    /*
    * listen() coloca o socket em modo de escuta
    * o segundo parametro é o número máximo de conexões pendentes
    * em Java, os Sockets já são criados em modo de escuta
    * porém como aqui temos que fazer tudo manualmente, temos a vantagem
    * de poder implementar o MAX_CONNECTIONS de uma forma mais intuitiva
    */
    if (listen(server_fd, MAX_CONNECTIONS) < 0)
    {
        perror("erro ao escutar");
        CLOSE_SOCKET(server_fd);
        SOCKET_CLEANUP();
        exit(EXIT_FAILURE);
    }

    printf("servidor C escutando na porta %d\n", PORT);

    // agora com tudo inicializado, podemos seguir a mesma lógica do Java
    while (1)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("falha ao aceitar conexao");
            CLOSE_SOCKET(server_fd);
            SOCKET_CLEANUP();
            exit(EXIT_FAILURE);
        }

        printf("cliente conectado\n");
        READ_SOCKET(new_socket, buffer, BUFFER_SIZE);
        
        //debug
        //printf("requisição recebida: %s\n", buffer);

        char *requestedFile = NULL;
        requestedFile = strstr(buffer, "GET /");
        if (requestedFile == NULL)
        {
            requestedFile = strstr(buffer, "GET /file.html");
        }

        if (requestedFile != NULL)
        {
            int file_fd = open("file.html", O_RDONLY);
            if (file_fd >= 0)
            {
                // como estamos em C, nao e necessario manipular o tipo de dados do arquivo
                // ja que se trata de char em C, entao podemos ler diretamente
                char file_buffer[BUFFER_SIZE];
                int bytes_read;
                while ((bytes_read = read(file_fd, file_buffer, BUFFER_SIZE)) > 0)
                {
                    WRITE_SOCKET(new_socket, file_buffer, bytes_read);
                }
                close(file_fd);
            }
            else
            {
                WRITE_SOCKET(new_socket, not_found_response, strlen(not_found_response));
            }
        }
        else
        {
            WRITE_SOCKET(new_socket, not_found_response, strlen(not_found_response));
        }
        CLOSE_SOCKET(new_socket);
    }

    CLOSE_SOCKET(server_fd);
    SOCKET_CLEANUP();

    return 0;
}
