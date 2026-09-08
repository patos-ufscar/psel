#include "stack.h"
#include <stdlib.h>

stack * stack_init(uint32_t max)
{
	stack *s = malloc(sizeof(stack));
	s->array = malloc(sizeof(uint32_t) * max);

	s->size 	= 0;
	s->max		= max;
	s->err		= 0;

	return s;
}

uint8_t is_empty(stack *s)
{
	return s->size ? 0 : 1;
}

uint32_t look_ahead(stack *s)
{
	s->err = 0;

	if (is_empty(s))
	{
		s->err = 1;
		return 0;
	}

	return s->array[s->size - 1];
}	

uint32_t pop(stack *s)
{
	s->err = 0;

	uint32_t ret = look_ahead(s);
	s->size--;

	return ret;
}

void add(stack *s, uint32_t val)
{
	s->err = 0;

	if (s->size == s->max)
	{
		s->err = 1;
		return ;
	}

	s->array[s->size] = val;
	s->size++;

	return ;
}
