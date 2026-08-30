#define _GNU_SOURCE

#include "checksum.h"
#include "rules.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/if_tun.h>
#include <linux/if.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#define BUFFER_SIZE 2048

int check_mask(int prefix, uint32_t packet_net_ip, uint32_t rule_net_ip){

    if (prefix < 0 || prefix > 32){
        return 0;
    }

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

    if(*dev){
        strncpy(ifr.ifr_name, dev, IFNAMSIZ - 1);
        ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    }

    if((err = ioctl(fd, TUNSETIFF, (void*) &ifr)) < 0) {          //syscall usando todas os regras necessarias  
        perror("Erro no ioctl (TUNSETIFF)");
        close(fd);
        return err;
    }
    
    strcpy(dev, ifr.ifr_name);
    return fd;
}

int check_payload(unsigned char *packet, int nread){
    struct iphdr *iph = (struct iphdr *)packet;
    int ip_header_len = iph->ihl * 4;

    unsigned char *payload = NULL;
    int payload_len = 0;

    if (iph->protocol == IPPROTO_TCP) {
        if(ip_header_len + (int)sizeof(struct tcphdr) > nread) return 0;

        struct tcphdr *tcph = (struct tcphdr *)(packet + ip_header_len);
        int tcp_header_len = tcph->doff *4;

        if(tcp_header_len < (int)sizeof(struct tcphdr)) return 0;

        payload = packet + ip_header_len + tcp_header_len;
        payload_len = ntohs(iph->tot_len) - (ip_header_len + tcp_header_len);
    }
    
    else if(iph->protocol == IPPROTO_UDP){
        payload = packet + ip_header_len + 8;
        payload_len = ntohs(iph->tot_len) - (ip_header_len + 8);
    } else {
        return 0;
    }

    if(payload_len <= 0) return 0;

    int payload_offset = payload - packet;

    if(payload_offset + payload_len > nread) return 0; //pacote truncado/malformatado

    const char *forbidden_words[] = {"hack", "malware", "exploit", "payload"};
    int num_words = 4;

    for(int i = 0; i < num_words; i++){
        int word_len = strlen(forbidden_words[i]);

        if(memmem(payload, payload_len, forbidden_words[i], word_len) != NULL){ //pacote malicioso detectado
            return 1;
        }
    }

    return 0;
}


int main() {
    FILE *log_file = fopen("/var/log/my_firewall.log", "a");

    if(log_file == NULL){
        printf("[X] erro ao abrir a log file");
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

        int ip_header_len = ip->ihl * 4;
        if(ip->ihl < 5 || ip_header_len > nread){
            continue; // cabecalho IP invalido/truncado - descarta
        }

        int malformed = 0;

        if(ip->protocol == IPPROTO_TCP || ip->protocol == IPPROTO_UDP){
            if(check_payload((unsigned char *)buffer, nread) == 1){  // verifica se existe alguma palavra maliciosa no payload
                printf("conexao malicionsa pacote dropado\n");
                continue;
            }
        }

        char src_ip[INET_ADDRSTRLEN];
        char dst_ip[INET_ADDRSTRLEN];                                
        inet_ntop(AF_INET, &ip->saddr, src_ip, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &ip->daddr, dst_ip, INET_ADDRSTRLEN);

        int processed_packet = 0;
        
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
                    processed_packet = 1; // marca que o pacote foi tratado (descartado)
                    break;    
            
                case TARPIT:                                    
                    if(ip->protocol != IPPROTO_TCP){
                        printf("[!] Pacote nao-TCP de %s bloqueado (regra TARPIT nao aplicavel)\n", src_ip);

                        if(ip_list[i].verbose){
                            log_event(log_file, "TARPIT-DENY", src_ip, dst_ip, ip->protocol, 0);
                        }
                        processed_packet = 1;
                        break; // se não for TCP, ignora o tarpit e avalia próximas regras ou da allow
                    }

                    if ((size_t)nread < (size_t)(ip_header_len + sizeof(struct tcphdr))){
                        malformed = 1;
                        break;
                    };
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

                    // flags especificas do tarpit
                    if(tcp->syn){
                        tcp->syn = 1;
                        tcp->ack = 1;
                    } else {
                        tcp->ack = 1;
                        tcp->psh = 0;
                    }
                    
                    // zera o parametro window (principal caracteristica do tarpit)
                    tcp->window = htons(0);

                    // força o tamanho total do IP a ser apenas os cabeçalhos (corta payloads antigos)
                    int tcp_hdr_len = tcp->doff * 4;
                    int novo_tot_len = ip_header_len + tcp_hdr_len;
                    ip->tot_len = htons(novo_tot_len);

                    // recalcula os checksums com os novos valores
                    ip->check = 0;
                    ip->check = checksum_ip((uint16_t *)ip, ip_header_len);

                    tcp->check = 0;
                    tcp->check = checksum_tcp(ip, tcp, tcp_hdr_len);

                    // manda o pacote tarpit modificado de volta pra rede
                    write(tunfd, buffer, novo_tot_len);
                    processed_packet = 1;
                    break;

                case ALLOW:
                    if(ip->protocol == IPPROTO_ICMP){
                        if(ip_header_len + (int)sizeof(struct icmphdr) > nread){
                            malformed = 1;
                            break;
                        }

                        struct icmphdr *icmph = (struct icmphdr *)(buffer + ip_header_len);

                        if(icmph->type == ICMP_ECHO){
                            printf("pacote icmp recebido");
                            icmph->type = ICMP_ECHOREPLY;

                            uint32_t original_saddr = ip->saddr;
                            ip->saddr = ip->daddr;
                            ip->daddr = original_saddr;

                            ip->check = 0;
                            ip->check = checksum_ip((uint16_t *)ip, ip_header_len);
                            
                            icmph->checksum = 0;

                            int icmp_len = ntohs(ip->tot_len) - ip_header_len;
                            icmph->checksum = checksum_ip((uint16_t *)icmph, icmp_len);
                
                        }
                    }

                    if(ip_list[i].verbose){
                        log_event(log_file, "ALLOW", src_ip, dst_ip, ip->protocol, 0);
                    }

                    int write_bytes = write(tunfd, buffer, nread);
                    if(write_bytes < 0){
                        printf("[!] Erro ao encaminhar o pacote ALLOW\n");
                    }
                    processed_packet = 1;
                    break;
                }
            }
            if (processed_packet) break; // sai do laço de regras se o pacote já teve um veredito
        }
        
        if(malformed){
            continue;
        }

        //comportamento padrão: se n ta na lista, fica como DENY
        if (!processed_packet) {
            printf("[!] Pacote de %s sem regra correspondente - descartado (default DENY)\n", src_ip);
            log_event(log_file, "DEFAULT-DENY", src_ip, dst_ip, ip->protocol, 0);
            
        }
    }
    close(tunfd);
    free(ip_list);
    if(log_file != NULL) fclose(log_file);
    return 0;
}