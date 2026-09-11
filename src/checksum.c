// calculo do checksum do ip header e do tcp header

#include "checksum.h"
#include <arpa/inet.h>

uint16_t checksum_ip(uint16_t *addr, int count){
    uint32_t sum = 0;

    while(count > 1){
        sum += *addr++;
        count -= 2;
    }

    if(count > 0){
        sum += *(uint8_t *)addr;
    }

    while(sum >> 16){
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return (uint16_t)(~sum);
}

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