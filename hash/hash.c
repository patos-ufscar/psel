#include "hash.h"

hashnode *hashn_init(uint16_t id, uint16_t source, uint16_t destin, uint16_t portin, uint16_t portout, uint8_t protocol, uint8_t tw_stage)
{
	hashnode *ret = (hashnode *) malloc(sizeof(hashnode));

	ret->id 	= id;
	ret->source 	= source;
	ret->destin 	= destin;
	ret->protocol 	= protocol;
	ret->tw_stage 	= tw_stage;
	ret->portin	= portin;
	ret->portout 	= portout;
	ret->expected	= 0;
	ret->received	= 0;

	ret->box 	= NULL;
	ret->next 	= NULL;

	return ret;
}

void free_hashn(hashnode *hn)
{

	// limpa fragmentos
	fragment * current = hn->box;
	if (current == NULL)
	{
		free(hn);
		return ;
	}
		

	fragment * forward = current->forward;

	while (forward != NULL)
	{
		free(current->all);
		free(current->data);
		free(current);

		current = forward;
		forward = current->forward;
	}
	free(current->all);
	free(current->data);
	free(current);

	free(hn);
	return ;
}


hash * hash_init(uint32_t max)
{
	hash *ret = (hash *) malloc(sizeof(hash));
	ret->array = (hashnode **) calloc(max, sizeof(hashnode *));

	ret->max = max;

	return ret;
}

void free_hash(hash *h)
{
	for (uint32_t i = 0; i < h->max; i++)
	{
		go_down(h->array[i]);
	}

	free(h->array);
	free(h);
	return ;
}

void go_down(hashnode *hn)
{
	if (hn == NULL)
	{
		return ;
	}

	go_down(hn->next);
	free_hashn(hn);

	return ;
}

void add_hashn(hash *h, hashnode * hn)
{	
	uint16_t id = hn->id;

	if (h->array[id] == NULL)
	{
		h->array[id] = hn;
		return ;
	}

	hashnode * tracker = h->array[id];
	while (tracker->next != NULL)
	{
		tracker = tracker->next;
	}

	tracker->next = hn;
	return ;
}

uint8_t pop_hashn(hash *h, hashnode *hn)
{
	uint16_t id = hn->id;

	if (h->array[id] == NULL)
	{
		return 1;
	}
	
	hashnode * tracker = h->array[id];
	if (tracker == hn)
	{
		h->array[id] = tracker->next;
	}
	else
	{
		while (tracker->next != hn && tracker->next != NULL)
		{
			tracker = tracker->next;
		}

		if (tracker->next == NULL)
		{
			return 1;
		}

		// [prev] -> [node] -> [...]
		// tracker = prev
		// tracker->next = node

		tracker->next = tracker->next->next;
	}

	return 0;
}


fragment * frag_init(uint16_t offset, uint8_t *data, uint8_t MF, uint32_t acknum, uint32_t seqnum, uint16_t size, uint8_t *all)
{
	fragment *ret = (fragment *) malloc(sizeof(fragment));

	ret->forward 	= NULL;
	ret->backward 	= NULL;


	ret->data = NULL;
	if (data)
	{
		ret->data = (uint8_t *) malloc(size);
		memcpy(ret->data, data, size);
	}

	ret->all = NULL;
	if (all)
	{
		ret->all = (uint8_t *) malloc(size + 20); // IP header size
		memcpy(ret->all, all, size + 20);
	}


	ret->offset 	= offset;
	ret->MF		= MF;
	ret->acknum	= acknum;
	ret->seqnum	= seqnum;
	ret->psize	= size;

	return ret;
}


