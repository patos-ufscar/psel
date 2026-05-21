#define _GNU_SOURCE

#include "rules.h"
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <time.h>

#define BUFFER_SIZE 2048

void log_event(FILE *log_file ,const char *action, const char *src, const char *dst, int proto, int port) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    

    if (port > 0) {
        fprintf(log_file ,"[%02d:%02d:%02d] %-10s | %s -> %s | Proto: %d | Porta: %d\n", 
               tm.tm_hour, tm.tm_min, tm.tm_sec, action, src, dst, proto, port);
    } else {
        fprintf(log_file ,"[%02d:%02d:%02d] %-10s | %s -> %s | Proto: %d\n", 
               tm.tm_hour, tm.tm_min, tm.tm_sec, action, src, dst, proto);
    }

    fflush(log_file);
}

int tun_alloc(char *dev) {
    struct ifreq ifr;
    int fd, err;

    
    if((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        perror("Erro ao abrir /dev/net/tun");
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI; // TUN (Camada 3) e sem informações extras de pacote

    if(*dev) strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    if((err = ioctl(fd, TUNSETIFF, (void*) &ifr)) < 0) {
        perror("Erro no ioctl (TUNSETIFF)");
        close(fd);
        return err;
    }
    
    strcpy(dev, ifr.ifr_name);
    return fd;
}


int main() {
    FILE *log_file = open("/var/log/my_firewall.log", "a");

    if(log_file == NULL){
        printf("[X]erro ao abrir a log file");
    }

    int total_rules = 0;
    Rule *ip_list = load_rules("conf.rc", &total_rules);

    if (ip_list == NULL || total_rules < 0) {
        printf("Falha ao inicializar o firewall. Saindo...\n");
        return 1;
    }

    printf("Sucesso! %d regras carregadas.\n", total_rules);

    char tun_name[IFNAMSIZ] = "tun0";
    int tunfd = tun_alloc(tun_name);

    if (tunfd < 0) {
        fprintf(stderr, "Falha ao criar a interface TUN.\n");
        return 1;
    }

    printf("Interface %s criada com sucesso. Iniciando Firewall...\n", tun_name);
    printf("Aguardando pacotes...\n\n");

    char buffer[BUFFER_SIZE];

    // loop de processamento de pacotes
    while(1) {
        int nread = read(tunfd, buffer, sizeof(buffer));
        if(nread < sizeof(struct iphdr)) {
            perror("Erro de leitura");
            continue;
        }

        // faz o cast do buffer para um cabeçalho IP
        struct iphdr *ip = (struct iphdr *)buffer;
        if(ip->version != 4){
            continue;
        }
    
        char src_ip[INET_ADDRSTRLEN];
        char dst_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &ip->saddr, src_ip, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &ip->daddr, dst_ip, INET_ADDRSTRLEN);

        for(int i = 0; i < total_rules; i++){
            if(ip->saddr == ip_list[i].ip){

                switch (ip_list[i].act)
                {
                case DENY:
                    if(ip_list[i].verbose){
                        log_event(log_file, "DENY", src_ip, dst_ip, ip->protocol, 0);
                    }
                    continue;
            
                case TARPIT:
                    if(ip_list[i].verbose){
                        log_event(log_file, "TARPIT", src_ip, dst_ip, ip->protocol, 0);
                    }
                    continue;

                case ALLOW:
                    if(ip_list[i].verbose){
                        log_event(log_file, "ALLOW", src_ip, dst_ip, ip->protocol, 0);
                    }

                    int write_bytes = write(tunfd, buffer, nread);
                    if(write_bytes < 0){
                        fprint("[!] erro ao encaminhar o pacode");
                    }
                    break;
                }
            
            }
        }
    }

    close(tunfd);
    free(ip_list);
    if(log_file != NULL) fclose(log_file);
    return 0;
}
