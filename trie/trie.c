#include "trie.h"

trnode * trnode_init(uint8_t block, uint8_t ways, uint8_t deep)
{
	trnode *ret 	= (trnode *) malloc(sizeof(trnode));
	ret->block	= block;
	ret->ways	= ways;
	ret->deep	= deep;

	ret->left	= NULL;
	ret->right	= NULL;

	return ret;
}

trtree * trtree_init()
{
	trtree *ret 	= (trtree *) malloc(sizeof(trtree));
	
	if (!ret)
	{
		return ret;
	}

	ret->size	= 0;
	ret->root	= trnode_init(0, NONE, 1);

	return ret;
}

trnode * tr_search(trtree *t, uint32_t value, uint8_t deep, uint8_t sz)
{
	trnode *target 	= t->root;
	uint32_t mask 	= 1U << (sz - 1);

	for (uint8_t i = 0; i < deep; i++)
	{
		if (target == NULL)
			break;

		if (value & mask)
			target = target->right;
		else
			target = target->left;

		mask = (mask >> 1);
	}

	return target;
}

uint8_t tr_insert(trtree *t, uint32_t value, uint8_t deep, uint8_t ways, uint8_t sz)
{
	uint32_t mask	= 1U << (sz - 1);
	trnode * target = t->root; 

	for (uint8_t i = 0; i < deep; i++)
	{
		if (value & mask)
		{
			if (target->right == NULL)
			{
				target->right = trnode_init(0, NONE, i + 1);
				if (target->right == NULL)
				{
					return 1;
				}
			}
			target = target->right;
		}
		else
		{
			if (target->left == NULL)
			{
				target->left = trnode_init(0, NONE, i + 1);
				if (target->left == NULL)
				{
					return 1;
				}
			}
			target = target->left;
		}

		mask = (mask >> 1);
	}

	if (!target->block)
	{
		target->block = 1;
		t->size++;
	}
	
	target->ways |= ways;

	return 0;
}

uint8_t tr_remove(trtree *t, uint32_t value, uint8_t deep, uint8_t ways, uint8_t sz)
{
	trnode * ret = tr_search(t, value, deep, sz);

	if (!ret)
	{
		return 1;
	}

	ret->ways &= ~ways;

	if (ret->ways == NONE)
	{
		ret->block = 0;
		t->size--;
	}
	return 0;
}

uint8_t blocked(trtree *t, uint32_t value, uint8_t deep, uint8_t ways)
{
	trnode *tracker = t->root;

	uint32_t mask = 1U << (deep - 1);


	for (uint8_t i = 0; i < deep; i++)
	{
		if (value & mask)
			tracker = tracker->right;
		else
			tracker = tracker->left;

		mask = (mask >> 1);

		if (!tracker)
		{
			return 0;
		}

		else if (tracker->block && (tracker->ways & ways))
		{
			return 1;
		}

	}

	return tracker->block && (tracker->ways & ways);
}

void tr_free(trnode * node)
{
	if (!node)
	{
		return ;
	}

	tr_free(node->left);
	tr_free(node->right);

	free(node);
}

uint32_t * get_nodes(trtree *t, trnode **anodes, uint8_t deep)
{
	uint32_t *path = calloc(t->size + 1, sizeof(uint32_t));
	uint32_t id = 0;

	down(t->root, path, &id, 1U << (deep - 1), 0, anodes);

	return path;
}
void down(trnode *node, uint32_t *path, uint32_t *id, uint32_t mask, uint32_t val, trnode **anodes)
{
    	if (node->block)
    	{
        	path[*id] = val;
        	anodes[*id] = node;
        	(*id)++;
   	}

  	if (node->left)
    	{
        	down(node->left, path, id, mask >> 1, val, anodes);
	}
	
	if (node->right)
    	{
        	down(node->right, path, id, mask >> 1, val | mask, anodes);
    	}
}
