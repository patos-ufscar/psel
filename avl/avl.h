#ifndef AVL_H
#define AVL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct avlnode{
	uint32_t key;
	int32_t height;
	int32_t balance;

	struct avlnode * father;
	struct avlnode * right;
	struct avlnode * left;

	void * data;
} avlnode;

typedef struct {
	avlnode *root;	
	uint32_t size;
} avl;

avlnode * avln_init(uint32_t key, void *data);
avl 	* avl_init();

avlnode * avl_insert(avl *t, avlnode *tracker, avlnode *node);
avlnode * avl_remove(avl *t, avlnode *tracker, uint32_t key);
avlnode * avl_search(avl *t, uint32_t key);

avlnode * rotate_right(avl *t, avlnode *a);
avlnode * rotate_left(avl *t, avlnode *a);

avlnode * rotate_right_left(avl *t, avlnode *a);
avlnode * rotate_left_right(avl *t, avlnode *a);

avlnode * sucessor(avlnode *a);

void 	see_tree(avlnode *node);
int32_t get_h(avlnode *a);
int32_t get_b(avlnode *a);

int32_t max(int32_t a, int32_t b);

#endif