uint8_t add_frag(hashnode * hn, fragment *fr)
{	
	hn->received += fr->psize;

	if (!fr->MF)
	{
		hn->expected = missing(fr->offset + fr->psize, hn->expected);
	}

	if (hn->box == NULL)
	{
		hn->box = fr;
		return 0; 
	}


	fragment *tracker = hn->box;
	uint16_t current = hn->box->offset;

	// Find fragment position on the list
	while (tracker->forward != NULL && fr->offset > current)
	{
		tracker = tracker->forward;
		current = tracker->offset;
	}

	// Fragment must be before an existing one
	if (fr->offset < current)
	{
		fr->forward = tracker;
		fr->backward = tracker->backward;

		if (tracker->backward != NULL)
		{
			tracker->backward->forward = fr;
		}
		else
		{
			hn->box = fr;
		}

		tracker->backward = fr;
	}
	// Fragment must be after an existing one
	else
	{
		fr->forward 	= tracker->forward;
		fr->backward 	= tracker;

		if (tracker->forward != NULL)
		{
			tracker->forward->backward = fr;
		}

		tracker->forward = fr;
	}

	return (hn->expected > 0 && hn->received == hn->expected);
}


hashnode *search_hn(hash *h, uint16_t id, uint16_t source, uint16_t destin, uint16_t portin, uint16_t portout, uint8_t protocol)
{
	hashnode *tracker = h->array[id]; 
	
	while (tracker != NULL)
	{
		if ((source == tracker->source) && (destin == tracker->destin) && (protocol == tracker->protocol) && (tracker->portin == portin) && (tracker->portout == portout))
		{
			return tracker;
		}

		tracker = tracker->next;
	}
	return (hashnode *)NULL;
}

uint16_t missing(uint16_t lenght, uint16_t miss)
{
	return lenght - miss;
}

uint8_t * hash_obtain_package(hashnode *hn)
{
	uint8_t *ret = (uint8_t *) malloc(sizeof(uint8_t));
	uint32_t current = 0;

	fragment *fr = hn->box;

	while (fr)
	{	
		ret = realloc(ret, sizeof(uint8_t) * (fr->psize + current));
		memcpy((ret + current), fr->data, fr->psize);

		current += fr->psize;
		fr = fr->forward;
	}

	return ret;
}

uint8_t * hash_obtain_all(hashnode *hn, uint16_t initial_increment)
{
	uint8_t *ret = (uint8_t *) malloc(sizeof(uint8_t));
	uint32_t current = 0;
	uint32_t csize = 0;

	fragment *fr = hn->box;

	while (fr)
	{	
		csize = (!current)? fr->psize + initial_increment: fr->psize;

		ret = realloc(ret, sizeof(uint8_t) * (csize + current));
		memcpy((ret + current), fr->all, csize);

		current += csize;
		fr = fr->forward;
	}

	return ret;
}

uint8_t change_hashn(hash *h, hashnode * node, uint32_t to)
{
	if (!node || !h)
	{
		return 1;
	}

	// Find hashnode // Validation
	hashnode * tracker = h->array[node->id];
	while (tracker != node)
	{
		if (tracker == NULL)
		{
			return 1;
		}

		tracker = tracker->next;
	}

	// Alter hash
	if (pop_hashn(h, node))
	{
		return 1;
	}
	
	node->id = to;
	add_hashn(h, node);

	return 0;
}

void 	fragpos_set(hashnode * hn, uint32_t fragstart, uint32_t fragend)
{
	if (hn == NULL)
	{
		return ;
	}
	
	hn->fragstart 	= fragstart;
	hn->fragend	= fragend;
	
	return ;
}

uint8_t pop_frag(hashnode *hn, fragment *fr)
{
	fragment * tracker = hn->box;

	if (tracker == fr)
	{
		hn->box = fr->forward;
		return 0;
	}

	while (tracker->forward != fr && tracker != NULL)
	{
		tracker = tracker->forward;
	}
	
	if (tracker == NULL)
	{
		return 1;
	}

	tracker->forward = fr->forward;	

	return 0;
}

uint8_t clear_frag(hashnode * hn)
{
		fragment * frag_send = hn->box; 
		fragment * next = hn->box;
	
		while (next != NULL)
		{
			frag_send = next;
			pop_frag(hn, frag_send);
				
			next = frag_send->forward;

			free(frag_send->all);
			free(frag_send->data);
			free(frag_send);
		}

		return 0;
}
