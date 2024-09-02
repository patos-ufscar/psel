#include <stdio.h>
#include <stdbool.h>

// exit();
#include <stdlib.h>


#include <arpa/inet.h>
#include <unistd.h>


#include <string.h>


#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>

#define MAX_CONNECTIONS 65535
#define RECV_BUF_SIZE 4096


char* getMimeType(const char* filepath)
{
    // assumindo application/octet-stream pra tudo pq sim
    return "application/octet-stream";
}

void sendDirectory(int client_fd, const char* dirpath)
{
    char tempBuffer[1024];

    struct stat tempStat;

    DIR* dir = opendir(dirpath);
    struct dirent* temp;

    send(client_fd, "HTTP/1.1 200 OK\r\n\r\n", 19, MSG_DONTWAIT | MSG_MORE);
    // html roubado do mirrors.ufscar
    const char htmlBeg[] = "<html><head><title>Index of %s</title></head><body><h1>Index of %s</h1><table><tr><th valign=\"top\"><img src=\"/icons/blank.gif\" alt=\"[ICO]\"></th><th><a href=\"?C=N;O=D\">Name</a></th><th><a href=\"?C=M;O=A\">Last modified</a></th><th><a href=\"?C=S;O=A\">Size</a></th></tr> <tr><th colspan=\"4\"><hr></th></tr>";
    //                          gambiarra pra apagar o primeiro caracter
    sprintf(tempBuffer, htmlBeg, dirpath+1, dirpath+1);
    send(client_fd, tempBuffer, strlen(tempBuffer), MSG_DONTWAIT | MSG_MORE);
    printf("%s\r\n", dirpath);
    
    bool isDir;
    while((temp = readdir(dir)) != NULL)
    {
        if(strcmp(temp->d_name, ".") == 0) continue;
        if(strcmp(temp->d_name, "..") == 0){
            send(client_fd, "<tr><td valign=\"top\"><img src=\"/icons/parent-dir.gif\" alt=\"[DIR]\"></td><td><a href=\"../\">Parente Dir</a></td><td align=\"right\">tiem</td><td align=\"right\"> 0.00 </td><td>&nbsp;</td></tr>", 180, 0);
            continue;
        }

        sprintf(tempBuffer, "%s/%s", dirpath, temp->d_name);
        if(stat(tempBuffer, &tempStat) != 0)
        {
            // houve algum erro ao ler o arquivo n sei pq
            continue;
        }
        isDir = S_ISDIR(tempStat.st_mode);

        printf(" -> %s\r\n", temp->d_name);
        sprintf(
            tempBuffer,
            "<tr><td valign=\"top\"><img src=\"/icons/%s.gif\" alt=\"[%s]\"></td><td><a href=\"%s%s%s\">%s</a></td><td align=\"right\">%s</td><td align=\"right\"> %.2f </td><td>&nbsp;</td></tr>",
            isDir ? "folder" : "file", isDir ? "DIR" : "FIL", dirpath+1, temp->d_name, S_ISDIR(tempStat.st_mode) ? "/" : "", temp->d_name, "tiem", tempStat.st_ctime);
        send(client_fd, tempBuffer, strlen(tempBuffer), MSG_DONTWAIT | MSG_MORE);
    }
    send(client_fd, "<tr><th colspan=\"5\"><hr></th></tr></table></body></html>", 56, MSG_DONTWAIT | MSG_MORE);
    send(client_fd, "\r\n\r\n", 4, 0);
    close(client_fd);
    // chamar closedir nao funciona n sei pq
    // closedir(dir);
}


void sendFile(int client_fd, const char* filepath, struct stat fileStat)
{
    FILE* file = fopen(filepath, "rb");
    if(file == NULL)
    {
        sendResponse(client_fd, "HTTP/1.1 500 Internal Server Error", "");
        return;
    }

    // aqui assumimos que o arquivo existe e pode ser lido etc.
    char buffer[8192];
    int readBytes;
    
    // send important headers
    char contentHeaders[256];
    char* mimeType = getMimeType(filepath);
    send(client_fd, "HTTP/1.1 200 OK\r\n", 17, MSG_DONTWAIT | MSG_MORE);
    sprintf(contentHeaders, "Content-Length: %lu\r\nContent-Type: %s\r\n", fileStat.st_size, mimeType);
    send(client_fd, contentHeaders, strlen(contentHeaders), MSG_DONTWAIT | MSG_MORE);
    send(client_fd, "\r\n", 2, MSG_DONTWAIT | MSG_MORE);
    while((readBytes = fread(buffer, sizeof(char), sizeof(buffer), file)) == sizeof(buffer))
    {
        // dont wait pra ser mais rapido
        send(client_fd, buffer, readBytes, MSG_DONTWAIT | MSG_MORE);
    }
    // enviar o ultimo pacote caso haja
    if(readBytes)
    {
        send(client_fd, buffer, readBytes, MSG_DONTWAIT | MSG_MORE);
    }

    // de acordo com a documentação da microsoft
    // neste estágio o arquivo pode ter dado algum erro de leitura
    // ou apenas ter terminado de ser lido, portanto é necessário checar

    if(feof(file))
    {
        // aqui o arquivo foi lido mesmo!!
        send(client_fd, "\r\n\r\n", 4, 0);
    }
    // caso haja erros
    // aqui o header ja foi enviado então não tem como
    // "salvar" o request
    close(client_fd);
}


void sendResponse(int client_fd, const char* response, const char* filepath)
{
    send(client_fd, response, strlen(response), 0);
    //sendFile(); remover linha de baixo
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


    // este pedaço de código é necessário para 
    // transformar o caminho oferecido pelo cliente ex: /arquivo
    // em algo utilizavel pelo so ex: ./arquivo
    char* DOTPATH;
    DOTPATH = malloc(sizeof(char) * (strlen(PATH) + 1));
    if(DOTPATH == NULL)
    {
        perror("Não foi possível alocar memória");
        sendResponse(client_fd, "HTTP/1.1 500 Internal Server Error", "");
        return;
    }
    DOTPATH[0] = '.';
    strcat(DOTPATH, PATH);


    // https://stackoverflow.com/questions/4553012/checking-if-a-file-is-a-directory-or-just-a-file
    struct stat pathStat;

    int statCode = stat(DOTPATH, &pathStat);
    switch(statCode)
    {
        case 0:
            break;
        case EACCES:
            // punish user bc they tried to hack the server
            sendResponse(client_fd, "HTTP/1.1 200 OK", "");
            return;
        default:
            // algo deu errado no servidor
            return sendResponse(client_fd, "HTTP/1.1 500 Internal Server Error", "");
    }

    if (S_ISDIR(pathStat.st_mode))
    {
        sendDirectory(client_fd, DOTPATH);
    }else{
        sendFile(client_fd, DOTPATH, pathStat);
    }

    free(DOTPATH);
    close(client_fd);
}



int main()
{
    
    struct sockaddr_in server_addr;

    // equavalente a
    // inet_aton("127.0.0.1", &server_addr.sin_addr);
    // server_addr.sin_addr.s_addr = inet_network("127.0.0.1");
    server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");

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