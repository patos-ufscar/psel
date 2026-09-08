#ifndef IP_H
#define IP_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


#pragma pack(1)

typedef struct {
	uint8_t ihl : 4, type : 4;
	uint8_t tos;
	uint16_t tot_lenght;
	uint16_t id;

	uint16_t MF: 3, offset: 13;

	uint8_t ttl;
	uint8_t protocol;

	uint16_t checksum;

	uint32_t from;
	uint32_t to;
} ip;

struct package {
	uint8_t protocol;
	void *header;
	uint8_t * msg;
};


void ip_fill(ip *ret, uint8_t *payload);
void ip_cpy(ip * copied, ip * original);
ip* ip_init(uint8_t *payload);
ip* ip_extract(uint8_t *payload);
void show_ip(uint32_t uip);
uint8_t ipv4_check(uint8_t *payload);

uint32_t sum16from8(uint8_t a, uint8_t b);
uint32_t sum16from32(uint32_t a);
uint8_t from16to8(uint16_t a);
uint16_t from8to16(uint8_t a, uint8_t b);
uint32_t from16to32(uint16_t a, uint16_t b);
void fill8from32(uint8_t *arr, uint32_t b);
void fill8from16(uint8_t *arr, uint16_t b);

uint32_t endianness32(uint32_t a);
uint16_t endianness16(uint16_t a);
void free_protocol(struct package *a);
uint16_t checksum(uint8_t *data, uint16_t lenght, uint32_t start);


#endif
