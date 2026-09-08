#ifndef STACK_H
#define STACK_H

#include <stdint.h>

typedef struct {
	uint32_t max;
	uint32_t size;
	uint32_t *array;
	uint8_t  err;
} stack;

stack * stack_init(uint32_t max);

uint8_t is_empty(stack *s);
uint32_t look_ahead(stack *s);

uint32_t pop(stack *s);
void add(stack *s, uint32_t val);


#endif
