// Consertar PING
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <stdint.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/epoll.h>

#include <pthread.h>
#include "stack/stack.h"
#include "queue/queue.h"
#include "hash/hash.h"
#include "ip/ip.h"
#include "udp/udp.h"
#include "tcp/tcp.h"
#include "icmp/icmp.h"
#include "trie/trie.h"
#include "linkedlist/linked.h"

#pragma pack(1)

#define MTU	1500
#define STATELESS	"statrules.bin"
	#define FWORDS	  	"words.bin"
#define SOCKPATH  	"rules.sock"

// RET values

#define MEMERR		1
#define SYSERR		2

#define INVALID		3
#define FRAGMENT	2
#define CORRUPT		1
#define ACCEPTED	0

#define NTHREADS	5
#define THREADRL	1
#define THREADPK	NTHREADS - THREADRL
#define RULETYPE	2
#define MAXRULESWD	100

#define TCP_OPTIONS	0x11

typedef struct rules{
	uint8_t op;
	uint8_t lim;
	uint8_t ways;
	uint32_t data;
} rules; // 56 bits, 7 bytes



uint32_t set_TUN();
void load_stateless();
uint32_t setup_socket();
uint32_t setup_exit();
void save_rules();

uint8_t  validate_package_thread(uint8_t *payload);
void * start_worker(void *arg);
void * start_worker_rule(void *arg);

uint8_t check_stateless(void *pack, uint8_t protocol);
uint8_t check_forbidden(uint8_t *msg, uint16_t lenght, lklist * rules);

void package_denied();
void send_ahead(uint8_t *pack, uint32_t destin, uint32_t tsize, struct sockaddr_in *addr);


void see_package(uint8_t *msg, uint32_t size);
void decode(udp *pack);
void turn_end(int32_t sig);
uint8_t nodata(uint8_t *head);
uint32_t max(uint32_t a, uint32_t b);

// Global

/* MUTEXES */
pthread_mutex_t hash_rw = PTHREAD_MUTEX_INITIALIZER; 
pthread_mutex_t q_write = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_rule = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t send_w	= PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t	wake	= PTHREAD_COND_INITIALIZER;
pthread_cond_t 	rwake	= PTHREAD_COND_INITIALIZER;

pthread_cond_t mwait 	= PTHREAD_COND_INITIALIZER;
queue *tr_queue	= NULL;

/* FRAG */
hash *fr_hash 	= NULL;

/* RULES */
trtree * port_rules 	= NULL;
trtree * ip_rules	= NULL;
lklist * word_rules	= NULL;

uint32_t tunfd;
uint32_t exitfd;
uint8_t end;


