// calculo do checksum do ip header e do tcp header

#include "checksum.h"
#include <arpa/inet.h>

uint16_t checksum_ip(uint16_t *addr, int count){
    uint32_t sum = 0;    // acumulador de 32 bits para evitar overflow durante a soma

    // soma 2 bytes por vez emquanto houver pelo menos 2 bytes restantes
    while(count > 1){
        sum += *addr++;    // adiciona o valor apontado e avança o ponteiro para o prox bloco
        count -= 2;        // desconta os 2 bytes processados
    }
    
    // caso sobre 1 byte isolado
    if(count > 0){                    
        sum += *(uint8_t *)addr;
    }

    // soma os 16 bits mais significativos que excederam o limite com os 16 bits menos significativos,
    // repetindo o processo até que o acumulador caiba em 16 bits.
    while(sum >> 16){
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    // retorna o complemento de um (negação bit a bit) do resultado final como exige a RFC.
    return (uint16_t)(~sum);
}

// mesmo processo do checksum_ip, exceto pela adição de um pseudo-cabeçalho ip exigida pela RFC
uint16_t checksum_tcp(struct iphdr *ip, struct tcphdr *tcp, int tcp_len){
    uint32_t sum = 0;

    uint16_t *ip_src = (uint16_t *)&ip->saddr;
    sum += ip_src[0]; sum += ip_src[1];

    uint16_t *ip_dst = (uint16_t *)&ip->daddr;                          // pseudo ip header 
    sum += ip_dst[0]; sum += ip_dst[1];

    sum += htons(ip->protocol);
    sum += htons(tcp_len); 

    uint16_t *tcp_words = (uint16_t *)tcp;
    int count = tcp_len; 
    
    while (count > 1) {
        sum += *tcp_words++;
        count -= 2;
    }
    if (count > 0) {
        sum += *(uint8_t *)tcp_words;
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (uint16_t)(~sum);
}
