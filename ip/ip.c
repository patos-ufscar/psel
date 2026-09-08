#include "ip.h"

void ip_fill(ip *ret, uint8_t *payload)
{
	ret->type  	= (payload[0] & 0xF0) >> 4;
        ret->ihl   	= payload[0] & 0x0F;	
	ret->tos	= payload[1];

	ret->tot_lenght = from8to16(payload[2], payload[3]);
	ret->id         = from8to16(payload[4], payload[5]);

        uint16_t itlfrag   = from8to16(payload[6], payload[7]);

	// 0100 0000 0000 0000
	// 0100 0000
	ret->MF		= (payload[6] & 0x20) >> 5;
        ret->offset	= itlfrag & 0x1FFF;

	ret->ttl	= payload[8];
        ret->protocol   = payload[9];
	ret->checksum	= from8to16(payload[10], payload[11]);

	ret->from  	= from16to32(from8to16(payload[12], payload[13]), from8to16(payload[14], payload[15]));
        ret->to	  	= from16to32(from8to16(payload[16], payload[17]), from8to16(payload[18], payload[19]));
}

ip* ip_init(uint8_t *payload)
{
	ip *ret = (ip *) malloc(sizeof(ip));
	ip_fill(ret, payload);
	
	return ret;
}

ip* ip_extract(uint8_t *payload)
{
	ip *ret = (void *) payload;

	return ret;
}

uint8_t from16to8(uint16_t a)
{
	uint16_t ret = a >> 8;
	return (uint8_t) ret;
}

uint16_t from8to16(uint8_t a, uint8_t b)
{
	// AA e BB
	// AABB

        uint16_t ret = ((uint16_t) a << 8) | b;
        return ret;
}

uint32_t from16to32(uint16_t a, uint16_t b)
{
        uint32_t ret = (a << 16) | b;
        return ret;
}

void fill8from32(uint8_t *arr, uint32_t b)
{
	arr[0] = (uint8_t)(b >> 24);
	arr[1] = (uint8_t)(b >> 16);
	arr[2] = (uint8_t)(b >> 8);
	arr[3] = (uint8_t) b;
	
	return ;
}

void fill8from16(uint8_t *arr, uint16_t b)
{
	// AA BB initial
	// AA BB

	arr[0] = (uint8_t)(b >> 8);
	arr[1] = (uint8_t) b;

	return ;
}

uint8_t ipv4_check(uint8_t *payload)
{
	uint32_t sum = 0;

	for (uint8_t i = 0; i < 20; i += 2)
	{
		sum += from8to16(payload[i], payload[i + 1]);
	}

	uint16_t add = (uint16_t)(sum >> 16);
	while (add)
	{
		sum = sum & 0xFFFF;
		sum += add;
		add = (uint16_t)(sum >> 16);

	}

	sum = ~(sum) & 0xFFFF;
	return (!sum) ? 0: 1;
}

uint32_t sum16from32(uint32_t a)
{
	return (a >> 16 & 0xffff) + (a & 0xffff);
}

uint32_t sum16from8(uint8_t a, uint8_t b)
{
	return  (((uint16_t) a) << 8) + b;
}


void free_protocol(struct package *a)
{
	free(a->header);
	free(a);

	return ;
}

uint32_t endianness32(uint32_t a)
{
	uint32_t ret = 0;
	
	// AA BB CC DD
	// DD CC BB AA

	
	ret = ((a & 0xFF) << 24) | ((a & 0xFF00) << 8) | ((a & 0xFF0000) >> 8) | ((a & 0xFF000000) >> 24);

	return ret;
}

uint16_t endianness16(uint16_t a)
{

	uint16_t ret = 0;

	ret = ((a & 0xff00) >> 8) | ((a & 0xff) << 8);

	return ret;
}



uint16_t checksum(uint8_t *data, uint16_t lenght, uint32_t start)
{
	uint8_t odd = 0;
		
	// Number of bytes is Odd
	if (lenght & 0x1)
	{
		odd = 1;
		start += (uint32_t)(data[lenght - 1] << 8);
	}
	
	// Package + Message
	for (uint32_t i = 0; i < (lenght - odd); i = i + 2)
	{
		start += sum16from8(data[i], data[i + 1]);
	}	

	// Overflow
	uint16_t add = (uint16_t) (start >> 16);
	while (add)
	{
		start &= 0xffff;
		start += add;
		add =  (uint16_t) (start >> 16);
	}

	return (~(start & 0xFFFF));

}

void ip_cpy(ip * copied, ip * original)
{
	copied->ihl 	= original->ihl;
	copied->type 	= original->type;
	copied->tot_lenght = original->tot_lenght;
	copied->id 	= original->id;
	copied->MF 	= original->MF;
	copied->offset 	= original->offset;
	copied->ttl 	= original->ttl;
	copied->protocol = original->protocol;
	copied->checksum = original->checksum;
	copied->from 	= original->from;
	copied->to 	= original->to;

	return ;
}

void show_ip(uint32_t uip)
{
	printf("%hhu.%hhu.%hhu.%hhu\n", uip >> 24, uip >> 16, uip >> 8, uip);

	return ;
}