int main(int argc, char *argv[])
{
	uint32_t ret;
	uint8_t  err;
	
	//Package Volume Control
	end = 0;
	struct sigaction sig;
	sig.sa_handler = turn_end;
	sigaction(SIGUSR1, &sig, NULL);

	// Thread Control
	pthread_t tid[NTHREADS];
	tr_queue = queue_init(200, MTU); // Valor aleatório	
	if (!tr_queue)
	{
		printf("Failed to allocate: queue.\n");
		return MEMERR;
	}
					 

	uint32_t k;
	for (k = 0; k < THREADPK; k++)
	{
		if (pthread_create(&tid[k], NULL, (void *) start_worker, (void *) tr_queue))
		{
			printf("thread initialization failed: worker\nMay cause errors.\n");
		}
	}

	for (uint32_t i = 0; i < THREADRL; i++, k++)
	{
		if (pthread_create(&tid[k], NULL, (void *)start_worker_rule, (void *)tr_queue))
		{
			printf("thread initialization failed: rules\nMay cause errors.\n");
		}
	}


	// Fragmentation Control
	fr_hash = hash_init(65536); // 16 bit max (IP ID)
	if (!fr_hash)
	{
		printf("Failed to allocate: hash.\n");
		return MEMERR;
	}

	// Load Rules 
	port_rules 	= trtree_init();
	ip_rules	= trtree_init();
	word_rules	= lklist_init();
	load_stateless();

	uint32_t sockfd = setup_socket();
	if (!sockfd)
	{
		printf("System error: socket create\n");
		return SYSERR;
	}

	// SEND
	exitfd = setup_exit();

	// RECEIVE
	tunfd = set_TUN();
	if (tunfd < 0)
	{
		printf("System error: TUN create\n");
		return SYSERR;
	}

	// Configure TUN
	err = system("sudo ip link set dev tun0 up");
	if (err == 127 || err < 0)
	{
		printf("System error: configure TUN\n");
		return SYSERR;
	}

	err = system("sudo ip route add 10.0.0.0/24 dev tun0");
	if (err == 127 || err < 0)
	{
		printf("System error: configure TUN\n");
		return SYSERR;
	}

	// Configure epoll

	uint32_t epollfd = epoll_create(1);
	if (epollfd < 0)
	{
		printf("System error: epoll create\n");
		return SYSERR;
	}

	struct epoll_event event;

	event.events = EPOLLIN;
	event.data.fd = tunfd;	
	epoll_ctl(epollfd, EPOLL_CTL_ADD, tunfd, &event);

	event.data.fd = sockfd;
	epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &event);

	struct epoll_event event_list[4];
	
	pthread_mutex_lock(&q_write);
	uint8_t *addr = off_enq(tr_queue);
	pthread_mutex_unlock(&q_write);

	while (!end) // kill 10
	{

		int16_t nevents = epoll_wait(epollfd, event_list, 4, -1);
		for (uint8_t i = 0; i < nevents; i++)
		{
			int16_t up = read(event_list[i].data.fd, (void *) addr, MTU);

			printf("%02x\n", *addr); // DEBUG
			if (up < 0 || (nodata(addr) && event_list[i].data.fd != sockfd))
			{
				continue;
			}

			pthread_mutex_lock(&q_write);
			set_tail(tr_queue);

			if (event_list[i].data.fd == sockfd)
			{
				pthread_cond_signal(&rwake);
			}
			else
			{
				pthread_cond_signal(&wake);
			}
			pthread_mutex_unlock(&q_write);

			// New addr
			pthread_mutex_lock(&q_write);	
			addr = off_enq(tr_queue);
			if (!addr)
			{
				// queue maximum capacity
				while (tr_queue->size == tr_queue->max - 1)
				{
					pthread_cond_wait(&mwait, &q_write);
				}
				pthread_mutex_unlock(&q_write);
			}
			pthread_mutex_unlock(&q_write);
		}
	}	



	// RULES
	
	// Stateless
		// IP
		// Ports
		// Protocol
		// Flags
		
		

	// RESULT
	



	printf("EXITING.\n");

	// Kill threads
	for (uint32_t i = 0; i < THREADPK; i++)
	{
		pthread_mutex_lock(&q_write);
		pthread_cond_signal(&wake);
		pthread_mutex_unlock(&q_write);
	}

	for (uint32_t i = 0; i < THREADRL; i++)
	{
		pthread_mutex_lock(&q_write);
		pthread_cond_signal(&rwake);
		pthread_mutex_unlock(&q_write);
	}

	// Wait for threads to exit
	for (uint32_t i = 0; i < NTHREADS; i++)
	{
		pthread_join(tid[i], (void **) &ret);
	}
	
	// Save applied rules
	save_rules();

	// Cleanup
	free_queue(tr_queue);
	free_hash(fr_hash);	

	tr_free(ip_rules->root);
	free(ip_rules);

	tr_free(port_rules->root);
	free(port_rules);

	lklist_print(word_rules);
	lklist_free(word_rules);
	free(word_rules);


	close(exitfd);
	close(epollfd);
	close(tunfd);
	close(sockfd);

	printf("EXITED.\n");
	return 0;
}

uint32_t set_TUN()
{
	uint32_t fd = open("/dev/net/tun", O_RDWR);
	if (fd == -1)
	{
		perror("Open error.\n");
		return -1;
	}

	struct ifreq ifr;

	memset(&ifr, 0, sizeof(struct ifreq));

	strncpy(ifr.ifr_name, "tun0", IFNAMSIZ);
	
	ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

	uint32_t err = ioctl(fd, TUNSETIFF, (void *) &ifr);

	if (err < 0)
	{
		perror("ioctl error.\n");
		close(fd);
		return -1;
	}

	return fd;
}

