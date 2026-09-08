#include "tcp.h"

tcp * set_tcp(uint8_t *payload)
{
	tcp *ret = (tcp *) malloc(sizeof(tcp));
	ret->header = (tcphdr *) malloc(sizeof(tcphdr));
	
	ret->header->source = from8to16(payload[0], payload[1]);
	ret->header->destin = from8to16(payload[2], payload[3]);

	ret->header->seqnum = from16to32(from8to16(payload[4], payload[5]), from8to16(payload[6], payload[7]));
	ret->header->acknum = from16to32(from8to16(payload[8], payload[9]), from8to16(payload[10], payload[11]));

	ret->header->offset = 	(payload[12] >> 4) & 0x0F;
	ret->header->reserved = (payload[12] >> 1) & 0x07; // 0000 0111
	ret->header->flags  = 	(uint16_t)(payload[12] & 0x01) << 8| payload[13]; // 1000 0000

	ret->header->winsiz = from8to16(payload[14], payload[15]);
	ret->header->checksum = from8to16(payload[16], payload[17]);
	ret->header->urgptr = from8to16(payload[18], payload[19]);
	
	int16_t optsiz = (ret->header->offset * 4) - 20;
	if (optsiz > 0)
	{
		ret->header->options = (uint8_t *) malloc(sizeof(uint8_t) * optsiz); // Options are inverted
		memcpy(ret->header->options, &payload[20], optsiz);
	}

	else
	{
		ret->header->options = NULL;
	}
	
	ret->msg = &payload[ret->header->offset * 4];

	return ret;
}

uint8_t set_optflags(tcp *pack, uint8_t options)
{
	int16_t optsize = (pack->header->offset * 4) - 20;
	if (optsize <= 0)
	{
		return 1;
	}

	// T L V
	uint8_t byte;

	uint16_t tsize = 0;
	uint8_t type = 0;
	uint8_t size = 0;
	uint8_t value[40];

	for (uint16_t i = 0; i < optsize; i++)
	{	
		byte = pack->header->options[i];
		if (!byte)
		{
			break;
		}
		else if (type == 0x1)
		{
			continue;
		}

		type = !type? byte: type;
		if (type && !size)
		{
			size = byte;
			tsize += size;

			if (tsize > optsize)
			{
				return 1;
			}

			if (size - 2 <= 0 || !(type & options))
			{
				continue ;
			}

			memcpy(value, pack->header->options + i + 1, size - 2);
				

			type = 0;
			size = 0;
			i += size - 1; 
		}

		

		

	}


	return 0;
}

tcp * tcp_extract(uint8_t *payload)
{
	tcp * ret = (tcp *) malloc(sizeof(tcp));

	ret->protocol 	= 6;
	ret->header	= (tcphdr *) payload;
	ret->msg	= &payload[ret->header->offset * 4];

	return ret;
}

tcphdr * tcph_extract(uint8_t *payload)
{
	tcphdr * ret = (void *)payload;

	return ret;
}

uint16_t tcp_check(tcp *pack, ip *info, uint16_t check)
{
	tcphdr *header = pack->header;

	uint16_t flags = (header->offset << 12) | (header->reserved << 9)| header->flags;

	// Fake IP header
	uint32_t sz  = info->tot_lenght - ((uint32_t) info->ihl << 2);

	// Continuous mem
	uint16_t ptroff = 20;
	uint8_t *pseudo = (uint8_t *) malloc(sz);
	memset(pseudo, 0, sz);

	// Starting Value
	uint32_t ret = 0;
	ret += sum16from32((info->from));
	ret += sum16from32((info->to));
	ret += 0x0006;
	ret += (sz);
	
	// Fill buffer
	fill8from16(pseudo, header->source);
	fill8from16(pseudo + 2, header->destin);
	fill8from32(pseudo + 4, header->seqnum);
	fill8from32(pseudo + 8, header->acknum);
	fill8from16(pseudo + 12, flags);
	fill8from16(pseudo + 14, header->winsiz);
	fill8from16(pseudo + 16, check); 
	fill8from16(pseudo + 18, header->urgptr);
	
	if (header->offset * 4 - 20 > 0)
	{
		memcpy(pseudo + 20, header->options, (header->offset * 4 - 20));
		ptroff = header->offset * 4;
	}
	if (sz - ptroff > 0)
	{
		memcpy(pseudo + ptroff, pack->msg, sz - ptroff);
	}
	ret = checksum(pseudo, sz, ret);

	free(pseudo);
	return ret;
}

