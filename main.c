#include <stdio.h>
#include <stdbool.h>

// exit();
#include <stdlib.h>


#include <arpa/inet.h>
#include <unistd.h>


#define MAX_CONNECTIONS 65535



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

        char buff[255];
        while(read(client_fd, buff, 255))
        {
            // printar coisas na tela
            printf("%s", buff);
        }
    }

    // at this point should I even return?
    return 0;
}