void *start_worker_rule(void *arg)
{
	queue *q = arg;
	uint8_t *copy = (uint8_t *) calloc(sizeof(struct rules) + 511, sizeof(uint8_t));

	while (!end)
	{
		pthread_mutex_lock(&q_write);
		while (q->size == 0 && !end)
		{
			pthread_cond_wait(&rwake, &q_write);
		}

		if (end)
		{
			pthread_mutex_unlock(&q_write);
			break;
		}
		
		uint8_t *package = dequeue(q);

		if (!package)
		{
			pthread_mutex_unlock(&q_write);
			continue ;
		}

		memcpy(copy, package, sizeof(rules) + 511);
		pthread_mutex_unlock(&q_write);

		rules *nr = (rules*) (copy);
		// printf("lim: %hhu, ways: %hhd, data: %d\n", nr->lim, nr->ways, nr->data); // DEBUG

		// Action
	
		pthread_mutex_lock(&mx_rule); // Rules Mutex	
		if (nr->op & 0x2) // 0x2 Stands for STR  
		{
			char * str = (char *) malloc(sizeof(char) * nr->lim);
			memcpy(str, (copy + sizeof(struct rules)),  nr->lim);

			lknode * node;
			if (nr->op & 0x1)
			{	
				node = lk_search(word_rules, str);
				if (lk_remove(word_rules, node))
				{
					printf("Unable to remove word rule.\n");
					free(str);
				}
				else
				{
					free(node->key);
					free(node);
				}
			}

			else
			{
				// With NULL char
				node = lknode_init(str, nr->lim, nr->ways);
				lk_insert(word_rules, node);

				printf("NEW WORD RULE: %s\n", str);
			}
		}
		else if (nr->op)
		{
			if (!nr->lim)
			{
				tr_remove(port_rules, nr->data, 16, nr->ways, 16);
			}
			else
			{
				tr_remove(ip_rules, nr->data, nr->lim, nr->ways, 32);
			}
		}
		else
		{

			if (!nr->lim)
			{
				if (tr_insert(port_rules, nr->data, 16, nr->ways, 16))
				{
					printf("RULE ADD FAILED.\n");
				}
				else
				{
					printf("NEW PORT RULE: %d\n", nr->data);
				}
			}
			else
			{
				if (tr_insert(ip_rules, nr->data, nr->lim, nr->ways, 32))
				{
					printf("RULE ADD FAILED.\n");
				}
				else
				{
					printf("NEW IP RULE: %hhd.%hhd.%hhd.%hhd/%hd (%b)\n", nr->data >> 24, nr->data >> 16, nr->data >> 8, nr->data, nr->lim, nr->data);
				}
			}
		}
		pthread_mutex_unlock(&mx_rule);
		
	}

	free(copy);
	pthread_exit(NULL);
}

void *start_worker(void *arg)
{
	queue *q = arg;
	uint8_t *copy = (uint8_t *) calloc(MTU, sizeof(uint8_t));
	uint8_t ret = 0;

	while (!end)
	{
		pthread_mutex_lock(&q_write);
		while (q->size == 0 && !end)
		{
			pthread_cond_wait(&wake, &q_write);
		}

		if (end)
		{ 
			pthread_mutex_unlock(&q_write);
			break;
		}

		uint8_t * package = dequeue(q);
		if (package == NULL)
		{
			printf("DISCARTED.\n");

			pthread_mutex_unlock(&q_write);
			continue;
		}

		memcpy(copy, package, MTU); // Prevenir corrupção (caso o input seja muito maior que o output)
		pthread_mutex_unlock(&q_write);

		ret = validate_package_thread(copy);
		if (ret == ACCEPTED)
		{
			printf("PACKAGE ACCEPTED.\n");
		}
		else if (ret == INVALID)
		{
			printf("INVALID PROTOCOL.\n");
		}
		else if (ret == CORRUPT)
		{
			printf("PACKAGE DENIED.\n");
			// package_denied();
			// tcp_close_connection();
		}	
		else if (ret == FRAGMENT)
		{
			printf("FRAGMENT DETECTED.\n");
		}
		else if (ret == IGNORE)
		{
			continue;
		}
	}

	free(copy);
	pthread_exit(NULL);
}




