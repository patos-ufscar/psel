#ifndef TRIE_H
#define TRIE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define NONE	0
#define IN 	0b01
#define OUT 	0b10
#define BOTH	0b11

typedef struct trnode {
	uint8_t		block;
	uint8_t 	ways;
	uint8_t 	deep;

	struct trnode 	*right;
	struct trnode 	*left;
} trnode;

typedef struct {
	trnode *root;
	uint32_t size;
} trtree;

trnode * trnode_init(uint8_t block, uint8_t ways, uint8_t deep);
trtree * trtree_init();

trnode * tr_search(trtree *t, uint32_t value, uint8_t deep, uint8_t sz);
uint8_t tr_insert(trtree *t, uint32_t value, uint8_t deep, uint8_t ways, uint8_t sz);
uint8_t tr_remove(trtree *t, uint32_t value, uint8_t deep, uint8_t ways, uint8_t sz);
uint8_t blocked(trtree *t, uint32_t value, uint8_t deep, uint8_t ways);

uint32_t * get_nodes(trtree *t, trnode **anodes, uint8_t deep);
void down(trnode *node, uint32_t *path, uint32_t *id, uint32_t mask, uint32_t val, trnode **anodes);

void tr_free(trnode * node);

#endif