uint16_t tcp_checksum(uint8_t *msg, uint16_t lenght)
{
	return 0;
}


uint8_t tcp_connect(tcp *pack, uint8_t *tw_stage, uint32_t seqnum, ip *info)
{
	uint8_t flags = pack->header->flags;
	uint8_t stage = *tw_stage;
	uint32_t payload_size = (info->tot_lenght - (info->ihl + pack->header->offset) * 4);

	// Begining of connection
	if (flags == SYN)
	{
		if (!stage)
		{
			*tw_stage = SYN | ACK;
			uint32_t rand = unix_random();
			if (!rand)
			{
				return 1;
			}

			pack->header->seqnum = rand;
			pack->header->acknum = seqnum + 1;

			pack->header->winsiz = WINSIZE;
		

			ip send_info;
			ip_cpy(&send_info, info);

			send_info.from  = info->to;
			send_info.to	= info->from;
			send_info.ihl	= 5;
			send_info.tot_lenght = 40;

			pack->header->flags = *tw_stage;
			pack->header->offset = 5;

			pack->header->checksum = tcp_check(pack, &send_info, 0);

			return CONNECT;
		}

		// Connection Failed, try again.
		else if (pack->header->seqnum == seqnum)
		{
			*tw_stage = 0;
			return tcp_connect(pack, tw_stage, seqnum, info);
		}
		else
		{
			return 1;
		}
	}

	// Acknowledge TWH
	else if (flags == ACK && (stage == (SYN | ACK)))
	{
		if (payload_size <= 0)
		{
			printf("TCP CONNECTED\n");
			*tw_stage = ACK;
			return IGNORE;
		}
	}

	// Three-Way-Handshake is done 
	else if ((flags == ACK || flags == (PSH | ACK)) && (stage == ACK))
	{
		if (payload_size <= 0) 
		{
			return 1;
		}	


		// Data will arrive now	
		uint32_t acknum = pack->header->acknum;
		pack->header->acknum = pack->header->seqnum + payload_size;
		pack->header->seqnum = acknum;

		*tw_stage = ACK;
		ip send_info;
		ip_cpy(&send_info, info);

		send_info.from  = info->to;
		send_info.to	= info->from;
		send_info.ihl	= 5;
		send_info.tot_lenght = 40;

		pack->header->flags = *tw_stage;
		pack->header->offset = 5;

		pack->header->checksum = tcp_check(pack, &send_info, 0);

		return RECEIVE;
	}

	// End Connection
	else if (flags & FIN)
	{
		// Make sure the connection has been opened
		if (stage & ACK)
		{

			*tw_stage = FIN | ACK;

			uint32_t acknum = pack->header->acknum;

			pack->header->acknum = pack->header->seqnum + 1;
			pack->header->seqnum = acknum;

			ip send_info;
			ip_cpy(&send_info, info);

			send_info.from  = info->to;
			send_info.to	= info->from;
			send_info.ihl	= 5;
			send_info.tot_lenght = 40;

			pack->header->flags = *tw_stage;
			pack->header->offset = 5;

			pack->header->checksum = tcp_check(pack, &send_info, 0);
			return STARTEND;
		}

		else 
		{
			return 1;
		}
	}
	
	// Client has received FIN | ACK
	if (flags == ACK  && stage == (FIN | ACK))
	{
		printf("TCP ENDED\n");
		return END;
	}

	return 1;
}


void tcp_free(tcp *pack)
{
	if ((pack->header->offset * 4 - 20) > 0)
	{
		free(pack->header->options);
	}
	free(pack->header);
	free(pack);

	return ;
}

uint32_t unix_random()
{
	uint32_t ret;
	int32_t randfd = open("/dev/urandom", 0, O_RDONLY);
	if (randfd < 0)
	{
		return 0;
	}

	if (read(randfd, &ret, 4) != 4)
	{
		return 0;
	}

	close(randfd);
	return ret;
}
