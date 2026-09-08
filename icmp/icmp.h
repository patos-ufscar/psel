#ifndef ICMP_H
#define ICMP_H

#define ICMPSZ 8 // BYTES

#define ECHOREP 0 
#define ECHOREQ 8

#include <stdint.h>
#include <stdio.h>
#include "../ip/ip.h"

#pragma pack(1)

typedef struct {
	uint8_t type;
	uint8_t code;

	uint16_t checksum;

	uint16_t id;
	uint16_t seqnum;
} icmphdr;

typedef struct {
	uint8_t protocol;
	icmphdr * header;
	uint8_t * msg;
} icmp;

icmp * icmp_init(uint8_t * payload);
void icmp_fill(icmp * pack, uint8_t * payload);
icmphdr * icmp_extract(uint8_t * payload);

uint8_t icmp_check(icmp * pack, uint32_t realsz);
uint8_t icmp_echo_requested(uint8_t * stage, icmp * current, uint16_t * prev_seqnum);
void icmp_free(icmp * pack);


#endif
