#ifndef TCP_H 
#define TCP_H

#pragma pack(1)

#define CONNECT  0
#define RECEIVE  2
#define STARTEND 3
#define END	 4
#define IGNORE	 5

#define FIN	0x01
#define SYN	0x02
#define PSH	0x08
#define ACK	0x10
#define WINSIZE 0xffff

#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "../ip/ip.h"

typedef struct {
	uint16_t source;
	uint16_t destin;
	uint32_t seqnum;
	uint32_t acknum;

	uint16_t reserved : 4, offset : 4, flags: 8;
	uint16_t winsiz;
	uint16_t checksum;
	uint16_t urgptr;

	uint8_t *options;
} tcphdr;

typedef struct {
	uint8_t protocol;
	tcphdr *header;	
	uint8_t *msg;
} tcp;

tcp * set_tcp(uint8_t *payload);
uint8_t set_optflags(tcp *pack, uint8_t options);

tcp * tcp_extract(uint8_t *payload);
tcphdr * tcph_extract(uint8_t *payload);

uint16_t tcp_check(tcp *pack, ip *info, uint16_t check);
uint16_t tcp_checksum(uint8_t * msg, uint16_t lenght);
uint8_t tcp_connect(tcp *pack, uint8_t *tw_stage, uint32_t seqnum, ip *info);
void tcp_free(tcp *pack);
uint32_t unix_random();

#endif
