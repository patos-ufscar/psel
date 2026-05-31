#define _GNU_SOURCE

#include "checksum.h"
#include "rules.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/if_tun.h>
#include <linux/if.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>


#define BUFFER_SIZE 2048

int check_mask(int prefix, uint32_t packet_net_ip, uint32_t rule_net_ip){
    uint32_t ip = ntohl(packet_net_ip);             //converte os enderecos de NBO (Network Byte Order) pra HBO (Host Byte Order)
    uint32_t netip = ntohl(rule_net_ip);

    if(prefix == 0) return 1;                   // caso "todos os ips sao validos (0.0.0.0/0)"

    uint32_t mask = 0xFFFFFFFF << (32 - prefix);

    if((ip & mask) == (netip & mask)){                   // usa a operacao AND bit a bit pra pegar a rede do ip 
        return 1;                               // e dps verifica se essa rede condiz com a do arquivo conf.rc (rede que queremos bloquear)
    } else {                                  
        return 0;                                
    }
    
}

void log_event(FILE *log_file ,const char *action, const char *src, const char *dst, int proto, int port) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);                                                  //usa a lib time para escrever um log detalhado 
    

    if (port > 0) {
        fprintf(log_file ,"[%02d:%02d:%02d] %-10s | %s -> %s | Proto: %d | Porta: %d\n", 
               tm.tm_hour, tm.tm_min, tm.tm_sec, action, src, dst, proto, port);            
    } else {
        fprintf(log_file ,"[%02d:%02d:%02d] %-10s | %s -> %s | Proto: %d\n",                //caso a porta seja invalida excluimos ela do log
               tm.tm_hour, tm.tm_min, tm.tm_sec, action, src, dst, proto);
    }

    fflush(log_file);               //atualiza o arquivo
}

int tun_alloc(char *dev) {
    struct ifreq ifr;
    int fd, err;

    // file descriptor pra interface tun
    if((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        perror("Erro ao abrir /dev/net/tun");
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI; // TUN (Camada 3) e sem informações extras de pacote

    if(*dev) strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    if((err = ioctl(fd, TUNSETIFF, (void*) &ifr)) < 0) {          //syscall usando todas os regras necessarias  
        perror("Erro no ioctl (TUNSETIFF)");
        close(fd);
        return err;
    }
    
    strcpy(dev, ifr.ifr_name);
    return fd;
}


int main() {
    FILE *log_file = fopen("/var/log/my_firewall.log", "a");

    if(log_file == NULL){
        printf("[X]erro ao abrir a log file");
        return 1;
    }

    int total_rules = 0;
    Rule *ip_list = load_rules("conf.rc", &total_rules);    // obtem a lista dos ips presentes no arquivo de configuracao

    if (ip_list == NULL || total_rules <= 0) {
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
    srand(time(NULL));
    
    // loop de processamento de pacotes
    while(1) {
        int nread = read(tunfd, buffer, sizeof(buffer));
        
        if(nread < 0) {
            perror("Erro de leitura");
            continue;
        }
        if ((size_t)nread < sizeof(struct iphdr)) {
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

        int pacote_processado = 0;

        for(int i = 0; i < total_rules; i++){

            // verifica se o ip do pacote ta na lista ou pertence a uma rede da lista
            if(ip->saddr == ip_list[i].ip || check_mask(ip_list[i].mask, ip->saddr, ip_list[i].ip)){  

                switch (ip_list[i].act)
                {
                case DENY:
                    printf("[!] IP %s Bloqueado (Regra DENY)\n", src_ip);
                    if(ip_list[i].verbose){
                        log_event(log_file, "DENY", src_ip, dst_ip, ip->protocol, 0);
                    }
                    pacote_processado = 1; // marca que o pacote foi tratado (descartado)
                    break;    
            
                case TARPIT:                                    
                    if(ip->protocol != IPPROTO_TCP){
                        break; // Se não for TCP, ignora o tarpit e avalia próximas regras ou da allow
                    }

                    if ((size_t)nread < (size_t)(ip->ihl * 4 + sizeof(struct tcphdr))) continue;
                    struct tcphdr *tcp = (struct tcphdr *)(buffer + (ip->ihl*4));

                    printf("[*] Aplicando TARPIT em %s:%d\n", src_ip, ntohs(tcp->th_sport));

                    if(ip_list[i].verbose){                    
                        log_event(log_file, "TARPIT", src_ip, dst_ip, ip->protocol, ntohs(tcp->th_sport));
                    }

                    // inverte os IPs 
                    uint32_t temp_ip = ip->daddr;
                    ip->daddr = ip->saddr;
                    ip->saddr = temp_ip;

                    // inverte as portas
                    uint16_t temp_p = tcp->th_sport;
                    tcp->th_sport = tcp->th_dport;
                    tcp->th_dport = temp_p;

                    // ajusta os numeros de sequencia
                    uint32_t attc_seq = ntohl(tcp->seq);
                    tcp->ack_seq = htonl(attc_seq + 1); 
                    tcp->seq = htonl(rand());

                    // Configura Flags de Resposta para prender a conexão
                    if(tcp->syn){
                        tcp->syn = 1;
                        tcp->ack = 1;
                    } else {
                        tcp->ack = 1;
                        tcp->psh = 0;
                    }
                    
                    // zera o parametro window (principal caracteristica do tarpit)
                    tcp->window = htons(0);

                    // Força o tamanho total do IP a ser apenas os cabeçalhos (corta payloads antigos)
                    int ip_hdr_len = ip->ihl * 4;
                    int tcp_hdr_len = tcp->doff * 4;
                    int novo_tot_len = ip_hdr_len + tcp_hdr_len;
                    ip->tot_len = htons(novo_tot_len);

                    // recalcula os checksums com os novos valores
                    ip->check = 0;
                    ip->check = checksum_ip((uint16_t *)ip, ip_hdr_len);

                    tcp->check = 0;
                    tcp->check = checksum_tcp(ip, tcp, tcp_hdr_len);

                    // manda o pacote tarpit modificado de volta pra rede
                    write(tunfd, buffer, novo_tot_len);
                    pacote_processado = 1;
                    break;

                case ALLOW:                 
                    if(ip_list[i].verbose){
                        log_event(log_file, "ALLOW", src_ip, dst_ip, ip->protocol, 0);
                    }

                    int write_bytes = write(tunfd, buffer, nread);
                    if(write_bytes < 0){
                        printf("[!] Erro ao encaminhar o pacote ALLOW\n");
                    }
                    pacote_processado = 1;
                    break;
                }
            }
            if (pacote_processado) break; // sai do laço de regras se o pacote já teve um veredito
        }

        // SE O PACOTE NÃO BATEU EM NENHUMA REGRA: 
        // Comportamento padrão : se n ta na lista, passa direto
        if (!pacote_processado) {
            write(tunfd, buffer, nread);
        }
    }
    close(tunfd);
    free(ip_list);
    if(log_file != NULL) fclose(log_file);
    return 0;
}