/* Thread Funcion */
uint8_t validate_package_thread(uint8_t *payload)
{

	uint8_t last = 0, frag = 0, ret = ACCEPTED, state = 1;

	hashnode *target = NULL;
	fragment *fr;
	uint8_t * package_done = payload;


	// Generic structure for UDP/TCP packages
	struct package *pack; // Package behind IP protocol

	ip *ipp = ip_init(payload);
	uint8_t *package_start = payload + (ipp->ihl * 4);
	uint32_t realsz = ipp->tot_lenght - (ipp->ihl * 4), msglen = 0;

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;

	// IP validation
	if (ipp->type == 4)
	{
		if (ipv4_check(payload) || check_stateless((void *) ipp, 0))
		{
			free(ipp);
			return CORRUPT;
		}
	}
	else
	{
		free(ipp);
		return INVALID;
	}


	// Houve fragmentação
	if (ipp->MF || ipp->offset > 0)
	{
		uint16_t portin 	= 0;
		uint16_t portout 	= 0;
		uint16_t seqnum		= 0;
		uint16_t id		= ipp->id; // To handle ICMP fragmentation followed by an echo reply 

		pthread_mutex_lock(&hash_rw);
		target = search_hn(fr_hash, id, ipp->from, ipp->to, portin, portout, ipp->protocol);
	
		if (target == NULL)
		{
			target = hashn_init(id, ipp->from, ipp->to, portin, portout, ipp->protocol, 0);
			add_hashn(fr_hash, target);
		}

		fr = frag_init(ipp->offset * 8, package_start, (uint8_t) ipp->MF, 0, seqnum, ipp->tot_lenght - (ipp->ihl * 4), payload);

		last = add_frag(target, fr);
		
		pthread_mutex_unlock(&hash_rw);

		// Missing at least one package
		if (last)
		{
			package_start = hash_obtain_package(target);
			frag = 1;
			realsz = target->expected;

			// Exceptions
			if (ipp->protocol == 1) // ICMP
			{
				pthread_mutex_lock(&hash_rw);
				icmphdr * hdr = icmp_extract(package_start);
				change_hashn(fr_hash, target, endianness16(hdr->id));
				target->box->seqnum = endianness16(hdr->seqnum) - 1; // Big Endian, -1 to match the first case
				pthread_mutex_unlock(&hash_rw);
			}
		}
		else
		{
			free(ipp);
			return FRAGMENT;
		}
	}
	
	// Package Protocol Validation
	if (ipp->protocol == 17) /* UDP */
	{
		pack = (struct package*) set_udp(package_start);
		if (!pack)
		{
			free(ipp);
			return CORRUPT;
		}

		udp *upack = (udp *) pack;
		if (udp_check(upack, ipp->from, ipp->to, realsz))
		{
			free(ipp);
			udp_free((udp *)pack);

			return CORRUPT;
		}
		
		in_port_t port = upack->header->destin;
		addr.sin_addr.s_addr = ipp->to;
		addr.sin_port = port;

		msglen = upack->header->lenght - 8; // msg size - header size
	}
	else if (ipp->protocol == 6) /* TCP */
	{
		pack = (struct package*) set_tcp(package_start);
		if (!pack)
		{
			free(ipp);
			return CORRUPT;
		}

		tcp * tpack = (tcp *)pack;
		if (tcp_check(tpack, ipp, tpack->header->checksum))
		{
			free(ipp);
			tcp_free((tcp *)pack);
			return CORRUPT;
		}

		pthread_mutex_lock(&hash_rw);
		target = search_hn(fr_hash, 0, ipp->from, ipp->to, tpack->header->source, tpack->header->destin, ipp->protocol);

		if (!target)
		{
			target = hashn_init(0, ipp->from, ipp->to, tpack->header->source, tpack->header->destin, ipp->protocol, 0);
			add_hashn(fr_hash, target);

			fr = frag_init(0, package_start, 0, tpack->header->acknum, tpack->header->seqnum, ipp->tot_lenght - ipp->ihl, payload); // Save the first TCP header
			add_frag(target, fr);
		}	
		pthread_mutex_unlock(&hash_rw);

		// Connection Stage
		state = tcp_connect(tpack, &target->tw_stage, target->box->seqnum, ipp);

		if (state == CORRUPT || state == IGNORE)
		{
			free(ipp);
			tcp_free((tcp *)pack);
			return state;
		}
		msglen = ipp->tot_lenght - (tpack->header->offset + ipp->ihl) * 4;
	}
	
	
	else if (ipp->protocol == 1) /* ICMP */
	{
		pack = (struct package *) icmp_init(package_start); 
		if (!pack)
		{
			free(ipp);
			return CORRUPT;
		}
	
		icmp * ipack = (icmp *)pack;
		if (icmp_check(ipack, realsz))
		{
			free(ipp);
			free_protocol((struct package *) ipack);
			return CORRUPT;
		}

		pthread_mutex_lock(&hash_rw);
		hashnode * req = search_hn(fr_hash, ipack->header->id, ipp->from, ipp->to, 0, 0, 1);

		if (!req) // Either it's the first package (without fragmentation) or it's wrong
		{
			if (ipack->header->type == ECHOREQ)
			{
				// Create hash entry with ICMP ID in IPV4 ID 
				req = hashn_init(ipack->header->id, ipp->from, ipp->to, 0, 0, 1, ECHOREQ);
				add_hashn(fr_hash, req);
				
				// Initializes seqnumber 
				fragment *fr = frag_init(0, package_start, 1, 0, ipack->header->seqnum - 1, realsz, payload);
				add_frag(req, fr);
			}
			else
			{
				pthread_mutex_unlock(&hash_rw);
				free(ipp);
				free_protocol((struct package *)ipack);
				return CORRUPT;
			}
		}

		// Checks if it's allowed to send/receive package
		if (!icmp_echo_requested(&req->tw_stage, ipack, (uint16_t *) &req->box->seqnum))
		{
			pthread_mutex_unlock(&hash_rw);
			free(ipp);
			free_protocol((struct package *) ipack);
			return CORRUPT;
		}

		// Build ECHO REPLY Package
		// Can be altered freely
		hashnode * sender = search_hn(fr_hash, ipack->header->id, ipp->to, ipp->from, 0, 0, 1);
		if (sender == NULL)
		{
			// Create new hashnode to avoid future consequences
			sender = hashn_init(ipack->header->id, ipp->to, ipp->from, 0, 0, 1, ECHOREP);
			add_hashn(fr_hash, sender);
		}

		// Suport to fragmentation
		fragment * original = (target != NULL) ? target->box: req->box; // Depends if there's fragmentation
		fragment * newfrag;

		// Clear sender fragments
		pthread_mutex_lock(&send_w);
		clear_frag(sender);	

		// Fill sender fragments
		uint8_t * all;
		while (original != NULL)
		{
			// frag_init function copies all the buffers (data && all)
			newfrag = frag_init(original->offset, original->data, original->MF, original->acknum, original->seqnum, original->psize, original->all);
			add_frag(sender, newfrag);

			all = newfrag->all;

			// Faking IP Header
			ip * faked_ip = ip_extract(all);
			
			uint32_t destin = faked_ip->from;
			fill8from32(all + 12, endianness32(faked_ip->to));
			fill8from32(all + 16, endianness32(destin));

			uint8_t ihl 		= faked_ip->ihl * 4;
			uint16_t tot_lenght 	= endianness16(faked_ip->tot_lenght);

			faked_ip->checksum = 0;	
			faked_ip->checksum = endianness16(checksum(all, ihl, 0));

			// Faking ICMP Header (offset == 0)
			if (!newfrag->offset)
			{
				icmphdr * faked_icmp 	= icmp_extract(all + ihl);
				faked_icmp->type 	= 0;
				faked_icmp->seqnum	= endianness16(original->seqnum); 

				// Checksum must relly on all fragments
				faked_icmp->checksum	= 0;
				faked_icmp->checksum	= endianness16(checksum(package_start + ihl, frag? target->received - ihl: tot_lenght - ihl, 0));
				addr.sin_addr.s_addr 	= endianness32(faked_ip->to);
			}


			// Only for not fragmented packages
			package_done = all;

			original = original->forward;
		}
		pthread_mutex_unlock(&hash_rw);

		// Only for fragmented packages
		target = sender;
	}

	// Not supported Protocol Number
	else
	{
		free(ipp);
		return INVALID;
	}

	// Stateless Rules Check && Forbidden words
	if (check_stateless(pack, ipp->protocol) || check_forbidden(pack->msg, (uint16_t)msglen, word_rules))
	{
		if (frag)
		{
			free(package_start);
		}

		if (ipp->protocol == 6) // TCP Header is bigger
		{
			tcp_free((tcp *) pack);
		}
		else
		{
			free_protocol((struct package *) pack);
		}

		free(ipp);

		pthread_mutex_unlock(&send_w);
		return CORRUPT;
	}

	/* TCP */
	// Send SYN/ACK Message
	else if (state == CONNECT || state == STARTEND || state == RECEIVE)
	{
		if (state == RECEIVE)
		{
			// Acknowledge Packge Later
			send_ahead(package_done, ipp->to, ipp->tot_lenght, &addr);
		}

		tcp * tpack = (tcp *) pack;

		in_port_t port = tpack->header->source;
		addr.sin_addr.s_addr 	= ipp->to;
		addr.sin_port		= port; 

		// Changes the payload ptr
		ip *faked = ip_extract(payload);
		
		// Big Endian
		faked->from 	= endianness32(ipp->to);
		faked->to	= endianness32(ipp->from);
		faked->tot_lenght = endianness16(40);
		faked->ihl	= 5;
		faked->checksum = 0;
		faked->checksum = endianness16(checksum(payload, 20, 0));
		
		tcphdr *tfaked = tcph_extract(package_start);
		tfaked->flags  = target->tw_stage;

		tfaked->reserved = 0;
		tfaked->source = endianness16(tpack->header->destin);
		tfaked->destin = endianness16(tpack->header->source);
		tfaked->offset = 0x5; // 4 bits	
		tfaked->acknum = endianness32(tpack->header->acknum);
		tfaked->seqnum = endianness32(tpack->header->seqnum);

		tfaked->checksum = endianness16(tpack->header->checksum);
		tfaked->winsiz = endianness16(tpack->header->winsiz);
	}

	else if (state == END)
	{
		// Clear hash

		pthread_mutex_lock(&hash_rw);	
		target = search_hn(fr_hash, 0, ipp->from, ipp->to, ((tcp *) pack)->header->source, ((tcp *) pack)->header->destin, ipp->protocol);

		if (target == NULL)
		{

			free(ipp);
			tcp_free((tcp *) pack);
			return CORRUPT;
		}

		pop_hashn(fr_hash, target);
		pthread_mutex_unlock(&hash_rw);

		free_hashn(target);

		tcp_free((tcp *) pack);
		free(ipp);

		printf("TCP CLOSED.\n");
		return ACCEPTED;
	}

	if (frag)
	{
		fragment *tracker = target->box;

		while (tracker)
		{
			ip *temp = ip_init(tracker->all);

			send_ahead(tracker->all, temp->to, temp->tot_lenght, &addr);
			tracker = tracker->forward;
			free(temp);
		}

		free(package_start);
	}
	else
	{
		send_ahead(package_done, ipp->to, ipp->tot_lenght, &addr);
	}

	if (ipp->protocol == 6)
	{
		tcp_free((tcp *) pack);
	}
	else
	{
		free_protocol((struct package *) pack);
	}
	pthread_mutex_unlock(&send_w);

	free(ipp);
	return ret;	
}

