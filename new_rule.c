#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <errno.h>

#pragma pack(1)

#define SOCKPATH "rules.sock"

#define ADD	0x0
#define REM	0x1
#define STR	0x2

#define IN 	1
#define OUT 	2
#define BOTH	3

typedef struct rules{
	uint8_t op;
	uint8_t lim;
	uint8_t ways;
	uint32_t data; // 56 bits = 7 bytes 
} rules;


int32_t main(int argc, char *argv[])
{
	if (argc < 5)
	{
		printf("USAGE:\n./rule <op> <type> <newrule> <in/out>");
		return 0;
	}

	char *op	= argv[1];
	char *type 	= argv[2];
	char *block 	= argv[3];
	char *io	= argv[4];
	rules info;
	memset(&info, 0, sizeof(struct rules));

	void *package = &info;
	uint8_t word = 0;


	if (!strcmp(op, "add") || !strcmp(op, "ADD"))
	{
		info.op = 0;
	}
	else if (!strcmp(op, "rem") || !strcmp(op, "REM"))
	{
		info.op = 1;
	}
	else
	{
		printf("<op> can be either ADD or REM\n");
	}

	if (!strcmp(io, "out") || !strcmp(io, "OUT"))
	{
		info.ways = OUT;
	}
	else if (!strcmp(io, "in") || !strcmp(io, "IN"))
	{
		info.ways = IN;
	}
	else if (!strcmp(io, "both") || !strcmp(io, "BOTH"))
	{
		info.ways = BOTH;
	}
	else
	{
		printf("<in/out> can be either IN or OUT or BOTH\n");
	}

	// IP


	if (!strcmp(type, "ip") || !strcmp(type, "IP"))
	{
		if (strlen(block) > 19)
		{
			printf("IP must be 19 chars or lower.\n");
			return 1;
		}	
		char *limit = (char *) malloc(sizeof(char) * 20); // IP maximum lenght + NULL char
		memcpy(limit, block, strlen(block) + 1);


		limit = strtok(limit, "/");

		info.lim = atoi(strtok(NULL, "/"));

		block = strtok(block, ".");
		info.data |= atoi(block) << 24;

		for (uint8_t i = 3; i && block; i--)
		{
			block = strtok(NULL, ".");
			info.data |= atoi(block) << ((i - 1) * 8);
		}

		free(limit);
	}
	else if (!strcmp(type, "port") || !strcmp(type, "PORT"))
	{
		info.data = atoi(block);
	}
	else if (!strcmp(type, "word") || !strcmp(type, "WORD"))
	{
		info.op |= STR;
		
		uint32_t lenght = strlen(block) + 1; // Word/size + \0
		if (lenght > 511)
		{
			printf("Word must be of size < 510 chars + \\0\n");
			return 1;
		}

		info.lim = lenght;
		package = (char *) malloc(sizeof(char) * (lenght + sizeof(struct rules)));

		memcpy(package, &info, sizeof(struct rules));
		memcpy((package + sizeof(struct rules)), block, lenght);

		word = 1;
	}
	else
	{
		printf("<type> can be either IP, PORT or WORD\n");
		return 0;
	}



	// Send DGRAM
	uint32_t sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
		return 1;
	}

	struct sockaddr_un destin;

	memset(&destin, 0, sizeof(destin));
	destin.sun_family = AF_UNIX;
	strncpy(destin.sun_path, SOCKPATH, sizeof(destin.sun_path));
		

	socklen_t lenght = sizeof(destin);

	printf("pack size:%ld\n", sizeof(struct rules) + info.lim);
	printf("struct size: %ld\n", sizeof(struct rules));

	if (sendto(sockfd, (void *) package, sizeof(struct rules) + info.lim , 0, (struct sockaddr *) &destin, lenght) < 0)
	{
		printf("%s\n", strerror(errno));
		printf("PACKAGE NOT SENT.\n");
	}
	else
	{
		printf("PACKAGE SENT.\n");
		printf("DATA: lim=%hhu, data=%d, ways=%d\n", info.lim, info.data, info.ways);
	}

	if (word)
	{
		printf("%s: %d\n", (char *)package + sizeof(struct rules), info.lim);
		free(package);
	}

	close(sockfd);
	return 0;
}
