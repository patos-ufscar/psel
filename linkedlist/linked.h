#ifndef LINKED_H
#define LINKED_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#pragma pack(1)

typedef struct lknode{
	void * key;
	uint32_t chars; // With null char
	uint8_t ways;

	struct lknode * forward;
} lknode;

typedef struct {
	lknode *head;
	uint32_t size;
} lklist;

lknode * lknode_init(void *data, uint32_t chars, uint8_t ways);
lklist * lklist_init();

uint8_t lk_insert(lklist *l, lknode *node);
uint8_t lk_remove(lklist *l, lknode * node);
lknode * lk_search(lklist *l, void * key);
void lklist_free(lklist *l);
void lklist_print(lklist *l);


#endif