void see_package(uint8_t *msg, uint32_t size)
{
	for (uint32_t i = 0; i < size; i++)
	{
		printf("%02x", msg[i]);
	}
	printf("\n");
}


void turn_end(int32_t sig)
{
	end = 1;
	return ;
}

uint8_t nodata(uint8_t *head)
{
	uint8_t hsiz = head[0] & 0x0F;
	uint16_t len = hsiz? (head[2] << 8) | head[3]: 0;

	return len? 0: 1;
}



uint8_t check_stateless(void *pack, uint8_t protocol)
{
	trtree *tree = port_rules;
	uint8_t ret  = 0; // Starts not Blocked because of ICMP (no validation)

	pthread_mutex_lock(&mx_rule); // Rules Mutex
	if (!protocol)
	{
		ip *spec = (ip *) pack;
		tree 	 = ip_rules;
		ret 	 = blocked(tree, spec->to, 32, IN) || blocked(tree, spec->from, 32, OUT);
	}
	else if (protocol == 6)
	{
		tcp *spec = (tcp *) pack;
		tree 	  = ip_rules;
		ret 	  = blocked(tree, spec->header->destin, 32, IN) || blocked(tree, spec->header->source, 32, OUT);

	}
	else if (protocol == 17)
	{
		udp *spec = (udp *) pack;
		ret 	  = blocked(tree, spec->header->destin, 16, IN) || blocked(tree, spec->header->source, 16, OUT);
	}
	pthread_mutex_unlock(&mx_rule);

	return ret;

}

