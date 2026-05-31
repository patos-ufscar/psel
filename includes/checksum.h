#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <stdint.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

uint16_t checksum_ip(uint16_t *addr, int count);

uint16_t checksum_tcp(struct iphdr *ip, struct tcphdr *tcp, int tcp_len);

#endif
