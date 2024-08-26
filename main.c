#include <stdio.h>
#include <stdbool.h>

// exit();
#include <stdlib.h>


#include <arpa/inet.h>
#include <unistd.h>


#include <string.h>


#define MAX_CONNECTIONS 65535
#define RECV_BUF_SIZE 4096


void sendResponse(int client_fd, const char* response, const char* filepath)
{
    send(client_fd, response, strlen(response), 0);
    //sendFile();
    send(client_fd, "\r\n\r\n", 4, MSG_DONTWAIT);
    close(client_fd);
}


void handle_client(int client_fd)
{
    int bufferRealSize;

    // assumindo que um request não será maior que RECV_BUF_SIZE
    char buffer[RECV_BUF_SIZE];

    bufferRealSize = recv(client_fd, buffer, sizeof(buffer), 0);
    
    // isso ainda não está funcionando, mas enfim
    if(bufferRealSize > RECV_BUF_SIZE)
    {
        // isso significa que não conseguimos ler o pacote inteiro
        // retornar 500 internal server error
        sendResponse(client_fd, "HTTP/1.1 500 Internal Server Error", "");
        return;
    }

    // https://developer.mozilla.org/en-US/docs/Web/HTTP/Messages
    // https://www.tutorialspoint.com/c_standard_library/string_h.htm

    char* tokPtr; // este ponteiro é exclusivo para uso da funcao strtok_r
    char* startLine = strtok_r(buffer, "\r\n", &tokPtr); // strtok retorna a primeira linha

    // primeiro é necessário fazer o parsing e processamento do METHOD, PATH e HTTPVERSION,
    // pois a função strok_r modifica o buffer (primeiro argumento) 
    char* startLineTokPtr;
    char* METHOD = strtok_r(startLine, " ", &startLineTokPtr);
    
    // se o request for mal formado ou o método for diferente de GET
    if(METHOD == NULL || strcmp(METHOD, "GET") != 0)
    {
        sendResponse(client_fd, "HTTP/1.1 400 Bad Request", "");
        return;
    }

    // ainda é necessário filtrar "../" para previnir ataques
    char* PATH = strtok_r(NULL, " ", &startLineTokPtr);
    // se o path for um diretório retornar aquele mostrador bonitinho de arquivos
    // como em https://mirror.ufscar.br/
    // dps faço essa parte, agr é hora de commit!!

    // se o path for um diretório retornar aquele mostrador bonitinho de arquivos
    // como em https://mirror.ufscar.br/
    // dps faço essa parte, agr é hora de commit!!

    // retorno teste
    const char msgF[] = "HTTP/1.1 200 OK\r\n\r\n<h1>%s</h1>\r\n\r\n";
    char msg[512];
    sprintf(msg, msgF, PATH);
    send(client_fd, msg, strlen(msg), 0);
    close(client_fd);
}



int main()
{
    
    struct sockaddr_in server_addr;

    // equavalente a
    // inet_aton("127.0.0.1", &server_addr.sin_addr);
    // server_addr.sin_addr.s_addr = inet_network("127.0.0.1");
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // é melhor utilizar inet_aton pois de acordo com
    // https://www.gta.ufrj.br/ensino/eel878/sockets/inet_ntoaman.html
    // ao se utilizar o ip "255.255.255.255" o retorno de inet_addr() é igual a 0xffffffff que é interpretado como -1
    // mas -1 também é o retorno quando a função falha! portanto existe essa confusão de valores;
    // neste caso entretanto estou criando o endereço para o servidor então o ip nunca será 255.255.255.255

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);


    int server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

    // tcp/127.0.0.1:8080
    //                                                         handy way of not needing a variable
    // i checked and this works
    int setsockopt_status = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, "\x1", 4); // then len needs to be at least 4 idk why

    if(setsockopt_status != 0)
    {
        puts("an error occured");
        exit(-1);
    }


    // aumentar o tamanho do buffer de retorno
    setsockopt(server_fd, SOL_SOCKET, SO_RCVBUF, "\xff\xff\xff\xff", 4);

    // register address for socket
    if(bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind nao funcono");
        exit(-1);
    }

    if(listen(server_fd, MAX_CONNECTIONS))
    {
        perror("listen nao funcono");
        exit(-1);
    }


    while(true)
    {
        //struct sockaddr_in client_sockaddr_in;
        struct sockaddr_in client_sockaddr_in;
        socklen_t client_addr;
        
        // o casting aqui é necessário pois a funcão accept é comum a vários protocolos e sockaddr_in é específico de IPv4
        int client_fd = accept(server_fd, (struct sockaddr*)&client_sockaddr_in, &client_addr);

        handle_client(client_fd);
    }

    // at this point should I even return?
    return 0;
}