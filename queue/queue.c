#include "queue.h"
#include <stdlib.h>

queue* queue_init(uint32_t max, uint32_t size)
{
	queue *ret = (queue *) malloc(sizeof(queue));

	if (!ret)
	{
		return NULL;
	}

	ret->array = (uint8_t **) malloc(sizeof(uint8_t *) * max);

	if (!ret->array)
	{
		free(ret);
		return NULL;
	}
	
	ret->max  = max;

	while (max)
	{
		ret->array[max - 1] = (uint8_t *) calloc(size, sizeof(uint8_t));
		max--;
	}

	ret->size = 0;
	ret->head = 0;
	ret->tail = 0;

	return ret;
}


uint8_t* off_enq(queue *q)
{
	if (q->size == q->max - 1)
	{
		return NULL;
	}

	return q->array[q->tail];
}

void set_tail(queue *q)
{
	q->size++;
	
	if (q->tail == q->max - 1)
	{
		q->tail = 0;
	}
	else
	{
		q->tail++;
	}
	
	return ;
}

uint8_t* dequeue(queue *q)
{
	if (q->size == 0)
	{
		return NULL;
	}

	uint8_t *ret = q->array[q->head];
	q->size--;

	if (q->head == q->max - 1)
	{
		q->head = 0;
	}
	else
	{
		q->head++;
	}

	return ret;
}

void free_queue(queue *q)
{
	while (q->max)
	{
		free(q->array[q->max - 1]);
		q->max--;
	}

	free(q->array);

	free(q);

	return ;
}