void load_stateless()
{
	// Multithreading haven't begun
	uint8_t ret;

	uint32_t fd = open(STATELESS, O_RDONLY);
	if (fd == -1)
	{
		perror("Read error.\n");
		return ;
	}


	rules fdata;
	uint32_t sttsz = sizeof(struct rules); 
	while (read(fd, &fdata, sttsz))
	{
		if (fdata.op & 0x2) // Detecting Strings
		{
			// Lim already includes NULL char
			char * str = (char *) malloc(sizeof(char) * (fdata.lim));
			read(fd, str, fdata.lim);


			lknode * node = lknode_init(str, fdata.lim, fdata.ways);
			if (lk_insert(word_rules, node))
			{
				printf("Unable to add rule: %s.\n", str);
			}
			else
			{
				printf("word: %s added\n", str);
			}
			//lklist_print(word_rules); // DEBUG

			continue ;
		}

		if (fdata.op)
		{
			printf("%d - %b not blocked\n", fdata.data, fdata.data);
			continue;
		}

		printf("%d - %b : %hhd -> %hd added\n", fdata.data, fdata.data, fdata.ways, fdata.lim);
		if (!fdata.lim)
		{
			ret = tr_insert(port_rules, fdata.data, 16, fdata.ways, 16);
		}
		else
		{
			ret = tr_insert(ip_rules, fdata.data, fdata.lim, fdata.ways, 32);
		}

		if (ret)
		{
			printf("Error while inserting new rule.\n");
		}
	}
	close(fd);

	return ;
}

