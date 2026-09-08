#ifndef HASH_H
#define HASH_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#pragma pack(1)	

typedef struct fragment{
	/* UDP */
	uint16_t offset;
	uint8_t  MF;

	/* TCP  && ICMP */
	uint32_t acknum;
	uint32_t seqnum;

	/* BOTH */
	uint8_t  *data;
	uint8_t  *all;
	uint16_t psize;

	struct fragment * forward;
	struct fragment * backward;
} fragment;


typedef struct hashnode{

	struct hashnode * next;
	
	/* UDP */
	uint16_t id;
	uint16_t expected;
	uint16_t received;

	/* TCP  && ICMP -> used for ECHO Req */
	uint8_t  tw_stage; /* 1- SYN, 2- ACK, 3- FIN */
	uint8_t  options[255];

	/* BOTH */
	uint16_t source;
	uint16_t destin;
	uint16_t portin;
	uint16_t portout;
	uint8_t	 protocol;

	uint32_t fragstart;
	uint32_t fragend;
	fragment * box;
} hashnode;


typedef struct {
	uint32_t max;
	hashnode ** array;
} hash;


hashnode *hashn_init(uint16_t id, uint16_t source, uint16_t destin, uint16_t portin, uint16_t portout, uint8_t protocol, uint8_t tw_stage);
void 	fragpos_set(hashnode * hn, uint32_t fragstart, uint32_t fragend);
void 	free_hashn(hashnode *hn);
hash 	*hash_init(uint32_t max);
void 	go_down(hashnode *hn);
void 	free_hash(hash *h);
void 	add_hashn(hash *h, hashnode *hn);
uint8_t pop_hashn(hash *h, hashnode *hn);
uint8_t* hash_obtain_package(hashnode *hn);
uint8_t * hash_obtain_all(hashnode *hn, uint16_t initial_increment);

uint16_t missing(uint16_t lenght, uint16_t miss);

hashnode *search_hn(hash *h, uint16_t id, uint16_t source, uint16_t destin, uint16_t portin, uint16_t portout, uint8_t protocol);
uint8_t change_hashn(hash *h, hashnode * node, uint32_t to);

fragment * frag_init(uint16_t offset, uint8_t *data, uint8_t MF, uint32_t acknum, uint32_t seqnum, uint16_t size, uint8_t *all);
uint8_t add_frag(hashnode * hn, fragment *fr);
uint8_t pop_frag(hashnode * hn, fragment *fr);
uint8_t clear_frag(hashnode * hn);

#endif
