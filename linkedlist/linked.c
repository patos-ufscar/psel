#include "linked.h"

lknode * lknode_init(void * data, uint32_t chars, uint8_t ways)
{
	lknode * ret = (lknode *) malloc(sizeof(lknode));

	if (!ret || !data)
	{
		return NULL;
	}

	ret->key  = data;
	ret->chars = chars;
	ret->ways = ways;
	ret->forward = NULL;

	return ret;
}

lklist * lklist_init()
{
	lklist * ret = (lklist *) malloc(sizeof(lklist));

	if (!ret)
	{
		return NULL;
	}

	ret->head = NULL;
	ret->size = 0;

	return ret;
}

uint8_t lk_insert(lklist *l, lknode * node)
{
	if (node == NULL || l == NULL)
	{
		return 1;
	}

	lknode * tracker = l->head;
	if (tracker == NULL)
	{
		l->head = node;
		l->size++;
		return 0;
	}


	while (tracker->forward != NULL)
	{
		tracker = tracker->forward;
	}

	tracker->forward = node;
	l->size++;
	return 0;
}

uint8_t lk_remove(lklist *l, lknode * node)
{
	if (node == NULL || l == NULL)
	{
		return 1;
	}

	lknode * tracker = l->head;
	if (!tracker)
	{
		return 0;
	}

	if (tracker == node)
	{
		l->head = node->forward;
		l->size--;

		return 0;
	}

	while (tracker != NULL && tracker->forward != node)
	{
		tracker = tracker->forward;
	}

	if (!tracker)
	{
		return 1;
	}


	tracker->forward = node->forward;

	l->size--;
	return 0;
}

lknode * lk_search(lklist *l, void * key)
{
	lknode * tracker = l->head;

	while (tracker != NULL && strcmp(tracker->key, key))
	{
		tracker = tracker->forward;
	}

	return tracker;
}

void lklist_free(lklist *l)
{
	lknode * tracker = l->head;
	if (tracker == NULL)
	{
		return ;
	}

	lknode * forward = tracker->forward;
	while (tracker)
	{
		forward = tracker->forward;

		free(tracker->key);
		free(tracker);

		tracker = forward;
	}

	return ;
}

void lklist_print(lklist * l)
{
	lknode * tracker = l->head;

	while (tracker != NULL)
	{
		printf("%s\n", (char *) tracker->key);
		tracker = tracker->forward;
	}
	
	return ;
}	