uint32_t setup_socket()
{

	uint32_t sockfd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sockfd < 0)
	{
		return 0;
	}

	unlink(SOCKPATH);

	struct sockaddr_un addr;

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SOCKPATH, sizeof(addr.sun_path));

	socklen_t lenght = sizeof(addr);

	if (bind(sockfd, (struct sockaddr *) &addr, lenght))
	{
		return 0;
	}

	return sockfd;	
}


void package_denied()
{
	return ;
}

void decode(udp *pack)
{
	for (uint32_t i = 0; i < pack->header->lenght; i++)
	{
		printf("%c", pack->msg[i]);
	}	
	printf("\n");
	fflush(0);
}

void save_rules()
{
	uint32_t rulesfd = open(STATELESS, O_WRONLY | O_CREAT | O_TRUNC, 0600);

	trtree *ptr;
	rules buf;

	uint32_t size = max(port_rules->size, ip_rules->size);
	uint32_t rulesz = sizeof(struct rules); 
	// Port Rules

	trnode ** anodes = (trnode **) malloc(sizeof(trnode *) * size);
	uint8_t deep;

	for (uint8_t l = 0; l < RULETYPE; l++)
	{
		switch(l)
		{
			case 0:
				deep = 16;
				ptr = port_rules;
				break;
			case 1:
				deep = 32;
				ptr = ip_rules;
				break;
		}


		uint32_t *rulesl = get_nodes(ptr, anodes, deep);

		for (uint32_t i = 0; i < ptr->size; i++)
		{	
			if (!rulesl[i] && !anodes[i])
			{
				continue;
			}

			buf.op	 = 0;
			buf.lim	 = l? anodes[i]->deep: 0;
			buf.ways = anodes[i]->ways;
			buf.data = rulesl[i];

			if (write(rulesfd, &buf, rulesz) == -1)
			{
				printf("Rules couldn't be saved.\n");
			}
			else
			{
				printf("Rule: %d:%hhd -> %hd saved\n", buf.data, buf.ways, buf.lim); // DEBUG

			}
		}
		free(rulesl);
	}
	free(anodes);

	char str[512];

	// Write Strings
	lknode * tracker = word_rules->head;
	while(tracker != NULL)
	{
		// Insert && String
		buf.op = 0x2; 

		// With NULL char
		buf.lim = tracker->chars; 
		buf.ways = tracker->ways;
		buf.data = 0;

		// Write struct rules without alt ptr or string
		if (write(rulesfd, &buf, rulesz) == -1)
		{
			printf("Rules couldn't be saved.\n");
			printf("%s\n", strerror(errno));

			tracker = tracker->forward;
			continue ;
		}

		memset(str, 0, 512 * sizeof(char));
		memcpy(str, tracker->key, tracker->chars);
		if (write(rulesfd, str, tracker->chars) == -1)
		{
			printf("Rules couldn't be saved.\nData corrupted, RUN restart.sh to clear the rules file.\n");
			close(rulesfd);

			return ;
		}
		
		else
		{
			printf("Rule: %s:%hhd -> %hd\n", str, buf.ways, tracker->chars);
		}

		tracker = tracker->forward;
	}

	close(rulesfd);

	return ;
}

