#include "icmp.h"

icmp * icmp_init(uint8_t * payload)
{
	icmp * ret = (icmp *) malloc(sizeof(icmp));
	if (!ret)
	{
		return NULL;
	}

	ret->header = (icmphdr *) malloc(sizeof(icmphdr));
	if (!ret->header)
	{
		return NULL;
	}

	icmp_fill(ret, payload);

	return ret;
}

void icmp_fill(icmp * ret, uint8_t * payload)
{

	ret->protocol = 1;

	icmphdr * header = ret->header;

	header->type = payload[0];
	header->code = payload[1];

	header->checksum = from8to16(payload[2], payload[3]);
	header->id	 = from8to16(payload[4], payload[5]);
	header->seqnum	 = from8to16(payload[6], payload[7]);

	ret->msg = (payload + ICMPSZ);

	return ;
}

icmphdr * icmp_extract(uint8_t * payload)
{
	icmphdr * ret = (icmphdr *) payload;
	return ret;
}

uint8_t icmp_check(icmp * pack, uint32_t realsz)
{
	// Clean vector
	uint8_t * pseudo = (uint8_t *) calloc(sizeof(uint8_t), realsz);
	icmphdr * header = pack->header;

	if (realsz < ICMPSZ)
	{
		return 1;
	}

	// Continuous memory
	pseudo[0] = header->type;
	pseudo[1] = header->code;

	fill8from16(pseudo + 2, header->checksum);
	fill8from16(pseudo + 4, header->id);
	fill8from16(pseudo + 6, header->seqnum);

	memcpy(pseudo + (ICMPSZ), pack->msg, realsz - ICMPSZ);

	uint16_t ret = checksum(pseudo, realsz, 0);
	free(pseudo);

	return ret ? 1: 0;
}

uint8_t icmp_echo_requested(uint8_t * state, icmp * current, uint16_t * prev_seqnum)
{
	uint16_t seqnum = current->header->seqnum; 

	switch (current->header->type)
	{
		case 0: // Echo Reply
			// Request consumed
			*state &= ~ECHOREQ;
			return 1;	

			break;

		case 8: // Echo Request
			// Flag the request
			*state |= ECHOREQ;

			// Change the saved seqnum
			(*prev_seqnum) = seqnum; 

			return 1;

			break;

		default:
			return 0;
			break;
	}


	return 0;
}

void icmp_free(icmp * pack)
{
	free(pack->header);
	free(pack);

	return ;
}
