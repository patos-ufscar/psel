#include "udp.h"

udp * set_udp(uint8_t *payload)
{
	udp *ret = (udp *) malloc(sizeof(udp));
	if (!ret)
	{
		return NULL;
	}

	ret->header = (udphdr *) malloc(sizeof(udphdr));
	if (!ret->header)
	{
		return NULL;
	}

	//Campos de 2 bytes   Campo de 1 byte
	ret->header->source = from8to16(payload[0], payload[1]);
	ret->header->destin = from8to16(payload[2], payload[3]);
	ret->header->lenght = from8to16(payload[4], payload[5]); 
	ret->header->checksum = from8to16(payload[6], payload[7]);

	ret->msg = &payload[8];

	return ret;
}

udphdr * udp_extract(uint8_t * payload)
{
	udphdr * ret = (udphdr *)payload;
	return ret;
}

uint8_t udp_check(udp *package, uint32_t source, uint32_t destin, uint32_t realsz)
{
	udphdr *header 		= package->header;
	uint8_t *msg		= package->msg;
	uint32_t lenght		= 0;

	if (header->lenght > realsz)
	{
		printf("Lenght: %hd > %hd\n", header->lenght, realsz);
		return 1;
	}

	uint8_t odd = 0;
	lenght += header->lenght;

	if (lenght % 2 == 1)
	{
		lenght += 1;
		odd = 1;
	}
	lenght += 12; // ips, 0x00, prot, upd, msg lenght

	uint16_t ret;

	uint8_t *pseudo = (uint8_t *) malloc(lenght * sizeof(uint8_t));
	memset(pseudo, 0, lenght * sizeof(uint8_t));

	fill8from32(pseudo, source);
	fill8from32((pseudo + 4), destin);

	pseudo[8] = 0x00; 
	pseudo[9] = 0x11;

	fill8from16((pseudo + 10), header->lenght);

	fill8from16((pseudo + 12), header->source);
	fill8from16((pseudo + 14), header->destin);
	fill8from16((pseudo + 16), header->lenght);
	fill8from16((pseudo + 18), header->checksum);

	uint32_t ovfw = header->lenght * sizeof(uint8_t ) - 8;
	memcpy((pseudo + 20), msg, ovfw);

	if (odd)
	{
		pseudo[lenght - 1] = 0x00;
	}
	
	ret = ~(udp_checksum(lenght, pseudo));

	free(pseudo);

	return (!ret) ? 0: 1;
}

uint16_t udp_checksum(uint32_t lenght,  uint8_t *msg)
{
	uint32_t sum = 0;

	for (uint32_t i = 0; i < lenght; i += 2)
	{
		sum += from8to16(msg[i], msg[i + 1]);
	}

	uint16_t add = (uint16_t)(sum >> 16);

	while (add)
	{
		sum = sum & 0xFFFF;
		sum += add;
		add = (uint16_t)(sum >> 16);

	}

	return (uint16_t)(sum & 0xFFFF);
}

void udp_free(udp *pack)
{
	free(pack->header);
	free(pack);
	return ;
}