uint32_t setup_exit() 
{
	uint32_t fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if (fd < 0)
	{
		return 0;
	}
	
	return fd;
}

void send_ahead(uint8_t *pack, uint32_t destin, uint32_t tsize, struct sockaddr_in *addr)
{
	if (destin & 0xA000000) // Mesmo ip da máquina = enviado para uma port
	{
		if (write(tunfd, (void *)pack, tsize) == -1)
		{
			printf("Error: %s\n", strerror(errno));
		}
		else
		{
			printf("Package Sent to:");
			show_ip(destin);
		}
	}

	else // Ip de fora, deve ser enviado para outra máquina
	{
		if (sendto(exitfd, pack, tsize, 0, (struct sockaddr *) addr, sizeof(struct sockaddr_in)) == -1)
		{
			printf("Error: %s\n", strerror(errno));
		}
		else
		{
			printf("Package Sent to:");
			show_ip(destin);
		}
	}
	
	return ;
}

uint8_t check_forbidden(uint8_t *msg, uint16_t lenght, lklist * rules)
{
	// Adds null char
	//
	char * string = (char *) malloc(sizeof(char) * (lenght + 1));
	memcpy(string, msg, lenght);
	string[lenght] = '\0';

	// Linked List head
	char * found = NULL;

	pthread_mutex_lock(&mx_rule);
	lknode *tracker = rules->head;

	// Checks all forbidden words
	while (tracker != NULL)
	{
		//printf("Checking for %s\n", (char *)tracker->key); // DEBUG
		found = strstr(string, tracker->key);
		if (found)
		{
			free(string);
			pthread_mutex_unlock(&mx_rule);
			return 1;
		}

		tracker = tracker->forward;
	}
	pthread_mutex_unlock(&mx_rule);

	free(string);
	return 0;
}

uint32_t max(uint32_t a, uint32_t b)
{
	return a > b? a: b;
}
