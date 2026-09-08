#ifndef UDP_H
#define UDP_H

#include "../ip/ip.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
	uint16_t source;
	uint16_t destin;
	uint16_t lenght;
	uint16_t checksum; 
} udphdr;

typedef struct {
	uint8_t protocol;
	udphdr *header;	
	uint8_t *msg;
} udp;


udp * set_udp(uint8_t *package);
udphdr * udp_extract(uint8_t * payload);

uint8_t udp_check(udp *package, uint32_t source, uint32_t destin, uint32_t realsz);
uint16_t udp_checksum(uint32_t lenght, uint8_t *msg);
void udp_free(udp *pack);

#endif
