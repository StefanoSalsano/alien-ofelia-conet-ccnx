#include <conet/conet.h>
#include <conet/info_passing.h>//&ct
//the following variables have default values set in conet.h and their value can be
//changed by the executable parameters argc
int USE_CONET = USE_CONET_DEFAULT;
int CONET_PREFETCH = 0;
//int TAG_VLAN = TAG_VLAN_DEFAULT;
int MIN_PACK_LEN = MIN_PACK_LEN_DEFAULT;

int SS_THRESH = SS_THRESH_DEFAULT;
int USER_PARAM_1 = 0;

#ifdef CONET_MULTI_HOP
int fwd_sock = 0;
int raw_toser_sock = 0;
int raw_tocli_sock = 0;
#endif

unsigned int conet_oos_stat = 0;
unsigned int conet_retransmit_stat = 0;

unsigned int out_file_si = 0;
unsigned int out_file_ri = 0;
unsigned int out_file_sd = 0;
unsigned int out_file_rd = 0;
unsigned int retrasmit_file = 0;
double start_time = 0;
unsigned int rcvbuf_size = 0;
struct conet_content_cache* cache = NULL;
struct conet_entry* ce_cache = NULL;

unsigned char * local_mac;
char * local_ip;
unsigned char * local_mac6;
char * local_ip6 = SRC_IP6_ADDR; //ipv6b
cp_descriptor_t* cp_descriptor;//&ct:all info about a received packet will be put here

/**
 * Replica of ccnd_listen_on_wildcards for CoNet server
 * Create the default udp socket for CONET.
 * @return -1 if an error occurred in setup 0 otherwise
 **/

int conet_listen_on_wildcards(struct ccnd_handle *h) //[CONET]: only on 9697, ipv6 tests
{
#ifndef K_MODULE
	int sd, sd6;
	int res, res6;
	struct sockaddr_in6 addr6;
	struct sockaddr_in addr;
	//
	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sd != -1) {
		int yes = 1;
		int rcvbuf = 0;
		socklen_t rcvbuf_sz;
		if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
			close(sd);
			perror("CONET error in setsockopt: ");
			return -1;
		}

		rcvbuf_sz = sizeof(rcvbuf);
		getsockopt(sd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, &rcvbuf_sz);
		int buffsize = 1024 * 1024 * 100;
		if (setsockopt(sd, SOL_SOCKET, SO_RCVBUF, (char*) &buffsize,
				sizeof(buffsize))) {
			perror("error  in setsockopt");
		}
		getsockopt(sd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, &rcvbuf_sz);

	    bzero((char *) &addr, sizeof(addr)); //whr
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY); //CoNet server listen on any addresses
		addr.sin_port = htons(CONET_DEFAULT_UNICAST_PORT_NUMBER); //CoNet server port number
		if (CONET_DEBUG >= 2)
			fprintf(stderr, "CONET PORT: %d  RCV_BUFFER: %d\n", ntohs(
					addr.sin_port), rcvbuf);
#ifdef IS_CLIENT
		if (CONET_DEBUG >= 2)
			fprintf(stderr, "Defined role: CLIENT\n");
#endif
#ifdef IS_SERVER
		if (CONET_DEBUG >= 2)
		fprintf(stderr, "Defined role: SERVER\n");
#endif
#ifdef IS_CACHE_SERVER
		if (CONET_DEBUG >= 2) fprintf(stderr, "Defined role: CACHE SERVER\n");
#endif
		if (CONET_DEBUG >= 2)
			fprintf(
					stderr,
					"USE_CONET:%d  CONET_PREFETCH:%d  MIN_PACK_LEN:%d  SS_THRESH:%d USER_PARAM_1:%d\n",
					USE_CONET, CONET_PREFETCH, MIN_PACK_LEN, SS_THRESH,
					USER_PARAM_1);

		res = bind(sd, (struct sockaddr *) &addr, sizeof(addr));//bind the address to the socket
		rcvbuf_size = rcvbuf;
		if (res != 0) {
			close(sd);
			perror("CONET error in bind: ");
			return -1;
		}

		struct face * face = record_connection(h, sd, (struct sockaddr*) &addr,
				sizeof(addr), CCN_FACE_DGRAM | CCN_FACE_PASSIVE );
		if (face == NULL) {
			abort();
		}
		//setting up of the ccn-handler for supporting CoNet
		h->conet_fds = calloc(1, sizeof(struct pollfd));
		h->conet_fds->fd = sd;
		h->conet_fd = sd;
		h->conet_fds[0].fd = sd;
		h->conet_nfds = 1;
		h->conet_fds[0].events = POLLIN; //TODO if the sending interest will use this fds(with a scheduler) we need to poll also for POLLOUT

		ccnd_msg(h, "[CONET]:accepting %s datagrams on fd %d rcvbuf %d",
				(addr.sin_family == AF_INET) ? "ipv4" : "ipv6", sd, rcvbuf);
	}
	//
    bzero((char *) &addr6, sizeof(addr6)); //whr
	addr6.sin6_family = AF_INET6;
	addr6.sin6_addr = in6addr_any; //CoNet server listen on any addresses
	addr6.sin6_port = htons(CONET_DEFAULT_UNICAST_PORT_NUMBER); //CoNet server port number


	sd6 = socket(AF_INET6, SOCK_DGRAM, 0);
	if (sd6 != -1) {
		int yes6 = 1;
		int rcvbuf6 = 0;
		socklen_t rcvbuf_sz6;
		if (setsockopt(sd6, SOL_SOCKET, SO_REUSEADDR, &yes6, sizeof(yes6)) < 0) {
			close(sd6);
			perror("CONET error in setsockopt: ");
			return -1;
		}

		rcvbuf_sz6 = sizeof(rcvbuf6);
		getsockopt(sd6, SOL_SOCKET, SO_RCVBUF, &rcvbuf6, &rcvbuf_sz6);
		int buffsize6 = 1024 * 1024 * 100;
		if (setsockopt(sd6, SOL_SOCKET, SO_RCVBUF, (char*) &buffsize6,
				sizeof(buffsize6))) {
			perror("error  in setsockopt");
		}
		getsockopt(sd6, SOL_SOCKET, SO_RCVBUF, &rcvbuf6, &rcvbuf_sz6);

		if (CONET_DEBUG >= 2)
			fprintf(stderr, "CONET PORT: %d  RCV_BUFFER: %d\n", ntohs(
					addr6.sin6_port), rcvbuf6);
#ifdef IS_CLIENT
		if (CONET_DEBUG >= 2)
			fprintf(stderr, "Defined role: CLIENT\n");
#endif
#ifdef IS_SERVER
		if (CONET_DEBUG >= 2)
		fprintf(stderr, "Defined role: SERVER\n");
#endif
#ifdef IS_CACHE_SERVER
		if (CONET_DEBUG >= 2) fprintf(stderr, "Defined role: CACHE SERVER\n");
#endif
		if (CONET_DEBUG >= 2)
			fprintf(
					stderr,
					"USE_CONET:%d  CONET_PREFETCH:%d  MIN_PACK_LEN:%d  SS_THRESH:%d USER_PARAM_1:%d\n",
					USE_CONET, CONET_PREFETCH, MIN_PACK_LEN, SS_THRESH,
					USER_PARAM_1);

		res6 = bind(sd6, (struct sockaddr *) &addr6, sizeof(addr6));//bind the address to the socket
		//		int rcvbuf_size6 = rcvbuf6;
		if (res6 != 0) {
			close(sd6);
			perror("CONET error in bind: ");
			return -1;
		}

		struct face * face6 = record_connection(h, sd6,
				(struct sockaddr*) &addr6, sizeof(addr6), CCN_FACE_DGRAM
						| CCN_FACE_PASSIVE );
		if (face6 == NULL) {
			abort();
		}
		//setting up of the ccn-handler for supporting CoNet
		//conet_fds6 dovrebbe essere inutile
		h->conet_fds6 = calloc(1, sizeof(struct pollfd));
		h->conet_fds6->fd = sd6;
		h->conet_fd6 = sd6;
		h->conet_fds6[0].fd = sd6;
		h->conet_nfds6 = 1;
		h->conet_fds6[0].events = POLLIN; //TODO if the sending interest will use this fds(with a scheduler) we need to poll also for POLLOUT

		ccnd_msg(h, "[CONET]:accepting %s datagrams on fd %d rcvbuf %d",
				(addr6.sin6_family == 4) ? "ipv4" : "ipv6", sd6, rcvbuf6);

		int raw_fd6 = setup_raw_socket_listener();
		if (setsockopt(sd6, SOL_SOCKET, SO_RCVBUF, (char*) &buffsize6,
				sizeof(buffsize6))) {
			perror("error  in setsockopt");
		}
		getsockopt(sd6, SOL_SOCKET, SO_RCVBUF, &rcvbuf6, &rcvbuf_sz6);
		h->conet_raw_fd = raw_fd6;
		struct sockaddr_in6 raw_addr6;
		addr6.sin6_family = AF_INET6;
		addr6.sin6_addr = in6addr_any; //CoNet server listen on any addresses
		addr6.sin6_port = htons(CONET_DEFAULT_RAW_PORT_NUMBER); //CoNet server port number

		struct face * raw_face6 = record_connection(h, raw_fd6,
				(struct sockaddr*) &raw_addr6, sizeof(raw_addr6),
				CCN_FACE_DGRAM | CCN_FACE_PASSIVE );
		if (raw_face6 == NULL) {
			abort();
		}
		ccnd_msg(h, "[CONET]:accepting raw datagrams on fd %d rcvbuf %d",
				raw_fd6, rcvbuf6);
		setup_local_addresses(h);

	}
#endif
	return 0;
}

int bind2if(int sock)
{
	struct ifreq ifr;
	bzero(&ifr, sizeof(struct ifreq));
	strncpy(ifr.ifr_name, CONET_IFNAME, IFNAMSIZ);

	if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
		perror("ioctl SIOCGIFINDEX");
		return -1;
	}

	struct sockaddr_ll saddr;
	bzero(&saddr, sizeof(struct sockaddr_ll));
	saddr.sll_family = AF_PACKET;
	saddr.sll_protocol = htons(ETH_P_ALL);
	saddr.sll_ifindex = ifr.ifr_ifindex;

	if (CONET_DEBUG >= 2)
		fprintf(stderr, "\nBinding to IFINDEX: %d \n", ifr.ifr_ifindex);

	if (bind(sock, (struct sockaddr *) (&saddr), sizeof(saddr)) < 0) {
		perror("bind error");
		return -1;
	}
	return 0;
}

int setup_local_addresses(struct ccnd_handle* h) {
	struct ifreq s;
	char* my_ip;
	strcpy(s.ifr_name, CONET_IFNAME);
	if (0 == ioctl(h->conet_raw_fd, SIOCGIFHWADDR, &s)) {
		local_mac = calloc(1, sizeof(s.ifr_addr.sa_data));
		memcpy(local_mac, s.ifr_addr.sa_data, sizeof(s.ifr_addr.sa_data));
	} else {
		if (CONET_DEBUG >= 2)
			fprintf(
					stderr,
					"ERROR in getting MAC address for %s ... check CONET_IFNAME\n",
					CONET_IFNAME);
		abort();
	}
	struct ifreq ifr;
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, CONET_IFNAME, IFNAMSIZ - 1);
	if (0 == ioctl(h->conet_raw_fd, SIOCGIFADDR, &ifr)) {

	} else {
		if (CONET_DEBUG >= 2)
			fprintf(stderr, "ERROR in getting IF address for %s\n",
					CONET_IFNAME);
		abort();
	}
	//ipv6b qui andrebbe settato il local ip per ipv6 se serve
	my_ip = inet_ntoa(((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr);
	local_ip = calloc(1, strlen(my_ip) + 1);
	memcpy(local_ip, my_ip, strlen(my_ip));
	if (CONET_DEBUG >= 2) {
		fprintf(stderr, "Interface %s IP: %s MAC: ", CONET_IFNAME, local_ip);
		int i;
		for (i = 0; i < 6; i++) {
			fprintf(stderr, "%02X ", local_mac[i]);
		}
		fprintf(stderr, "\n");
	}
	return 0;
}

int setup_raw_socket_listener(void)
{
	//	int sock = socket(AF_INET,SOCK_RAW,CONET_PROTOCOL_NUMBER) ;
	//
	//
	//	if (sock< 0) {
	//		perror("[CONET]: Failed to create RAW socket");
	//		return -1;
	//	}
	int sock;
	if ((sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
		perror("[CONET]: Failed to create RAW socket");
		return -1;
	}
	//	int tmp = 1, z_stop = 0; const int *val = &tmp;
	//	if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, val, sizeof(tmp)) < 0) {
	//		if (CONET_DEBUG >= 2) fprintf(stderr,"Error: setsockopt() - Cannot set HDRINCL:\n");
	//		return -1; }
	if (sock < 0) {
		perror("RawSocketImp: open(): Cannot open raw socket");
		return -1;
	}

	//	char *opt;
	//	opt = CONET_IFNAME;
	//	if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, opt, strlen(CONET_IFNAME))<0){
	//		if (CONET_DEBUG >= 2) fprintf(stderr, "[CONET]: Failed to bind socket to interface %s", CONET_IFNAME);
	//	}
	//	struct sockaddr_in server_addr;
	//
	//	bzero((char*)&server_addr,sizeof(server_addr));
	//	server_addr.sin_family = AF_INET;
	//	server_addr.sin_addr.s_addr = INADDR_ANY;
	//
	//	if (bind(sock, (struct sockaddr *) &server_addr, sizeof server_addr) < 0)
	//	{
	//		perror("RawSocketImp: bind()");
	//		return -1;
	//	}
	if ((bind2if(sock)) < 0) {
		if (CONET_DEBUG >= 2)
			fprintf(stderr, "[CONET]: Failed to bind socket to interface %s",
					CONET_IFNAME);
	}

	return sock;
}

/**
 * NOT USED
 * TODO use after implementing support for listening on a different port (see conet_listen_on)
 */
int conet_listen_on_address(struct ccnd_handle *h, const char *addr)
{
	/*
	 int fd;
	 int res;
	 struct addrinfo hints = {0};
	 struct addrinfo *addrinfo = NULL;
	 struct addrinfo *a;
	 int ok = 0;

	 ccnd_msg(h, "listen_on %s", addr);
	 hints.ai_socktype = SOCK_DGRAM;
	 hints.ai_flags = AI_PASSIVE;
	 res = getaddrinfo(addr, h->portcp, &hints, &addrinfo);
	 if (res == 0) {
	 for (a = addrinfo; a != NULL; a = a->ai_next) {
	 fd = socket(a->ai_family, SOCK_DGRAM, 0);
	 if (fd != -1) {
	 struct face *face = NULL;
	 int yes = 1;
	 int rcvbuf = 0;
	 socklen_t rcvbuf_sz;
	 setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
	 rcvbuf_sz = sizeof(rcvbuf);
	 getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, &rcvbuf_sz);
	 if (a->ai_family == AF_INET6)
	 ccnd_setsockopt_v6only(h, fd);
	 res = bind(fd, a->ai_addr, a->ai_addrlen);
	 if (res != 0) {
	 close(fd);
	 continue;
	 }
	 face = record_connection(h, fd,
	 a->ai_addr, a->ai_addrlen,
	 CCN_FACE_DGRAM | CCN_FACE_PASSIVE);
	 if (face == NULL) {
	 close(fd);
	 continue;
	 }
	 if (a->ai_family == AF_INET)
	 h->ipv4_faceid = face->faceid;
	 else
	 h->ipv6_faceid = face->faceid;
	 ccnd_msg(h, "accepting %s datagrams on fd %d rcvbuf %d",
	 af_name(a->ai_family), fd, rcvbuf);
	 ok++;
	 }
	 }
	 for (a = addrinfo; a != NULL; a = a->ai_next) {
	 fd = socket(a->ai_family, SOCK_STREAM, 0);
	 if (fd != -1) {
	 int yes = 1;
	 setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
	 if (a->ai_family == AF_INET6)
	 ccnd_setsockopt_v6only(h, fd);
	 res = bind(fd, a->ai_addr, a->ai_addrlen);
	 if (res != 0) {
	 close(fd);
	 continue;
	 }
	 res = listen(fd, 30);
	 if (res == -1) {
	 close(fd);
	 continue;
	 }
	 record_connection(h, fd,
	 a->ai_addr, a->ai_addrlen,
	 CCN_FACE_PASSIVE);
	 ccnd_msg(h, "accepting %s connections on fd %d",
	 af_name(a->ai_family), fd);
	 ok++;
	 }
	 }
	 freeaddrinfo(addrinfo);
	 }
	 return(ok > 0 ? 0 : -1);
	 */
	return -17;
}
/**
 * adapted replica of ccnd_listen_on for CoNet server
 * TODO no support for listening on a different port
 */
int conet_listen_on(struct ccnd_handle *h, const char *addrs) //TODO update also conet_listen_on_address!!!
{
	/*
	 unsigned char ch;
	 unsigned char dlm;
	 int res = 0;
	 int i;
	 struct ccn_charbuf *addr = NULL;
	 */

	if (addrs == NULL || !*addrs || 0 == strcmp(addrs, "*"))
		return (conet_listen_on_wildcards(h));
	//else
	if (CONET_DEBUG >= 2)
		fprintf(stderr,
				"[CONET]: using specific addr to listen on not yet supported/planned.");
	abort();

	/*
	 addr = ccn_charbuf_create();
	 for (i = 0, ch = addrs[i]; addrs[i] != 0;) {
	 addr->length = 0;
	 dlm = 0;
	 if (ch == '[') {
	 dlm = ']';
	 ch = addrs[++i];
	 }
	 for (; ch > ' ' && ch != ',' && ch != ';' && ch != dlm; ch = addrs[++i])
	 ccn_charbuf_append_value(addr, ch, 1);
	 if (ch && ch == dlm)
	 ch = addrs[++i];
	 if (addr->length > 0) {
	 res |= conet_listen_on_address(h, ccn_charbuf_as_string(addr));
	 }
	 while ((0 < ch && ch <= ' ') || ch == ',' || ch == ';')
	 ch = addrs[++i];
	 }
	 ccn_charbuf_destroy(&addr);
	 return(res);
	 */

	return -1;
}

/**
 * Retrieve the input from the CoNet port and call the function to process the received message.
 * @param fd is the descriptor of sender socket
 * @return the number of bytes read or -1 if receiving input error
 */
int conet_process_input(struct ccnd_handle* h, int fd)
{
	if (CONET_DEBUG >= 2)
		fprintf(stderr,
				"\n[conet.c: %d] new conet_process_input(...) invocation\n",
				__LINE__);

	unsigned char* recvbuf = NULL;
	struct sockaddr_storage sstor;
	socklen_t addrlen = sizeof(sstor);
	struct sockaddr *addr = (struct sockaddr *) &sstor;
	int expected_dim = recvfrom(fd, recvbuf, 0, MSG_TRUNC | MSG_PEEK, NULL,
			NULL); //read a packet dimension from the socket without removing data from the queue
	if (expected_dim == 0) {
		perror("[CONET]: Empty UDP packet received.\n");
		return 0;
	}
	if (expected_dim < 0) {
		perror("[CONET]: Error in conet_process_input -recvfrom()-");
		return -1;
	}
	recvbuf = calloc(expected_dim, sizeof(unsigned char)); //allocate memory for the dimension obtained from the last recvfrom call
	if (recvbuf == NULL) {
		perror("[CONET]: Error in conet_process_input -calloc()-");
		return -1;
	}
	memset(&sstor, 0, sizeof(sstor));
	int read = recvfrom(fd, recvbuf, expected_dim, 0, addr, &addrlen);//read the whole UDP packet
	//if (CONET_DEBUG >= 2) fprintf(stderr, "[conet.c:%d]: Bytes read from CONET UDP socket=%d, from address:%s\n",__LINE__,  read, inet_ntoa( ((struct sockaddr_in*)addr)->sin_addr ) );

	if (read == -1) {
		perror("[CONET]: Error in conet_process_input-recvfrom()-");
		return -1;
	}
	char* src_addr;
	if (sstor.ss_family == AF_INET6) {
		char src_addr6[INET6_ADDRSTRLEN];
		inet_ntop(AF_INET6, ((struct sockaddr_in6*) addr)->sin6_addr.s6_addr,
				src_addr6, INET6_ADDRSTRLEN);
		src_addr = &src_addr6[0];
	} else {
		src_addr = inet_ntoa(((struct sockaddr_in*) addr)->sin_addr);
	}
	//src_addr = "192.168.1.83"; //&ct: ATTENZIONE: facendo in questo modo si inviranno sempre i dati indietro verso la cache. Quando il controller sara' ok questa riga va tolta

	if (fd == h->conet_raw_fd) {

		if (CONET_DEBUG == 2) {
			int i = 0;
			fprintf(stderr, "Received bytes:\n");
			for (i = 0; i < expected_dim; i++) {
				if (i % 16 == 0) {
					fprintf(stderr, "%04X  ", i);
				}
				fprintf(stderr, " %02X", recvbuf[i]);
				if (i % 16 == 15) {
					fprintf(stderr, "\n");
				}
			}
			fprintf(stderr, "\n");
		}

		//unsigned short eth_proto = *(recvbuf+12);
		unsigned short eth_proto = *(unsigned short *) (recvbuf + 12);
		if (CONET_DEBUG >= 2)
			fprintf(stderr, "ETH PROTO: %04X \n", ntohs(eth_proto));

		unsigned short vlan_oh = 0;
		if (eth_proto == htons(0x8100)) {
			if (CONET_DEBUG >= 2)
				fprintf(stderr, "ETH PROTO: %04X (VLAN)\n", ntohs(eth_proto));
			vlan_oh = 4;
		}

		if (eth_proto == htons(0x0806)) {
			if (CONET_DEBUG >= 2)
				fprintf(stderr, "ETH PROTO: %04X (ARP)\n", ntohs(eth_proto));
			if (CONET_DEBUG >= 2)
				fprintf(stderr, "This is ARP. Drop it.\n");
			free(recvbuf);
			return -1;
		}

		if (memcmp(recvbuf + 6, local_mac, 6) == 0) { //source MAC address is local MAC
			if (CONET_DEBUG >= 2)
				fprintf(stderr, "Source MAC is my local MAC. Drop it.\n");
			free(recvbuf);
			return -1;
		}
#ifndef	IS_CACHE_SERVER
		if (memcmp(recvbuf, local_mac, 6) != 0) {
			if (CONET_DEBUG >= 2)
				fprintf(stderr,
						"[conet.c:%d] Dest MAC is not my local MAC. Drop it",
						__LINE__);
			free(recvbuf);
			return -1;
		} else {
			if (CONET_DEBUG >= 2)
				fprintf(
						stderr,
						"[conet.c:%d] Dest MAC is my local MAC. I'm going to process the frame",
						__LINE__);
		}
#endif
		int ip_src_offset;
		if (eth_proto == htons(0x0800)) {
			ip_src_offset = 14 + vlan_oh + 12;
			struct sockaddr_in src_sockaddr;

			bzero((char *) &src_sockaddr, sizeof(src_sockaddr)); //whr
			memcpy(&(src_sockaddr.sin_addr), recvbuf + ip_src_offset, 4);
			src_addr = inet_ntoa(src_sockaddr.sin_addr);
		} else if (eth_proto == htons(0x86dd)) {
			src_addr = ""; //metti qui l'ip sorgente dei pacchetti in arrivo (per il server il client per il client il server)
		}

		//&ct: inizio
#ifdef IS_CACHE_SERVER
		memset(cp_descriptor->source_ip_addr, '\0', IP_ADDR_STRING_LEN);//IP_ADDR_STRING_LEN from info_passing.h //&ct
		memcpy(cp_descriptor->source_ip_addr, src_addr, strlen(src_addr));//&ct
#endif
		//&ct: fine
#ifdef CONET_MULTI_HOP
		conet_process_raw_input_message(h, src_addr, recvbuf,expected_dim);

#else
		conet_process_raw_input_message(h, src_addr, recvbuf);
#endif

	} else {
#ifdef CONET_MULTI_HOP
		conet_process_input_message(h, src_addr, recvbuf, 0,expected_dim); //TODO maybe it's necessary to create a thread
#else
		conet_process_input_message(h, src_addr, recvbuf, 0); //TODO maybe it's necessary to create a thread
#endif
	}
	//free recvbuf if necessary. IF AND ONLY IF the handler-function is NOT called within a thread o new process]
	free(recvbuf);
	return read; //TODO check return value list
}

#ifdef CONET_MULTI_HOP
void conet_process_raw_input_message(struct ccnd_handle* h, char* src_addr,
		unsigned char* readbuf, int size)
#else
void conet_process_raw_input_message(struct ccnd_handle* h, char* src_addr,
		unsigned char* readbuf)
#endif
{
	unsigned short eth_proto = *(unsigned short*) (readbuf + 12);
	unsigned short vlan_oh = 0;
	if (eth_proto == htons(0x8100)) {
		if (CONET_DEBUG >= 2)
			fprintf(stderr, "ETH PROTO: %04X (VLAN)\n", ntohs(eth_proto));
		vlan_oh = 4;
		unsigned short eth_proto2 = *(unsigned short*) (readbuf + 12 + vlan_oh);
		if (eth_proto2 == htons(0x0800)) {
			if (CONET_DEBUG >= 2)
				fprintf(stderr, "ETH PROTO2: %04X (IP)\n", ntohs(eth_proto2));
		} else {
			if (CONET_DEBUG >= 2)
				fprintf(stderr,
						"ETH PROTO2: %04X (NOT IP!!!!). Packet dropped\n",
						ntohs(eth_proto2));
			return;
		}
	} else {
		if (eth_proto != htons(0x86dd) && eth_proto != htons(0x0800)) {
			if (CONET_DEBUG >= 2)
				fprintf(stderr,
						"ETH PROTO: %04X (NOT IP!!!!). Packet dropped\n",
						ntohs(eth_proto));
			return;
		}
	}

	//	int hdr_offset = 14 + vlan_oh + 20 + 2; //14 byte eth header + 4 byte vlan tag + 20 byte IP header overhead + 2 byte ip option type and len
	int hdr_offset;
	if (eth_proto == htons(0x86dd))
		hdr_offset = 14 + vlan_oh + 40 + 1; //14 byte eth header + 4 byte vlan tag + 40 byte IP header overhead + 1 byte ip option type, now len is procesed in conet_process_data_cp() or conet_process_interest_cp()
	else
		hdr_offset = 14 + vlan_oh + 20 + 1; //14 byte eth header + 4 byte vlan tag + 20 byte IP header overhead + 1 byte ip option type, now len is procesed in conet_process_data_cp() or conet_process_interest_cp()

	if (readbuf[hdr_offset - 1] == CONET_IPV6_OPTION_CODE) {
		//	int hdr_offset = 14 + vlan_oh + 20 + 1; //14 byte eth header + 4 byte vlan tag + 20 byte IP header overhead + 1 byte ip option type, now len is procesed in conet_process_data_cp() or conet_process_interest_cp()
#ifdef CONET_MULTI_HOP
		conet_process_input_message(h, src_addr, readbuf + hdr_offset, 1, size);
#else
		conet_process_input_message(h, src_addr, readbuf + hdr_offset, 1);
#endif
	} else if (readbuf[hdr_offset - 1] == CONET_IP_OPTION_CODE) {
#ifdef CONET_MULTI_HOP
		conet_process_input_message(h, src_addr, readbuf + hdr_offset, 1, size);// per adesso niente intermedio raw
#else
		conet_process_input_message(h, src_addr, readbuf + hdr_offset, 1);
#endif
	} else {
		if (CONET_DEBUG >= 2)
			fprintf(stderr, "ETH PROTO: %04X ", ntohs(eth_proto));
		if (CONET_DEBUG >= 2)
			fprintf(stderr,
					"Packet from %s dropped, not matching option type=%02X\n",
					src_addr, readbuf[hdr_offset - 2]);
#ifdef CONET_TRANSPORT
#ifdef IPOPT
		hdr_offset = 14 + vlan_oh + 20 +4;
#else
		hdr_offset = 14 + vlan_oh + 20;
#endif
		conet_process_input_message(h, src_addr, readbuf + hdr_offset, 1);
#endif
	}
}

/**
 * Process the message to extract some common information for carrier packet
 * and pass the message to the correct callback function (data or interest)
 */
#ifdef CONET_MULTI_HOP
void conet_process_input_message(struct ccnd_handle* h, char* src_addr,
		unsigned char* readbuf, unsigned short is_raw, int size)
#else
void conet_process_input_message(struct ccnd_handle* h, char* src_addr,
		unsigned char* readbuf, unsigned short is_raw)
#endif
{
	unsigned char CIUflags;
	unsigned char pppp;
	unsigned short ll;
	unsigned short c = 1; //default: cache
	//	unsigned short ask_info = 0;
	unsigned short test_res;

	//analyzing the first byte-flag
#ifndef CONET_TRANSPORT
	if (is_raw)
	CIUflags = readbuf[1]; //ok con len
	else
	CIUflags = readbuf[0];
#else
	CIUflags = readbuf[0];
#endif
	pppp = CIUflags >> 4;
	if ((test_res = (CIUflags | 243)) == 243) //243 = 11110011, LL='00'
		ll = 0;
	else if (test_res == 251) //251 = 11111011, LL='10'
		ll = 2;
	else {
		if (CONET_DEBUG >= 2)
			fprintf(stderr,
					"[CONET]: Using reserved ll-flag in input message. Not yet supported\n");
		return; //TODO manage reserved coding
	}
	if ((CIUflags | 253) == 253) //253 = 11111101, c='0' else c='1'
		c = 0;

	/*
	 if (CONET_DEBUG >= 2)
	 fprintf(stderr,(stderr,"[conet.c:%d]ipv4 flags of input message=%X,ppp=%X,ll=%hu,c=%hu,test_res=%hu,c=%hu\n",
	 __LINE__, CIUflags,pppp,ll,test_res,c);
	 */
	if (pppp & CONET_INTEREST_CIU_TYPE_FLAG) {
		//if (CONET_DEBUG >= 2) fprintf(stderr, "[CONET]: ...calling conet_process_interest_cp()...\n");
#ifdef CONET_MULTI_HOP
		//		if (!is_raw)
		if (propagate_interest_cp(h, src_addr, readbuf, ll, c, 0, is_raw,size) < 0)
#else
		if (conet_process_interest_cp(h, src_addr, readbuf, ll, c, 0, is_raw)
				< 0)
#endif
			if (CONET_DEBUG >= 2)
				fprintf(stderr,
						"[CONET]: Error handling an interest carrier-packet. \n");
		//if (CONET_DEBUG >= 2) fprintf(stderr, "[CONET]: ...returned from call to conet_process_interest_cp()...\n");
	} else if (pppp & CONET_NAMED_DATA_CIU_TYPE_FLAG) {
		//if (CONET_DEBUG >= 2) fprintf(stderr, "[CONET]: ...calling conet_process_data_cp()...\n");
		int ret_process_data;
#ifdef CONET_MULTI_HOP
		//		if (!is_raw)
		if ((ret_process_data = propagate_data_cp(h, src_addr, readbuf, ll,
								c, is_raw, size)) < 0)
#else
		if ((ret_process_data = conet_process_data_cp(h, src_addr, readbuf, ll,
				c, is_raw)) < 0)
#endif
			if (CONET_DEBUG >= 2)
				fprintf(stderr,
						"[CONET]: Error handling a data carrier-packet.\n");
		//		if (CONET_DEBUG >= 2) fprintf(stderr,"[conet.c: %d]: conet_process_data_cp(..) has returned %d (0 means that the chunk has been completely received and has been forwarded to ccnx\n",__LINE__,ret_process_data);

		//if (CONET_DEBUG >= 2) fprintf(stderr, "[CONET]: ...returned from call to conet_process_data_cp()...\n");
	} else {
		if (CONET_DEBUG >= 2)
			fprintf(stderr,
					"[CONET]: Using reserved pppp-flag in input message. Not yet supported\n");
		return; //TODO manage reserved coding
	}
}

/**
 * sends a number of interests according to the available slots in the window
 * returns 0 if ok
 * returns -6 if the sending of one interest resulted in an error (in this case it
 * stops from sending the remaining interests)
 */
int fill_the_cwnd(struct ccnd_handle* h, struct conet_entry* ce,
		struct chunk_entry* ch)
{
	struct chunk_entry* target_ch = NULL;
	int remain_to_send;
	int ret = 0;
	//	if (CONET_DEBUG >= 2) fprintf(stderr, "[conet.c:%d] Fill the cwnd chunk:%d lcp:%d \n", __LINE__, ch->chunk_number, ce->last_processed_chunk);

	if (ch->chunk_number == ce->last_processed_chunk) {
		target_ch = ch;
	} else {
		target_ch = hashtb_lookup(ce->chunks, &(ce->last_processed_chunk),
				sizeof(int));
		if (target_ch == NULL) {
			if (ch->chunk_number > ce->last_processed_chunk) {
				target_ch = ch;
			} else {
				return 0; //SS not clear to me why it returns null //whr messo a 0
			}
		}
	}
	if (ce->final_chunk_number == -1) { //we do not have the last chunk
		while (ce->cwnd > ce->in_flight) {
			remain_to_send = ce->cwnd - ce->in_flight;
			target_ch->last_possible_interest += remain_to_send;
			//if (CONET_DEBUG >= 2) fprintf(stderr, "[conet.c:%d] target_ch:%d\n", __LINE__, target_ch->chunk_number);
			if ((ret = conet_send_interest_cp(h, ce, target_ch, 0)) < 0) {
				if (CONET_DEBUG >= 2)
					fprintf(
							stderr,
							"[conet.c:%d] Error in conet_send interest return -6\n",
							__LINE__);
				return -6;
			}
			//if (CONET_DEBUG >= 2) fprintf(stderr, "[conet.c:%d] res after send:%d\n", __LINE__, ret);

			remain_to_send = ce->cwnd - ce->in_flight;
			//if (CONET_DEBUG >= 2) fprintf(stderr, "[conet.c:%d] remain to send:%d\n", __LINE__, remain_to_send);
			if (ce->cwnd > ce->in_flight) {
				target_ch = chunk_change(h, ce, target_ch, remain_to_send);
			}
			if (target_ch == NULL) {
				break;
			}
		}
	} else { // we already have the last chunk
#ifndef TCP_BUG_FIX  // OK da togliere
		int k;
		struct chunk_entry* ce_oos;
		target_ch = ce->chunk_expected;
		if (ce->chunk_expected != NULL) {
			for (k = ce->chunk_expected->chunk_number; k
					< ce->final_chunk_number && ce->cwnd > ce->in_flight; k++) {
				if (k == ce->chunk_expected->chunk_number) {
					ce_oos = ce->chunk_expected;
				} else {
					ce_oos = hashtb_lookup(ce->chunks, &k, sizeof(int));
				}
				if (ce_oos == NULL) {
					if (CONET_DEBUG >= 2)
					fprintf(stderr,
							"[conet.c:%d] Chunk %d already deleted \n",
							__LINE__, k);
				} else {
					ret += retransmit(h, ce, ce_oos);
					ce->in_flight++;
				}
			}
		}
#endif
	}
	return ret;
}

struct content_entry* conet_cache_check(char *nid, unsigned long long csn) {
	if (cache == NULL)
		return NULL;
	if (memcmp(nid, cache->name, strlen(cache->name))) {
		if (csn == cache->chunk_number) {
#ifdef IS_SERVER
			//&ct
			//always return  NULL: no cache check.
			return NULL;
#else
			return cache->ccn_content;

#endif
		}
	}
	return NULL;
}

/**
 * Manage the data carrier packet, at the end if necessary send new interest or retrasmit
 * @return 0 if the chunk has been completely received and has been forwarded to ccnx ????
 *
 * @return -1 if the data carrier-packet contains reserved flags
 * @return -2 if an error occurred searching in hashtable-data-structures
 * @return -3 if no interest has been sent for this named-data-content or if it was a mistake delivery
 * @return -4 if no interest has been sent for this data carrier-packet or if it was a mistake delivery
 * @return -5 if unexpected replication of an old segment received
 * @return -6 if a request of new segments failed
 */
int conet_process_data_cp(struct ccnd_handle* h, char *src_addr,
		unsigned char* readbuf, unsigned short ll, unsigned short c,
		unsigned short is_raw) {

	char* nid;
	unsigned short nid_length;
	unsigned char* iterator;
	int ret = 0;
	unsigned short is_final = 0;
//	unsigned int retrasmit_start = 0;
	struct conet_entry* ce = NULL;
#ifndef CONET_TRANSPORT
	if (is_raw)
	iterator = readbuf + 1+2; //ip_opt len +flag + DS&T field
	else
	iterator = readbuf + 1;//caso udp da rivedere
#else
	iterator = readbuf + 1;
#endif
	nid = obtainNID(&iterator, ll);
	nid_length = strlen(nid);
#ifdef IS_CACHE_SERVER
	cp_descriptor->type = DATA_DESCRIPTOR;
	cp_descriptor->nid_length = nid_length;
	memcpy(cp_descriptor->nid, nid, cp_descriptor->nid_length);
#endif

	if (ll == 0)
		iterator = iterator + nid_length;//iterator on the first byte of CSN field
	else if (ll == 2)
		iterator = iterator + 1 + nid_length;//iterator on the first byte of CSN field
	else {
		if (CONET_DEBUG >= 2)
			fprintf(stderr, "[conet.c:%d] ll=%d this is an unsupported flag\n",
					__LINE__, ll);
		abort();
	}
	int pos = iterator - readbuf; //pos is the position of the field after NID in byte array readbuf containing the data-cp

	if (ce_cache != NULL && ce_cache->nid != NULL && memcmp(ce_cache->nid, nid,
			nid_length) == 0) {
		ce = ce_cache;
	}
	if (ce_cache != NULL && ce_cache->nid == NULL) {
		//sanitize ce_cache
		ce_cache = NULL;
		ce = NULL;
	}
	if (ce == NULL) {
#ifdef IS_CACHE_SERVER

		ce = malloc(sizeof(struct conet_entry));//&ct: create a fictitious ce
#else
		struct hashtb_enumerator ee;
		struct hashtb_enumerator *e = &ee;

		hashtb_start(h->conetht, e);
		ce = hashtb_lookup(h->conetht, nid, nid_length);

		ce_cache = ce;
		if (ce == NULL) {
			ce_cache = NULL;
			//TODO XXX maybe it's a data-cp for a chunk requested from another node. Check the cache value and decide if cache it
			if (CONET_DEBUG >= 2)
				fprintf(
						stderr,
						"[conet.c:%d] No entry for nid %s discard this carrier packet and return -3\n",
						__LINE__, nid);
			return -3;
		}
#endif
	}
	unsigned long long csn = read_variable_len_number(readbuf, &pos);
#ifndef CONET_TRANSPORT
	if (is_raw)
	pos = (int) readbuf[0] - 1; // - ip_opt len
#endif
#ifdef IS_CACHE_SERVER
	cp_descriptor->csn = csn;//&ct
	int u;
	for (u = 0; u < 4; u++)
	cp_descriptor->tag[u] = readbuf[pos + u];
#endif
#ifndef CONET_TRANSPORT
#ifdef IPOPT
	if (is_raw) {
		pos += 4; //jump 4 byte openflow flag
	}
#endif
#endif
	unsigned char cp_flag = (unsigned char) *(readbuf + pos);
	pos++;

	//&ct: inizio
#ifdef IS_CACHE_SERVER
	cp_descriptor->flags = cp_flag;
#endif
	if (CONET_DEBUG >= 2)
		fprintf(stderr, "[conet.c: %d] cp flags=%d\n", __LINE__,
				(unsigned short) cp_flag);
	//&ct: fine

	unsigned long long l_edge = read_variable_len_number(readbuf, &pos);
	unsigned long long r_edge = read_variable_len_number(readbuf, &pos);

	if ((cp_flag & CONET_FINAL_SEGMENT_FLAG) != 0) {
		if (CONET_DEBUG >= 2)
			fprintf(
					stderr,
					"[conet.c: %d, conet_process_data_cp(..)]: This is a final segment (cp_flag=%d)\n",
					__LINE__, cp_flag);
		is_final = 1;
	} else if (CONET_DEBUG >= 1)
		fprintf(
				stderr,
				"[conet.c: %d, conet_process_data_cp(..)]: This is NOT a final segment (cp_flag=%d)\n",
				__LINE__, cp_flag);

	if (CONET_DEBUG >= 1)
		fprintf(stderr, "[conet.c:%d] Data cp for %s/%d left:%d, right:%d \n",
				__LINE__, nid, (unsigned int) csn, (unsigned int) l_edge,
				(unsigned int) r_edge);
	if (CONET_LOG_TO_FILE) {
		if (out_file_rd == 0) {
			out_file_rd = (unsigned int) fopen("out_rd.csv", "wt");
		}
		log_to_file((FILE*) out_file_rd, csn, l_edge, r_edge);
	}

	struct chunk_entry* ch;
#ifdef IS_CACHE_SERVER
	ch = (struct chunk_entry*) malloc(sizeof(struct chunk_entry));//&ct: allochiamo uno spazio fasullo
	memset(ch, 0, sizeof(struct chunk_entry));
#else
	ch = hashtb_lookup(ce->chunks, &csn, sizeof(int));
	if (ch == NULL) { //TODO XXX maybe it's a data-cp requested from another node. Check the cache value and decide if cache it.
		if (CONET_DEBUG >= 2)
			fprintf(stderr, "[conet.c:%d] Chunk %u is unexpected return -4 \n",
					__LINE__, (unsigned int) csn);
		return -4;
	}
#endif

	if (CONET_DEBUG == -3) { //whr debug
		fprintf(stderr, "[conet.c inizio process data:");
		conet_print_stats(ce, ch, __LINE__);
	}

	unsigned char chsz_flag = cp_flag >> 4; //first 4 bits of cp_flag contain the chunk size
	if ((unsigned short) (chsz_flag) != CONET_CONTINUATION) {
		if ((unsigned short) (chsz_flag) == CONET_FOLLOW_VAR_CHUNK_SIZE) {
			ce->chunk_size = read_variable_len_number(readbuf, &pos);
		} else {
			ce->chunk_size = 1 << (9 + (unsigned short) chsz_flag); //TODO check (not so useful in ccnx context)
		}

		if (CONET_DEBUG >= 2)
			fprintf(stderr, "[conet.c:%d] Chunk size is: %llu\n", __LINE__,
					ce->chunk_size);

		ch->starting_segment_size = r_edge - l_edge + 1;

#ifdef IS_CACHE_SERVER
		cp_descriptor->chunk_size = ce->chunk_size;
#endif

		if (ch->starting_segment_size < ce->chunk_size) {
			//this is the chunk size less the received bytes in the first data-cp
			unsigned long long reduced_chunk_size = ce->chunk_size
					- ch->starting_segment_size;
			//get the segment size requestable in the next interest extimating variable fields like left and right (in the worst case adapted to received chunk size)
			if (CONET_DEBUG >= 2)
				if (CONET_DEBUG >= 2)
					fprintf(stderr, "[conet.c:%d] m_segment_size was %u\n",
							__LINE__, ch->m_segment_size);
			ch->m_segment_size = get_segment_size(nid, csn, reduced_chunk_size,
					CONET_DEFAULT_MTU);
			if (CONET_DEBUG >= 2)
				if (CONET_DEBUG >= 2)
					fprintf(stderr, "[conet.c:%d] Now m_segment_size is %u\n",
							__LINE__, ch->m_segment_size);

			if (ch->m_segment_size > ch->starting_segment_size) {
				ch->bytes_not_sent_on_starting = ch->m_segment_size
						- ch->starting_segment_size;
			} else {
				ch->bytes_not_sent_on_starting = 0;
			}
#ifdef IS_CACHE_SERVER
			cp_descriptor->bytes_not_sent_on_starting
			= ch->bytes_not_sent_on_starting;
			cp_descriptor->m_segment_size = ch->m_segment_size;
#else
			unsigned int number_of_remaining_interests = (reduced_chunk_size
					/ ch->m_segment_size) + ((reduced_chunk_size
					% ch->m_segment_size > 0) ? 1 : 0); //this is the number of interest we need to send for this chunk less 1 (the already sent one)

			ch->sending_interest_table = realloc(ch->sending_interest_table,
					sizeof(conet_bit) * (number_of_remaining_interests + 1));
			ch->expected = realloc(ch->expected, sizeof(conet_bit)
					* (number_of_remaining_interests + 1));
			ch->out_of_sequence_received = realloc(
					ch->out_of_sequence_received, sizeof(conet_bit)
							* (number_of_remaining_interests + 1));

			ch->bitmap_size += number_of_remaining_interests;

			unsigned int bitmap_it;
			/*setting initial state for new bitmaps positions associated with this chunk*/
			for (bitmap_it = 1; bitmap_it <= number_of_remaining_interests; bitmap_it++) {
				//the first position must not be cleared beacause it has been already used
				if (CONET_DEBUG >= 2)
					fprintf(stderr,
							"[conet.c:%d] ch->sending_interest_table[%u]\n",
							__LINE__, bitmap_it);

				ch->sending_interest_table[bitmap_it].bit = 0;
				ch->expected[bitmap_it].bit = 1;
				ch->out_of_sequence_received[bitmap_it].bit = 0;
			}
#endif
		}
	}

	int seg_index = 0;
#ifndef	IS_CACHE_SERVER //&ct: put the lines of code into if-body
	seg_index = calculate_seg_index(l_edge, r_edge, ch->m_segment_size,
			ch->starting_segment_size, ch->bytes_not_sent_on_starting);
#endif

	unsigned int seg_size = r_edge - l_edge + 1;

#ifdef	IS_CACHE_SERVER
	cp_descriptor->l_edge = l_edge;
	cp_descriptor->segment_size = seg_size;
	memcpy((cp_descriptor->nid + cp_descriptor->nid_length), readbuf + pos,
			cp_descriptor->segment_size);

	int j;
	for (j = 0; j < cp_descriptor->segment_size; j++)
	if (CONET_DEBUG >= 2)
	fprintf(stderr, "%2x ", *(cp_descriptor->nid
					+ cp_descriptor->nid_length + j));
	if (CONET_DEBUG >= 2)
	fprintf(stderr, "\n");
	return ret;
#endif

	if (CONET_DEBUG >= 2) {
		if (ch->sending_interest_table[seg_index].bit == 1) {
			fprintf(stderr,
					"[conet.c:%d] ch->sending_interest_table[%d].bit==1\n",
					__LINE__, seg_index);
		} else {
			fprintf(stderr,
					"[conet.c:%d] ch->sending_interest_table[%d].bit!=1\n",
					__LINE__, seg_index);
		}
	}

	//if the received data corresponds to an interest that was sent beforehand
	if (ch->sending_interest_table[seg_index].bit == 1) {
		if (ch->expected[seg_index].bit == 0) { //Replicated data-cp (it was already received)
			if (CONET_DEBUG >= 2)
				fprintf(
						stderr,
						"[conet.c:%d] Replicated data-cp! Dropped and return -5\n",
						__LINE__);
			return -5;
		} else { //not a replicated data
			if (ce->in_flight > 0) {
				ce->in_flight--;
				if (CONET_DEBUG == -3) { //whr debug
					fprintf(stderr,
							"[conet.c:%d]: Recieved data for %s/%llu /%d \n",
							__LINE__, ce->nid, ch->chunk_number, seg_index);
					fprintf(stderr, "[conet.c:%d] in_flight-- =%d\n", __LINE__,
							ce->in_flight);
				}
			}

#ifdef TCP_BUG_FIX
			//nuova uscita dalla fast_rec
			if (ce->in_fast_rec && ce->recover_chunk == ch->chunk_number
					&& ce->recover_cp == seg_index) {
				if (CONET_DEBUG == -3) {//whr debug
					fprintf(
							stderr,
							"[conet.c:%d]  prima di azzerare ce->out_of_sequence_counter %d \n",
							__LINE__, ce->out_of_sequence_counter);
					fprintf(
							stderr,
							"[conet.c:%d] RESET expcn:%llu , last_in_seq:%d \t last_interest:%d \n",
							__LINE__, ce->chunk_expected->chunk_number,
							ch->last_in_sequence, ch->last_interest);
				}
				ce->in_fast_rec = 0;
				ce->out_of_sequence_counter = 0; //whr
			}
			//nuova uscita fine
#endif

		} //endif not a replicated data

		//check if this segment is a new segment, if it is realloc the chunk and update the current size
		if (r_edge + 1 > ch->current_size) {
			ch->chunk = realloc(ch->chunk, r_edge + 1);
			ch->current_size = r_edge + 1;
		}

#ifdef	IS_CACHE_SERVER
		return ret;//&ct
		}
#else

		memcpy(ch->chunk + l_edge, readbuf + pos, seg_size);

		int chunk_not_in_sequence = 0;
		//check if the chunk arrived is the chunk expected if not set chunk_not_in_sequence flag
		if (ce->chunk_expected != NULL && ce->chunk_expected->chunk_number
				!= ch->chunk_number) {
			chunk_not_in_sequence = 1;
		}

		//if this is the last segment of a chunk set a flag in chunk structure
		if (is_final == 1) {
			ch->final_segment_received = 1;
		}

		if (CONET_DEBUG >= 2)
			fprintf(
					stderr,
					"[conet.c:%d] in_fast_rec:%d last_in_seq:%d \t last_interest:%d \n",
					__LINE__, ce->in_fast_rec, ch->last_in_sequence,
					ch->last_interest);
		ch->expected[seg_index].bit = 0;

		//check if this is the right segment i expect
		if (seg_index == ch->last_in_sequence && chunk_not_in_sequence == 0) {
			//if expected[i]=0 segment is arrived
			if (CONET_DEBUG >= 2)
				fprintf(
						stderr,
						"[conet.c:%d] in_fast_rec:%d last_in_seq:%d \t last_interest:%d is_oos:%d \n",
						__LINE__, ce->in_fast_rec, ch->last_in_sequence,
						ch->last_interest, ch->is_oos);

			if (ce->in_fast_rec == 1 || ch->is_oos) {
				//last in sequence received during fast recovery. Find the new last-in-sequence position
				if (CONET_DEBUG == -3) { //whr debug

					fprintf(
							stderr,
							"[conet.c:%d] in_fast_rec:%d last_in_seq:%d \t last_interest:%d is_oos:%d \n",
							__LINE__, ce->in_fast_rec, ch->last_in_sequence,
							ch->last_interest, ch->is_oos);
					fprintf(
							stderr,
							"[conet.c:%d] -----------, ch->last_in_sequence %llu/%d\n",
							__LINE__, ch->chunk_number, ch->last_in_sequence);
				}
				ch->last_in_sequence++;
#ifndef TCP_BUG_FIX
				ce->out_of_sequence_counter = 0; //whr
#endif
				while (ch->out_of_sequence_received[ch->last_in_sequence].bit
						== 1 && ch->expected[ch->last_in_sequence].bit == 0
						&& ch->last_in_sequence < ch->last_interest) {
					ch->last_in_sequence++;
				}
				if (CONET_DEBUG == -3) //whr debug
					fprintf(
							stderr,
							"[conet.c:%d] >>>>>>>, new ch->last_in_sequence %llu/%d\n",
							__LINE__, ch->chunk_number, ch->last_in_sequence);
				if (CONET_DEBUG >= 2)
					fprintf(
							stderr,
							"[conet.c:%d] in_fast_rec:%d last_in_seq:%d \t last_interest:%d \n",
							__LINE__, ce->in_fast_rec, ch->last_in_sequence,
							ch->last_interest);

#ifndef TCP_BUG_FIX
				if (ch->last_in_sequence == ch->last_interest
						&& ce->chunk_expected->chunk_number == ch->chunk_number) {
					if (CONET_DEBUG == -3)//whr debug
					fprintf(
							stderr,
							"[conet.c:%d] RESET expcn:%llu , last_in_seq:%d \t last_interest:%d \n",
							__LINE__, ce->chunk_expected->chunk_number,
							ch->last_in_sequence, ch->last_interest);

					ce->in_fast_rec = 0;
				}
#endif
				if (seg_index + 1 > ch->last_interest) {
					ch->last_interest = seg_index + 1;
				}
			} else {
				//last in sequence received in normal way update last_interest and last_in_seq
				ch->last_interest++;
				ch->last_in_sequence = ch->last_interest;
			}
			window_control(ce, ch);
			//update last possible interest only if last_possible is under the chunk size
			if (ch->last_possible_interest * ch->m_segment_size
					- (ch->m_segment_size - ch->starting_segment_size)
					<= ce->chunk_size) {
				ch->last_possible_interest = ch->last_interest + ce->cwnd;
			}
		} else { // end check if this is the right segment i expect: here out of sequence data
			if (CONET_DEBUG >= 1)
				fprintf(
						stderr,
						"[conet.c:%d] OUT OF SEQUENCE data received for %s/%llu\n",
						__LINE__, nid, csn);
			if (chunk_not_in_sequence == 0) {
				ch->is_oos = 1;
			}
			conet_oos_stat++;
			ch->out_of_sequence_received[seg_index].bit = 1;
			ce->out_of_sequence_counter++;
			ce->in_fast_rec = 1;

			if (seg_index == ch->last_in_sequence) {
				//recovery of a last in sequence if the chunk is also out of sequence
				ch->last_in_sequence++;
				while (ch->out_of_sequence_received[ch->last_in_sequence].bit
						== 1 && ch->last_in_sequence < ch->last_interest) {
					ch->last_in_sequence++;
				}
			}

			ch->last_interest = (ch->last_interest < seg_index + 1) ? seg_index
					+ 1 : ch->last_interest;
			//if (CONET_DEBUG >= 2) fprintf(stderr, "[conet.c:%d] last_in_seq:%d \t last_interest:%d\t is_final:%d \n", __LINE__, ch->last_in_sequence, ch->last_interest, is_final);
			if (ce->out_of_sequence_counter == 3) {
				window_control(ce, ch);
				if (CONET_DEBUG == -3) //whr debug
					fprintf(
							stderr,
							"[conet.c:%d] 3 out of sequence received call retransmit() , ch->last_in_sequence %llu/%d\n",
							__LINE__, ch->chunk_number, ch->last_in_sequence);

				//				ce->in_fast_rec = 1; //whr spostato

				if (chunk_not_in_sequence != 0) {
					ret += retransmit(h, ce, ce->chunk_expected);
#ifdef TCP_BUG_FIX
					ce->recover_chunk = ce->chunk_expected->chunk_number; //whr qui per reno
					ce->recover_cp = ce->chunk_expected->last_in_sequence;
#endif
					if (CONET_DEBUG == -3) //whr debug
						fprintf(
								stderr,
								"[conet.c:%d] retransmit(h, ce, ce->chunk_expected); %llu/%d\n",
								__LINE__, ce->chunk_expected->chunk_number,
								ce->chunk_expected->last_in_sequence);
#ifndef TCP_BUG_FIX
					int k;
					if (ret <= 0) {
						retrasmit_start = 1;
						for (k = ce->chunk_expected->chunk_number + 1; k
								< ch->chunk_number; k++) {
							struct chunk_entry* ce_oos = hashtb_lookup(
									ce->chunks, &k, sizeof(int));
							if (ce_oos == NULL) {
								if (CONET_DEBUG >= 2)
								fprintf(
										stderr,
										"[conet.c:%d] Chunk %d already deleted \n",
										__LINE__, k);
							} else {
								ret += retransmit(h, ce, ce_oos);
							}
						}
					}
#endif
				} else { //chunk_not_in_sequence == 0
//					retrasmit_start = 1;
					ret += retransmit(h, ce, ch);
					if (CONET_DEBUG == -3) //whr debug
						fprintf(
								stderr,
								"[conet.c:%d] retransmit( chunk_not_in_sequence == 0 \n",
								__LINE__);
#ifdef TCP_BUG_FIX
					ce->recover_chunk = ch->chunk_number; //whr qui per reno
					ce->recover_cp = ch->last_in_sequence;
#endif
				}
#ifdef TCP_BUG_FIX
				if (CONET_DEBUG == -3)
					fprintf(stderr,
							"[conet.c:%d] recover =============== , %llu/%d\n",
							__LINE__, ce->recover_chunk, ce->recover_cp);
#endif
			} else {
				window_control(ce, ch);

				ch->last_possible_interest += ch->last_interest + ce->cwnd;
			}
		}

		if (ch->last_in_sequence > ch->last_interest) {
			ch->last_interest = ch->last_in_sequence;
		}

		if (CONET_DEBUG >= 1)
			fprintf(
					stderr,
					"[conet.c:%d] in_fast_rec:%d last_in_seq:%d \t last_interest:%d \n",
					__LINE__, ce->in_fast_rec, ch->last_in_sequence,
					ch->last_interest);

		/**
		 * first condition is true if final segment is received and there are no out of sequence
		 * second condition is true if final segment is already received and we have received the out of sequence segment
		 */
		//if (CONET_DEBUG >= 2) fprintf(stderr, "\n lis:%d li:%d", ch->last_in_sequence, ch->last_interest);
		if ((is_final == 1 && ch->last_in_sequence == ch->last_interest)
				|| (ch->final_segment_received == 1 && ch->last_in_sequence
						== ch->last_interest)) {

			if (CONET_DEBUG >= 1)
				fprintf(stderr, "[conet.c:%d] chunk %llu completed!\n",
						__LINE__, ch->chunk_number);
			int is_eof_value = is_eof(h, ch->chunk, ch->current_size);
			if (CONET_DEBUG >= 1)
				fprintf(stderr, "[conet.c:%d] is_eof=%d \n", __LINE__,
						is_eof_value);

			if (is_eof_value) {
				ce->completed = 1;
				ce->final_chunk_number = ch->chunk_number;
				close_chunk_transmission(h, ce);
				if (CONET_DEBUG >= 2)
					if (CONET_DEBUG >= 1)
						fprintf(stderr, "[conet.c:%d] last chunk received \n",
								__LINE__);
			} else if (ce->completed == 1) {
				if (CONET_DEBUG >= 2)
					if (CONET_DEBUG >= 1)
						fprintf(
								stderr,
								"[conet.c:%d] last chunk was NOT YET received \n",
								__LINE__);
				close_chunk_transmission(h, ce);
			}

			if (ce->chunk_expected == NULL || (ce->chunk_expected->chunk_number
					== ch->chunk_number && ch->chunk_number
					== ce->chunk_expected->chunk_number)) {
				int next_chunk = ch->chunk_number + 1;
				ce->chunk_expected = hashtb_lookup(ce->chunks, &next_chunk,
						sizeof(int));
				if (ce->chunk_expected == NULL) {
					if (CONET_DEBUG >= 1)
						fprintf(stderr, "[conet.c:%d] No new chunk lcp:%d\n",
								__LINE__, ce->last_processed_chunk);

					while (ce->chunk_expected == NULL && next_chunk
							< ce->last_processed_chunk) {
						next_chunk++;
						ce->chunk_expected = hashtb_lookup(ce->chunks,
								&next_chunk, sizeof(int));

						if (ce->chunk_expected != NULL) {
							if (CONET_DEBUG >= 1)
								fprintf(
										stderr,
										"[conet.c:%d] NEW chunk expected: %llu\n",
										__LINE__,
										ce->chunk_expected->chunk_number);
							break;
						}
					}
				}
				if (ce->chunk_expected == NULL) {
					if (CONET_DEBUG >= 1)
						fprintf(stderr, "[conet.c:%d] No new chunk lcp:%d\n",
								__LINE__, ce->last_processed_chunk);
				}
			}

			conet_send_chunk_to_ccn(h, ch, ch->current_size);
			ch->arrived = 1;

			update_rto(ch, ce);

		}
		if (ce->chunk_expected != NULL)
			if (CONET_DEBUG == -3) //whr debug
				fprintf(
						stderr,
						"[conet.c:%d] hunk_expected %llu/%d , cwnd %d filght %d\n",
						__LINE__, ce->chunk_expected->chunk_number,
						ce->chunk_expected->last_in_sequence, ce->cwnd,
						ce->in_flight);

		//		int remain_to_send = ce->cwnd - ce->in_flight;
		//		int target_chunk_number = 0;
		//		struct chunk_entry* target_ch = ch;
		//chunk is terminated
		//		if (target_ch->arrived==1){
		//			target_chunk_number = chunk_change(h, ce, target_ch, remain_to_send);
		//		}
		//		else {
		//			target_chunk_number=ch->chunk_number;
		//		}


		if (ce->cwnd > ce->in_flight) {
			if (ce->completed == 0) {
				// if the file wait to be completed request further interest
				fill_the_cwnd(h, ce, ch);
			} else {
				//fill the cwnd with retrasmit.
//				retrasmit_start = 1;
				if (ce->chunk_expected == NULL) {
					ce->chunk_expected = ch;
				}
#ifndef TCP_BUG_FIX
				struct chunk_entry* ce_oos;
				int k;
				for (k = ce->chunk_expected->chunk_number; k < ch->chunk_number
						&& ce->cwnd > ce->in_flight; k++) {
					if (k == ce->chunk_expected->chunk_number) {
						ce_oos = ce->chunk_expected;
					} else {
						ce_oos = hashtb_lookup(ce->chunks, &k, sizeof(int));
					}
					if (ce_oos == NULL) {
						if (CONET_DEBUG >= 2)
						fprintf(stderr,
								"[conet.c:%d] Chunk %d already deleted \n",
								__LINE__, k);
					} else {
						ret += retransmit(h, ce, ce_oos); //whr e' giusto??????????????????
						ce->in_flight++;
					}
				}
#endif
			}
		}

		if (CONET_DEBUG >= 1) { //whr debug
			fprintf(stderr, "[conet.c fine process data:");
			conet_print_stats(ce, ch, __LINE__);
		}
	}
	// end if the received data corresponds to an interest that was sent beforehand

	if (ch->arrived == 1) {
		remove_chunk(ce, ch);
	}
	free(nid);
	return ret;
#endif
}
/**
 * Manage the interest carrier packet, if the requested interest exist
 * it send the segment in response for the interest
 * @return number of sent bytes for the interest carrier-packet
 * @return 0 if no interest has been requested (no cache interest)
 * @return -1 if the interest carrier-packet cannot be satisfied
 * @return -2 if an error has occurred in allocating memory
 * @return -3 if an error has occurred during sending data carrier-packet response
 */
int conet_process_interest_cp(struct ccnd_handle* h, char *src_addr,
		unsigned char* readbuf, unsigned short ll, unsigned short c,
		unsigned short ask_info, unsigned short is_raw) { //TODO filter necessary parameters

	char* nid;
	unsigned short nid_length;
	unsigned char* iterator;
	unsigned int is_final = 0;
	int res = 0;

	if (c == CONET_CIU_NOCACHE_FLAG) {
		//simply forward the interest without further analyze
		if (CONET_DEBUG >= 2)
			fprintf(
					stderr,
					"[conet.c:%d] Interest cp has flag 'no cache' forwarding \n",
					__LINE__);
		return 0;
	}
	//iterator on the first byte of NID field
#ifndef CONET_TRANSPORT
	if (is_raw)
	iterator = readbuf + 1 + 2; //ip_opt len +flag + DS&T field
	else
	iterator = readbuf + 1;//caso udp da rivedere
#else
	iterator = readbuf + 1;
#endif
	nid = obtainNID(&iterator, ll);
	nid_length = strlen(nid);
	if (CONET_DEBUG && ll == 2) {
		if (CONET_DEBUG >= 2)
			fprintf(stderr, "[conet.c:%d]with strlen nid_length=%hu, nid= ",
					__LINE__, nid_length);
		int t;
		for (t = 0; t < nid_length; t++)
			if (CONET_DEBUG >= 2)
				fprintf(stderr, "%.2X ", iterator[1 + t]);
		if (CONET_DEBUG >= 2)
			fprintf(stderr, "\n");

	}
	if (ll == 0) {
		//iterator on the first byte of CSN field
		iterator = iterator + nid_length;
	} else if (ll == 2) {
		//iterator on the first byte of CSN field
		if (CONET_DEBUG >= 2)
			fprintf(stderr, "[conet.c:%d]The one byte length field is %.2X\n",
					__LINE__, iterator[0]);
		iterator = iterator + 1 + nid_length;
	} else {
		if (CONET_DEBUG >= 2)
			fprintf(stderr,
					"[CONET]: Using reserved ll-flag in input message. Not yet supported\n");
		//TODO handle this case
		abort();
	}

	int pos = iterator - readbuf;
	if (CONET_DEBUG >= 2)
		fprintf(stderr,
				"[conet.c:%d]ll=%hu,pos=%d,with strlen nid_length=%hu\n",
				__LINE__, ll, pos, nid_length);
	if (CONET_DEBUG >= 2)
		fprintf(stderr, "[conet.c:%d]Reading csn\n", __LINE__);

	unsigned long long csn = read_variable_len_number(readbuf, &pos);
	if (CONET_DEBUG >= 2)
		fprintf(
				stderr,
				"[conet.c:%d]After reading csn pos=%d and next bytes are %2X %2X %2X %2X\n",
				__LINE__, pos, readbuf[pos + 0], readbuf[pos + 1], readbuf[pos
						+ 2], readbuf[pos + 3]);
#ifndef CONET_TRANSPORT
	//Skip the "End of ip option list" character
	if (is_raw) //da controllare
	pos = (int) readbuf[0] - 1; // - ip_opt len
#endif

	//&ct:inizio
	if (CONET_DEBUG >= 2)
		fprintf(
				stderr,
				"[conet.c:%d]After esacpin null chars, pos=%d and next bytes are %2X %2X %2X %2X\n",
				__LINE__, pos, readbuf[pos + 0], readbuf[pos + 1], readbuf[pos
						+ 2], readbuf[pos + 3]);

#ifdef	IS_CACHE_SERVER
	if (CONET_DEBUG == 1)
	if (CONET_DEBUG >= 2)
	fprintf(stderr, "[conet.c: %d] Reading tag\n", __LINE__);//&ct
	int u;
	for (u = 0; u < 4; u++)
	cp_descriptor->tag[u] = readbuf[pos + u];
#endif
	//&ct:fine
#ifndef CONET_TRANSPORT
#ifdef IPOPT
	if (is_raw) {
		pos += 4; //jump 4 byte openflow tag
	}
#endif
#endif
	if (CONET_DEBUG >= 2)
		fprintf(
				stderr,
				"[conet.c:%d]Reading cp_flag, pos=%d and next bytes are %2X %2X %2X %2X\n",
				__LINE__, pos, readbuf[pos + 0], readbuf[pos + 1], readbuf[pos
						+ 2], readbuf[pos + 3]);

	unsigned char cp_flag = (unsigned char) *(readbuf + pos);
	if (CONET_DEBUG >= 2)
		fprintf(stderr, "[conet.c:%d]cp_flag=%2X, readbuf[%d]=%2X\n", __LINE__,
				pos, cp_flag, readbuf[pos]);

	pos++;

	if ((cp_flag & CONET_ASK_CHUNK_INFO_FLAG) != 0) {
		ask_info = 1;
	}

	if (CONET_DEBUG >= 2)
		fprintf(
				stderr,
				"[conet.c:%d]Reading l_edge, pos=%d. The first bytes from pos are %2X %2X\n",
				__LINE__, pos, readbuf[pos], readbuf[pos + 1]);
	unsigned long long l_edge = read_variable_len_number(readbuf, &pos);
	//if (CONET_DEBUG >= 2) fprintf(stderr, "[CONET]: left edge:%u\n", (unsigned int)l_edge);

	if (CONET_DEBUG >= 2)
		fprintf(
				stderr,
				"[conet.c:%d]Reading r_edge, pos=%d. The first byte from pos are %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X %2X\n",
				__LINE__, pos, readbuf[pos], readbuf[pos + 1],
				readbuf[pos + 2], readbuf[pos + 3], readbuf[pos + 4],
				readbuf[pos + 5], readbuf[pos + 6], readbuf[pos + 7],
				readbuf[pos + 8], readbuf[pos + 9], readbuf[pos + 10],
				readbuf[pos] + 11);
	unsigned long long r_edge = read_variable_len_number(readbuf, &pos);
	//if (CONET_DEBUG >= 2) fprintf(stderr, "[CONET]: right edge:%u\n", (unsigned int)r_edge);

	if (CONET_DEBUG >= 2)
		fprintf(
				stderr,
				"[conet.c:%d]: Received an interest for %s/%llu left edge:%llu, right edge:%llu \n",
				__LINE__, nid, csn, l_edge, r_edge);

	//&ct
#ifdef	IS_CACHE_SERVER
	cp_descriptor->type = INTEREST_DESCRIPTOR;
	cp_descriptor->nid_length = nid_length;
	memcpy(cp_descriptor->nid, nid, cp_descriptor->nid_length);
	cp_descriptor->csn = csn;
	cp_descriptor->l_edge = l_edge;
	cp_descriptor->segment_size = (r_edge - l_edge) + 1;
	cp_descriptor->flags = cp_flag;
	return res;
#endif
	//fine &ct

	if (CONET_LOG_TO_FILE) {
		if (out_file_ri == 0) {
			out_file_ri = ((unsigned int) fopen("out_ri.csv", "wt"));
		}
		log_to_file((FILE*)out_file_ri, csn, l_edge, r_edge);
	}
	char *uri = calloc(255, sizeof(char));
	sprintf(uri, "%s/%llu", nid, csn);

	//if (CONET_DEBUG >= 2) fprintf(stderr, "[conet.c:%d]: Search %s in cache \n", __LINE__,uri );


	struct ccn_charbuf *name = ccn_charbuf_create();
	struct ccn_charbuf *name_dec = ccn_charbuf_create();

	int res0 = ccn_name_from_uri(name, nid);
	res0 += ccn_name_append_numeric(name, CCN_MARKER_SEQNUM, csn);
	res0 += ccn_name_from_uri(name_dec, uri);

	struct content_entry* content_e = NULL;
	int not_cached = 0;
	content_e = conet_cache_check(nid, csn);
	if (content_e == NULL) {
		not_cached = 1;
		content_e = find_ccn_content(h, name);
		if (content_e == NULL || content_e->size - 36 == 0) {
			if (CONET_DEBUG >= 2)
				fprintf(
						stderr,
						"[conet.c:%d]: No content found for %s/%llu. Dropped\n",
						__LINE__, nid, csn);
			if (CONET_SEVERE_DEBUG)
				exit(-661);
			//start border node processing
			if (CONET_FORWARDING) {
				//				if (CONET_DEBUG >= 2) per adesso questa parte va tolta
				//					fprintf(stderr, "[conet.c:%d]: EXITING !!!\n", __LINE__);
				//				abort(); // so that we do not enter in a not tested part !!!
				//				unsigned char * msg_ONLY_FOR_SIGNATURE;
				//				return propagate_interest_cp(h, name, msg_ONLY_FOR_SIGNATURE);
			} else {
				ccn_charbuf_destroy(&name);
				ccn_charbuf_destroy(&name_dec);
				free(nid);
				free(uri);
				return -1;
			}
		}

		if (content_match(content_e, name) != 0) {
			//if(CONET_SEVERE_DEBUG) exit(-999);//andrea.araldo@gmail.com: //TODO:che cosa e' questo matching?

			if (CONET_DEBUG >= 2)
				fprintf(
						stderr,
						"[conet.c:%d]: Prefix mismatch. Fallback in chunk in decimal mode\n",
						__LINE__);
			content_e = find_ccn_content(h, name_dec);
			if (content_e == NULL || content_e->size - 36 == 0) {
				if (CONET_DEBUG >= 2)
					fprintf(
							stderr,
							"[conet.c:%d]: Decimal mode no content found for %s/%llu. Dropped\n",
							__LINE__, nid, csn);
				ccn_charbuf_destroy(&name);
				ccn_charbuf_destroy(&name_dec);
				free(nid);
				free(uri);
				return -1;
			}
			if (content_match(content_e, name_dec) != 0) {
				if (CONET_DEBUG >= 2)
					fprintf(
							stderr,
							"[conet.c:%d]:Decimal mode prefix mismatch. Dropped\n",
							__LINE__);
				ccn_charbuf_destroy(&name);
				ccn_charbuf_destroy(&name_dec);
				free(nid);
				free(uri);
				return -1;
			}
		}

	}

	if (not_cached) {
		if (cache == NULL) {
			cache = calloc(1, sizeof(struct conet_content_cache));
		}
		if (cache->name != NULL) {
			free(cache->name);
		}
		cache->name = NULL;
		cache->chunk_number = -1;
		cache->ccn_content = NULL;
		cache->name = calloc(1, strlen(nid) + 1);
		strcpy(cache->name, nid);
		cache->chunk_number = csn;
		cache->ccn_content = content_e;
	}

	//ccnb digest removal
	int d_start, d_end;
	/* Excise the message-digest name component */
	int n = content_e->ncomps;
	if (n < 2)
		abort();
	d_start = content_e->comps[n - 2];
	d_end = content_e->comps[n - 1];
	if (d_end - d_start != 36) {
		/* strange digest length */
		abort();
	}

	int segment_size = r_edge - l_edge + 1;
	int content_size = content_e->size - 36;

	if (content_size - l_edge < segment_size) {
		//end of chunk with a dimension different from a multiple of segment-dimension reached
		if (content_size == 0) {
			r_edge = 0;
		} else {
			r_edge = content_size - 1;
			is_final = 1;
		}
		segment_size = r_edge - l_edge + 1;
	}

	if (content_size - (int) l_edge < 0) {
		if (CONET_DEBUG >= 2)
			fprintf(
					stderr,
					"[conet.c:%d]: The segment is shorter than this request. Drop\n",
					__LINE__);
		ccn_charbuf_destroy(&name);
		ccn_charbuf_destroy(&name_dec);
		free(nid);
		free(uri);
		return -1;
	}
	unsigned char * segment = calloc(segment_size, sizeof(unsigned char));

	if (segment == NULL) {
		if (CONET_DEBUG >= 2)
			fprintf(
					stderr,
					"[conet.c:%d]: Error allocating memory for this segment \n",
					__LINE__ );
		ccn_charbuf_destroy(&name);
		ccn_charbuf_destroy(&name_dec);
		free(nid);
		free(uri);
		return -2;
	}
	//if (CONET_DEBUG >= 2) fprintf(stderr, "[conet.c:%d]: Copying segment from %llu for %d byte \n", __LINE__, l_edge, segment_size );

	if (r_edge < d_start) {
		memcpy(segment, content_e->key + l_edge, segment_size);
	} else if (r_edge > d_start && l_edge < d_start) {
		memcpy(segment, content_e->key + l_edge, d_start - l_edge);
		memcpy(segment + (d_start - l_edge), content_e->key + d_end,
				segment_size - (d_start - l_edge));

	} else {
		memcpy(segment, content_e->key + l_edge + 36, segment_size);
	}
	//	memcpy(segment, cc->content_nodigest+l_edge, segment_size);

	res = conet_send_data_cp(src_addr, content_size, segment, segment_size,
			nid, csn, l_edge, r_edge, is_final, ask_info, is_raw);

	if (res < 0) {
		if (CONET_DEBUG >= 2)
			fprintf(
					stderr,
					"[conet.c:%d]: Error in conet_send_data_cp() returned %d \n",
					__LINE__, res);

		free(segment);
		ccn_charbuf_destroy(&name);
		ccn_charbuf_destroy(&name_dec);
		free(nid);
		free(uri);
		return -3;
	}

	ccn_charbuf_destroy(&name);
	ccn_charbuf_destroy(&name_dec);
	free(nid);
	free(uri);
	free(segment);
	return res;
}
/**
 * Send the segment back in response to the interest
 */
int udp_sock = 0; //possiamo evitare queste variabili globali
int udp_sock_ipv6 = 0;
int raw_sock = 0;

int conet_send_data_cp(char* src_addr, int chunk_size, unsigned char * segment,
		unsigned int segment_size, const char* nid, unsigned long long csn,
		unsigned int l_edge, unsigned int r_edge, unsigned int is_final,
		unsigned int add_chunk_info, unsigned short use_raw) {

	unsigned char* packet = NULL;
	unsigned char cp_flag = CONET_CONTINUATION << 4;

	/*setting-up flags*/
	if (add_chunk_info == 1) {
		//first data-CP response
		//TODO vedere se potenza di 2->codificare con enum altrimenti segue...
		cp_flag = CONET_FOLLOW_VAR_CHUNK_SIZE << 4;
	}
	if (is_final == 1) {
		cp_flag = cp_flag | CONET_FINAL_SEGMENT_FLAG;
	}
	//	unsigned int pckt_size = build_carrier_packet(&packet,CONET_NAMED_DATA_CIU_TYPE_FLAG, CONET_CIU_CACHE_FLAG, nid, csn, l_edge, r_edge, cp_flag, chunk_size); //TODO choose yes/no-CACHE
	//if packt_size is over the MTU refresh the packet with a reduced segment
	unsigned int pckt_size = 0;
	unsigned int ipoption_size;

	if (use_raw) {
		//questa define si puo togliere nho
#ifdef RAW_IPV6
		int family;
		if (strstr(src_addr, "::"))
		family = AF_INET6; // cercare una migliore definizione
		else
		family = AF_INET;
		pckt_size = build_ip_option(&packet, CONET_NAMED_DATA_CIU_TYPE_FLAG,
				CONET_CIU_CACHE_FLAG, nid, csn, family);
#else
#ifndef CONET_TRANSPORT
		pckt_size = build_ip_option(&packet, CONET_NAMED_DATA_CIU_TYPE_FLAG,
				CONET_CIU_CACHE_FLAG, nid, csn, AF_INET);
#endif
#endif
		ipoption_size = pckt_size;
		if (CONET_DEBUG >= 2)
			fprintf(stderr,
					"!!![conet.c: %d]: sto chiamando set_open_flow_tag",
					__LINE__);//&CT
#ifdef IPOPT
		pckt_size = set_openflow_tag(&packet, pckt_size,
				CONET_NAMED_DATA_CIU_TYPE_FLAG, nid, csn);
#endif
		pckt_size = build_carrier_packet(&packet, pckt_size,
				CONET_NAMED_DATA_CIU_TYPE_FLAG, CONET_CIU_CACHE_FLAG, nid, csn,
				l_edge, r_edge, cp_flag, chunk_size);

		if (pckt_size + segment_size > CONET_DEFAULT_MTU) {
			int diff = (pckt_size + segment_size) - CONET_DEFAULT_MTU;
			r_edge = r_edge - diff;
			segment_size = r_edge - l_edge + 1;
			if (packet != NULL) {
				free(packet);
			}
			packet = NULL;
			//define che si puo togliere
			//inserire parametro per distinguere famiglia
#ifdef RAW_IPV6
			pckt_size = build_ip_option(&packet,
					CONET_NAMED_DATA_CIU_TYPE_FLAG, CONET_CIU_CACHE_FLAG, nid,
					csn, family);
#else
#ifndef CONET_TRANSPORT
			pckt_size = build_ip_option(&packet,
					CONET_NAMED_DATA_CIU_TYPE_FLAG, CONET_CIU_CACHE_FLAG, nid,
					csn, AF_INET);
#endif
#endif
#ifdef IPOPT
			pckt_size = set_openflow_tag(&packet, pckt_size,
					CONET_NAMED_DATA_CIU_TYPE_FLAG, nid, csn);
#endif
			pckt_size = build_carrier_packet(&packet, pckt_size,
					CONET_NAMED_DATA_CIU_TYPE_FLAG, CONET_CIU_CACHE_FLAG, nid,
					csn, l_edge, r_edge, cp_flag, chunk_size);
		}
		packet = realloc(packet, pckt_size + segment_size);
		memcpy(packet + pckt_size, segment, segment_size);
	} else {
		pckt_size = build_carrier_packet(&packet,0,
				CONET_NAMED_DATA_CIU_TYPE_FLAG, CONET_CIU_CACHE_FLAG, nid, csn,
				l_edge, r_edge, cp_flag, chunk_size); //TODO choose yes/no-CACHE
		//if packt_size is over the MTU refresh the packet with a reduced segment
		if (pckt_size + segment_size > CONET_DEFAULT_MTU) {
			int diff = (pckt_size + segment_size) - CONET_DEFAULT_MTU;
			r_edge = r_edge - diff;
			segment_size = r_edge - l_edge + 1;
			if (packet != NULL) {
				free(packet);
			}
			packet = NULL;
			pckt_size = build_carrier_packet(&packet,0,
					CONET_NAMED_DATA_CIU_TYPE_FLAG, CONET_CIU_CACHE_FLAG, nid,
					csn, l_edge, r_edge, cp_flag, chunk_size); //TODO choose yes/no-CACHE
		}
		packet = realloc(packet, pckt_size + segment_size);
		memcpy(packet + pckt_size, segment, segment_size);
	}
	size_t packet_size = (size_t) (pckt_size + segment_size);

	if (CONET_DEBUG >= 2)
		fprintf(
				stderr,
				"[conet.c: %d] pckt_size(%d)+segment_size(%d)=packet_size(%d); chunk_size=%d,nid=%s,l_edge=%u, r_edge=%u \n",
				__LINE__, pckt_size, segment_size, packet_size, chunk_size,
				nid, l_edge, r_edge);
	/**
	 * Send via udp socket TODO XXX next step is to avoid creating a socket everytime
	 */
	/* Create the UDP socket */
	int sock;

	struct sockaddr* to_addr;
	struct sockaddr_storage next_hop_storage;
	int addr_size = 0;

	memset(&next_hop_storage, 0, sizeof(next_hop_storage));

	if (strstr(src_addr, "::")) //porcata per vedere la family
		next_hop_storage.ss_family = AF_INET6;
	else
		next_hop_storage.ss_family = AF_INET;

	if (next_hop_storage.ss_family == AF_INET) {
		//si puo' accorciare assegnamento diretto e casting nho
		struct sockaddr_in *sin;
		sin = (struct sockaddr_in *) &next_hop_storage;
		sin->sin_addr.s_addr = inet_addr(src_addr); /* IP address */
		sin->sin_port = htons(atoi(CONET_DEFAULT_UNICAST_PORT)); /* server port */
	} else {
		struct sockaddr_in6* sin6;
		sin6 = (struct sockaddr_in6 *) &next_hop_storage;
		inet_pton(AF_INET6, src_addr, sin6->sin6_addr.s6_addr); /* IP address */

		sin6->sin6_port = htons(atoi(CONET_DEFAULT_UNICAST_PORT)); /* server port */
	}

	addr_size =20;//solo ipv4 adattare anche ad ipv6 sizeof(next_hop_storage);
	to_addr = (struct sockaddr*) &next_hop_storage;

	if (!use_raw) {
		if (next_hop_storage.ss_family == AF_INET) {
			if (udp_sock <= 0) {
				if ((udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
					perror("[CONET]: Failed to create socket");
					//TODO
					free(packet);
					return -1;
				}
			}
			sock = udp_sock;
		} else {
			if (udp_sock_ipv6 <= 0) {
				if ((udp_sock_ipv6 = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP))
						< 0) {
					perror("[CONET]: Failed to create ipv6 socket");
					//TODO
					free(packet);
					return -1;
				}
			}
			sock = udp_sock_ipv6;
		}
	} else { //use_raw
		if (raw_sock <= 0) {
			if (CONET_DEBUG >= 2)
				fprintf(stderr, "[conet.c:%d]create a RAW socket\n", __LINE__);
			if ((raw_sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
				perror("[CONET]: Failed to create RAW socket");
				free(packet);
				return -1;
			}
			if (CONET_DEBUG >= 2)
				fprintf(stderr, "[conet.c:%d] RAW socket: %d created\n",
						__LINE__, raw_sock);

			//			int tmp = 1;
			//			const int *val = &tmp;
			//			if (setsockopt(raw_sock, IPPROTO_IP, IP_HDRINCL, val, sizeof(tmp)) < 0) {
			//				if (CONET_DEBUG >= 2) fprintf(stderr,"Error: setsockopt() - Cannot set HDRINCL:\n");
			//				return 1;
			//			}
			//			if ((bind2if(sock))<0){
			//					if (CONET_DEBUG >= 2) fprintf(stderr, "[CONET]: Failed to bind socket to interface %s", CONET_IFNAME);
			//				}
		}
		sock = raw_sock;

	}

	///we're in conet_send_data_cp
	if (use_raw) {
		// DA PULIRE ASSOLUTAMENTE, DEFINE INUTILE CI VA UN IF ,NHO
#ifdef RAW_IPV6
		if (next_hop_storage.ss_family == AF_INET) {
#endif
		struct sockaddr_in next_hop; // togliere quando ci sara' ipv6 raw
		//struct sockaddr_storage next_hop;
		memset(&next_hop, 0, sizeof(next_hop)); /* Clear struct */
		next_hop.sin_family = AF_INET; /* Internet/IP */
		next_hop.sin_addr.s_addr = inet_addr(src_addr); /* IP address */
		next_hop.sin_port = htons(atoi(CONET_DEFAULT_UNICAST_PORT)); /* server port */
		addr_size = sizeof(next_hop);
		to_addr = (struct sockaddr*) &next_hop;
		//		next_hop = (struct sockaddr_in *) &next_hop_storage; // togliere quando ci sara' ipv6 raw
		//unsigned char* old_packet=packet;
		if (CONET_DEBUG >= 2)
			fprintf(stderr, "\n SENDING (RAW) DATA TO %s\n", src_addr);
		packet = setup_ipeth_headers(NULL, &next_hop, packet, &packet_size,
				ipoption_size);
		to_addr = setup_raw_sockaddr(&next_hop);
		//addr_size=20;//sizeof(next_hop);
		//memccpy(packet+18+20, old_packet, packet_size, 1);
		// 14 header eth + 20 header IP + 4 VLAN ??
		//packet_size+=34 + ((TAG_VLAN)?4:0); already done in setup_ipeth_headers
		addr_size = 20;//solo ipv4 adattare anche ad ipv6
		//if (CONET_DEBUG >= 2) fprintf(stderr, "\n 1 packet size:%d \n", packet_size);
		//conet_print_ccnb(packet, packet_size);
#ifdef RAW_IPV6
	} else if (next_hop_storage.ss_family == AF_INET6) {
		struct sockaddr_in6 next_hop;
		memset(&next_hop, 0, sizeof(next_hop)); /* Clear struct */
		next_hop.sin6_family = AF_INET6; /* Internet/IP */
		inet_pton(AF_INET6, src_addr, &next_hop.sin6_addr.s6_addr); /* IP address */
		next_hop.sin6_port = htons(atoi(CONET_DEFAULT_UNICAST_PORT)); /* server port */
		to_addr = (struct sockaddr*) &next_hop;
		//		next_hop = (struct sockaddr_in *) &next_hop_storage; // togliere quando ci sara' ipv6 raw
		//unsigned char* old_packet=packet;
		if (CONET_DEBUG >= 1)
		fprintf(stderr, "\n SENDING (RAW ipv6) DATA TO %s\n", src_addr);
		packet = setup_ipeth_headers(NULL, &next_hop, packet, &packet_size,
				ipoption_size);
		to_addr = setup_raw_sockaddr(&next_hop);
		//addr_size=20;//sizeof(next_hop);
		//memccpy(packet+18+20, old_packet, packet_size, 1);
		// 14 header eth + 20 header IP + 4 VLAN ??
		//packet_size+=34 + ((TAG_VLAN)?4:0); already done in setup_ipeth_headers
		addr_size = sizeof(next_hop);
		//if (CONET_DEBUG >= 2) fprintf(stderr, "\n 1 packet size:%d \n", packet_size);
		//conet_print_ccnb(packet, packet_size);
	}
#endif
	}

	if ((CONET_DEBUG >= 2) && use_raw == 1)
		fprintf(stderr, "sending data to socket raw: %d packet_size: %d\n",
				sock, packet_size);
	if (CONET_DEBUG && use_raw != 1)
		//		fprintf(stderr, "sending data to socket plain: %d\n", sock);

		if ((CONET_DEBUG >= 2) && use_raw == 1) {

			fprintf(stderr, "family: %d ",
					((struct sockaddr_ll*) to_addr)->sll_family);
			fprintf(stderr, "protocol: %d ",
					((struct sockaddr_ll*) to_addr)->sll_protocol);
			fprintf(stderr, "if_index: %d ",
					((struct sockaddr_ll*) to_addr)->sll_ifindex);
			fprintf(stderr, "hatype: %d ",
					((struct sockaddr_ll*) to_addr)->sll_hatype);
			fprintf(stderr, "pkttype: %d ",
					((struct sockaddr_ll*) to_addr)->sll_pkttype);
			fprintf(stderr, "halen %d ",
					((struct sockaddr_ll*) to_addr)->sll_halen);
			fprintf(stderr, "dest MAC: ");
			int i;
			for (i = 0; i < 6; i++) {
				fprintf(stderr, "%02X ",
						((struct sockaddr_ll*) to_addr)->sll_addr[i]);
			}
			fprintf(stderr, "\n");

			if (CONET_DEBUG == 2) {
				fprintf(stderr, "Sending bytes:\n");
				for (i = 0; i < packet_size; i++) {
					if (i % 16 == 0) {
						fprintf(stderr, "%04X  ", i);
					}
					fprintf(stderr, "%02X ", packet[i]);
					if (i % 16 == 15)
						fprintf(stderr, "\n");
				}
				fprintf(stderr, "\n");
			}
		}

	//we're in conet_send_data_cp
	int sent = sendto(sock, packet, packet_size, 0, to_addr, addr_size);
	if (CONET_LOG_TO_FILE) {
		if (out_file_sd == 0) {
			out_file_sd = (unsigned int) fopen("out_sd.csv", "wt");
		}
		log_to_file((FILE*)out_file_sd, csn, l_edge, r_edge);
	}
	//if (CONET_DEBUG >= 2) fprintf(stderr, "[conet.c:%d]: Sent %d byte \n", __LINE__, sent );

	if (sent != packet_size) {
		perror("[CONET]: Mismatch in number of sent bytes");
		if (sent == -1)
			if (CONET_DEBUG >= 2)
				fprintf(stderr, "[conet.c:%d]: Sendto error %s \n", __LINE__,
						strerror(errno));
		udp_sock = 0;
		udp_sock_ipv6 = 0;
		raw_sock = 0;
		free(packet);
		close(sock);
		if (use_raw)
			free(to_addr);
		return -1;
	}

	free(packet);
	if (use_raw)
		free(to_addr);
	return sent;

}

/**
 * 2012 05 20 SS: looks like setup_ip_header is never used !

 unsigned char* setup_ip_header(struct sockaddr_in* this_hop,
 struct sockaddr_in* next_hop, unsigned char * packet,
 size_t packet_size, size_t ipoption_size) {
 unsigned char* buffer = calloc(1, packet_size + sizeof(struct iphdr));
 struct iphdr *ip = (struct iphdr *) buffer;

 ip->ihl = 5 + ipoption_size / 4; //riempio la struttura IP

 ip->version = 4;
 ip->tos = 16;
 ip->tot_len = sizeof(struct iphdr) + ipoption_size;
 ip->id = htons(52407);
 ip->frag_off = 0;
 ip->ttl = 65;
 ip->protocol = CONET_PROTOCOL_NUMBER;
 ip->check = 0; //per ora setto a 0 poi calcolero' quando conoscerÃ² la dimensione del pacchetto TCP
 ip->saddr = (this_hop == NULL) ? inet_addr(CONET_CLIENT_IP)
 : this_hop->sin_addr.s_addr;
 ip->daddr = next_hop->sin_addr.s_addr;
 //	if (CONET_DEBUG >= 2) fprintf(stderr, "RAW");
 memcpy(buffer + sizeof(struct iphdr), packet, packet_size);
 return buffer;
 }
 */
/**
 * sets the raw socket address (only used for sending out raw packets)
 */
#ifdef RAW_IPV6
// FARNE UNA UNICA  NHO
struct sockaddr* setup_raw_sockaddr(struct sockaddr_storage* next_hop) {

	struct sockaddr_ll* sock_addr = calloc(1, sizeof(struct sockaddr_ll));

	//	unsigned char dst_mac[6] = { 0x00, 0x1b, 0x38, 0x45, 0xae, 0x3e };
	int if_index = 0;
	unsigned char dst_mac[6] = DEST_MAC_ADDR; //intanto e' hardcodato

	//porcata per beccare l'indice della interfaccia in ipv4 invece che in6 whr
	//da togliere quando get_mac_from_ip() sara' anche ipv6
	//	unsigned char dst_mac2[6];
	//	memset(dst_mac2, 0, 6);
	//	struct sockaddr_in ip4addr;
	//	ip4addr.sin_family = AF_INET;
	//	inet_pton(AF_INET, SERV_IP_ADDR, &ip4addr.sin_addr);
	//	int ret_mac = get_mac_from_ip(dst_mac2, &ip4addr, &if_index);
	// fine porcata


	//		if (ret_mac != 0) {
	//			if (CONET_DEBUG >= 1)
	//				fprintf(stderr,
	//						"error in setup_raw_sockaddr, unable to get dst MAC address\n");
	//			return NULL;
	//		}
	if (CONET_DEBUG >= 1) {
		if (CONET_DEBUG >= 1)
		fprintf(stderr,
				"conet.c[%d] setup_raw_sockaddr() interface index: %d \n",
				__LINE__, if_index);
		if (CONET_DEBUG >= 1)
		fprintf(stderr, "DEST MAC: ");
		int i;
		for (i = 0; i < 6; i++) {
			if (CONET_DEBUG >= 1)
			fprintf(stderr, "%02X ", dst_mac[i]);
		}
		if (CONET_DEBUG >= 1)
		fprintf(stderr, "\n");
	}

	//unsigned char src_mac[6] = SOURCE_MAC_ADDR;

	//see http://linux.about.com/library/cmd/blcmdl7_packet.htm

	// When you send packets it is enough to specify
	// sll_family, sll_addr, sll_halen, sll_ifindex.
	// The other fields should be 0.
	// sll_hatype and sll_pkttype are set on received packets for your information.
	// For bind only sll_protocol and sll_ifindex are used.

	sock_addr->sll_family = AF_PACKET;
	sock_addr->sll_protocol = 0; // 0x0800 should be 0
	sock_addr->sll_ifindex = 2; //per eth0 = 2 eth1 = 3 eth2 = 4 almeno dovrebbe
	sock_addr->sll_hatype = 0; //should be 0
	sock_addr->sll_pkttype = 0; //should be 0

	// matteo cancellieri had also set other fields that are not useful
	//		sock_addr->sll_family = AF_PACKET;
	//		sock_addr->sll_protocol = htons(ETH_P_IP);  // 0x0800 should be 0
	//		sock_addr->sll_ifindex = if_index;
	//		sock_addr->sll_hatype = ARPHRD_ETHER;       //should be 0
	//		sock_addr->sll_pkttype = PACKET_OTHERHOST;  //should be 0

	sock_addr->sll_halen = ETH_ALEN;
	sock_addr->sll_addr[0] = dst_mac[0];
	sock_addr->sll_addr[1] = dst_mac[1];
	sock_addr->sll_addr[2] = dst_mac[2];
	sock_addr->sll_addr[3] = dst_mac[3];
	sock_addr->sll_addr[4] = dst_mac[4];
	sock_addr->sll_addr[5] = dst_mac[5];
	sock_addr->sll_addr[6] = 0x00;
	sock_addr->sll_addr[7] = 0x00;
	//	sock_addr->sll_addr[0] = 0x80;
	//	sock_addr->sll_addr[1] = 0x00;
	//	sock_addr->sll_addr[2] = 0x27;
	//	sock_addr->sll_addr[3] = 0x47;
	//	sock_addr->sll_addr[4] = 0xbb;
	//	sock_addr->sll_addr[5] = 0xf8;
	//	sock_addr->sll_addr[6] = 0x00;
	//	sock_addr->sll_addr[7] = 0x00;

	return (struct sockaddr*) sock_addr;

}
#else  //IPV4
struct sockaddr* setup_raw_sockaddr(struct sockaddr_in* next_hop) {
	struct sockaddr_ll* sock_addr = calloc(1, sizeof(struct sockaddr_ll));

	//	unsigned char dst_mac[6] = { 0x00, 0x1b, 0x38, 0x45, 0xae, 0x3e };
	int if_index = 0;
	unsigned char dst_mac[6];
	memset(dst_mac, 0, 6);

	int ret_mac = get_mac_from_ip(dst_mac, next_hop, &if_index);
	if (ret_mac != 0) {
		if (CONET_DEBUG >= 2)
			fprintf(stderr,
					"error in setup_raw_sockaddr, unable to get dst MAC address\n");
		return NULL;
	}
	if (CONET_DEBUG >= 2) {
		if (CONET_DEBUG >= 2)
			fprintf(stderr,
					"conet.c[%d] setup_raw_sockaddr() interface index: %d \n",
					__LINE__, if_index);
		if (CONET_DEBUG >= 2)
			fprintf(stderr, "DEST MAC: ");
		int i;
		for (i = 0; i < 6; i++) {
			if (CONET_DEBUG >= 2)
				fprintf(stderr, "%02X ", dst_mac[i]);
		}
		if (CONET_DEBUG >= 2)
			fprintf(stderr, "\n");
	}

	//unsigned char src_mac[6] = SOURCE_MAC_ADDR;

	//see http://linux.about.com/library/cmd/blcmdl7_packet.htm

	// When you send packets it is enough to specify
	// sll_family, sll_addr, sll_halen, sll_ifindex.
	// The other fields should be 0.
	// sll_hatype and sll_pkttype are set on received packets for your information.
	// For bind only sll_protocol and sll_ifindex are used.

	sock_addr->sll_family = AF_PACKET;
	sock_addr->sll_protocol = 0; // 0x0800 should be 0
	sock_addr->sll_ifindex = if_index;
	sock_addr->sll_hatype = 0; //should be 0
	sock_addr->sll_pkttype = 0; //should be 0

	// matteo cancellieri had also set other fields that are not useful
	//		sock_addr->sll_family = AF_PACKET;
	//		sock_addr->sll_protocol = htons(ETH_P_IP);  // 0x0800 should be 0
	//		sock_addr->sll_ifindex = if_index;
	//		sock_addr->sll_hatype = ARPHRD_ETHER;       //should be 0
	//		sock_addr->sll_pkttype = PACKET_OTHERHOST;  //should be 0

	sock_addr->sll_halen = ETH_ALEN;
	sock_addr->sll_addr[0] = dst_mac[0];
	sock_addr->sll_addr[1] = dst_mac[1];
	sock_addr->sll_addr[2] = dst_mac[2];
	sock_addr->sll_addr[3] = dst_mac[3];
	sock_addr->sll_addr[4] = dst_mac[4];
	sock_addr->sll_addr[5] = dst_mac[5];
	sock_addr->sll_addr[6] = 0x00;
	sock_addr->sll_addr[7] = 0x00;
	//	sock_addr->sll_addr[0] = 0x80;
	//	sock_addr->sll_addr[1] = 0x00;
	//	sock_addr->sll_addr[2] = 0x27;
	//	sock_addr->sll_addr[3] = 0x47;
	//	sock_addr->sll_addr[4] = 0xbb;
	//	sock_addr->sll_addr[5] = 0xf8;
	//	sock_addr->sll_addr[6] = 0x00;
	//	sock_addr->sll_addr[7] = 0x00;

	return (struct sockaddr*) sock_addr;

}
#endif
/**
 * it returns the interface name and sets the in/out ifindex parameter to the corresponding interface index
 * it returns NULL if it can't find the interface
 */
char* get_ifname_for_ip(int sock, struct in_addr ipaddr, int* ifindex) {
	struct ifreq ifr;
	in_addr_t addr, netmask;
	int i;
	char *ifname;

	for (i = 0; i < 255; i++) {
		memset(&ifr, 0, sizeof(ifr));
		ifr.ifr_ifindex = i;
		if (ioctl(sock, SIOCGIFNAME, &ifr) == -1) {
			continue;
		}

		if (ioctl(sock, SIOCGIFFLAGS, &ifr) == -1) {
			if (CONET_DEBUG >= 2)
				fprintf(
						stderr,
						"ioctl(SIOCGIFFLAGS) for interface \"%s\" failed (%d): %s\n",
						ifr.ifr_name, errno, strerror(errno));
			continue;
		}
		if (!(ifr.ifr_flags & IFF_UP)) {
			continue;
		}

		if (ioctl(sock, SIOCGIFADDR, &ifr) == -1) {
			if (CONET_DEBUG >= 2)
				fprintf(
						stderr,
						"ioctl(SIOCGIFADDR) for interface \"%s\" failed (%d): %s\n",
						ifr.ifr_name, errno, strerror(errno));
			continue;
		}
		if (((struct sockaddr_in*) &ifr.ifr_addr)->sin_family != AF_INET) {
			continue;
		}
		addr = ((struct sockaddr_in*) &ifr.ifr_addr)->sin_addr.s_addr;

		if (ioctl(sock, SIOCGIFNETMASK, &ifr) == -1) {
			if (CONET_DEBUG >= 2)
				fprintf(
						stderr,
						"ioctl(SIOCGIFNETMASK) for interface \"%s\" failed (%d): %s\n",
						ifr.ifr_name, errno, strerror(errno));
			continue;
		}
		netmask = ((struct sockaddr_in*) &ifr.ifr_netmask)->sin_addr.s_addr;
		if (CONET_DEBUG >= 2)
			fprintf(stderr, "ifindex=%d name=%s netmask=%s\n", i, ifr.ifr_name,
					inet_ntoa(
							((struct sockaddr_in*) &ifr.ifr_netmask)->sin_addr));

		if ((ipaddr.s_addr & netmask) == (addr & netmask)) {
			ifname = (char*) malloc(strlen(ifr.ifr_name) + 1);
			strcpy(ifname, ifr.ifr_name);
			*ifindex = i;
			return ifname;
		}
	}

	return NULL;
}

sa_family_t iface_encap(int sock, const char *ifname) {
	struct ifreq ifr;

	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, ifname);

	if (ioctl(sock, SIOCGIFHWADDR, &ifr) == -1) {
		if (CONET_DEBUG >= 2)
			fprintf(
					stderr,
					"ioctl(SIOCGIFHWADDR) for interface \"%s\" failed (%d): %s\n",
					ifname, errno, strerror(errno));
		return -1;
	}

	return ifr.ifr_hwaddr.sa_family;
}

/**
 * it returns 0 if ok, <0 if error
 * it sets the in/out if_index parameter to the index of the interface over which the packet should be sent
 */
int get_mac_from_ip(unsigned char * dst_mac, struct sockaddr_in *addr,
		int *if_index) {
	struct arpreq ar;
	int sock;
	struct sockaddr_in *saip;
	char *ifname;
	//    unsigned char       ether[8];

	memset(&ar, 0, sizeof(ar));

	saip = (struct sockaddr_in*) &ar.arp_pa;
	saip->sin_addr = addr->sin_addr;

	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock == -1) {
		perror("socket");
		return -1;
	}

	ifname = get_ifname_for_ip(sock, saip->sin_addr, if_index);

	if (ifname == NULL) {
		if (CONET_DEBUG >= 1)
			fprintf(stderr, "%s is not on a local network.\n", inet_ntoa(
					saip->sin_addr));
		return -1;
	} else {
		if (CONET_DEBUG >= 1)
			fprintf(stderr, "%s is on %s\n", inet_ntoa(saip->sin_addr), ifname);
	}

	if (iface_encap(sock, ifname) != ARPHRD_ETHER) {
		if (CONET_DEBUG >= 1)
			fprintf(stderr, "Interface %s is does not use ethernet.\n", ifname);
		return -1;
	}

	saip->sin_family = AF_INET;
	strcpy(ar.arp_dev, ifname);

	if (ioctl(sock, SIOCGARP, &ar) == -1) {
		if (errno == ENXIO) {
			if (CONET_DEBUG >= 1)
				fprintf(stderr, "No ARP entry.  Sending packet...\n");

			saip->sin_port = htons(7); /* port doesn't matter.  7 = echo */
			if (sendto(sock, "Hi", 2, 0, &ar.arp_pa, sizeof(struct sockaddr_in))
					== -1) {
				perror("sendto");
				return -1;
			}

			/* give it a second to respond to an ARP request */
			sleep(1);

			if (ioctl(sock, SIOCGARP, &ar) == -1) {
				if (errno == ENXIO) {
					if (CONET_DEBUG >= 1)
						fprintf(stderr, "Still no ARP entry.\n"
							"The machine may be unreachable or just slow to respond.\n");
				} else {
					perror("ioctl(SIOCGARP)");
				}
				return -1;
			}
		} else {
			perror("ioctl(SIOCGARP)");
			return -1;
		}
	}

	if (!(ar.arp_flags & ATF_COM)) {
		if (CONET_DEBUG >= 1)
			fprintf(stderr, "ARP lookup incomplete.\n"
				"The address may be unreachable or just slow to respond.\n");
		return -1;
	}

	if (ar.arp_ha.sa_family != ARPHRD_ETHER) {
		if (CONET_DEBUG >= 1)
			fprintf(stderr, "Unrecognized hardware address type: %hu\n",
					ar.arp_ha.sa_family);
		return -1;
	}
	close(sock);
	/* printf("%s\n", ether_ntoa((struct ether_addr*)&ar.arp_ha.sa_data)); */
	//    memcpy(ether, &ar.arp_ha.sa_data, 6);

	//    unsigned char * mybuff =  malloc (6);
	memcpy(dst_mac, &ar.arp_ha.sa_data, 6);
	free(ifname);
	return 0;
}

unsigned short int csum(unsigned short int *buff, int words) {
	unsigned int sum, i;
	sum = 0;
	for (i = 0; i < words; i++) {
		sum = sum + *(buff + i);
	}
	sum = (sum >> 16) + sum;
	return ~sum;
}

/**
 * it fills the bytes of the ethernet and ip header (prepending them to the packet)
 * both for interest cp and for data cp
 * it sets the in/out parameter packet_size
 */
#ifdef RAW_IPV6
//FARNE UNA UNICA NHO
unsigned char* setup_ipeth_headers(struct sockaddr_storage* this_hop,
		struct sockaddr_storage* next_hop, unsigned char * packet,
		size_t * packet_size, size_t ipoption_size) {

	if (CONET_DEBUG >= 1)
	fprintf(
			stderr,
			"[conet.c: %d] !!!setup_eth_headers, MIN_PACK_LEN=%d, CONET_VLAN_ID=%d\n",
			__LINE__, MIN_PACK_LEN, CONET_VLAN_ID);//&ct
	unsigned int ethhdr_size = 14;
	unsigned short int vid = htons(CONET_VLAN_ID), vlanproto;
	//	unsigned int vlan_size = (TAG_VLAN)?4:0;
	//	unsigned int headers_size = ethhdr_size + vlan_size + sizeof (struct iphdr);
	unsigned int headers_size = ethhdr_size + sizeof(struct iphdr);

	unsigned int headers_size6 = ethhdr_size + sizeof(struct ip6_hdr);

	if (CONET_DEBUG >= 1)
	fprintf(stderr, "[conet.c: %d] *packet_size =%d\n", __LINE__,
			*packet_size);//&ct

	unsigned int payload_size = *packet_size; //we call it payload size, but it includes the CONET ip option


	//	(*packet_size) = ethhdr_size/*ETH 14B*/ + vlan_size  + sizeof (struct iphdr)/*ip (20B)*/ + *packet_size/*including the IP option*/;
	if (next_hop->ss_family == AF_INET)
	(*packet_size) = ethhdr_size/*ETH 14B*/+ sizeof(struct iphdr)
	/*ip (20B)*/+ *packet_size/*including the IP option*/;
	if (next_hop->ss_family == AF_INET6)
	(*packet_size) = ethhdr_size/*ETH 14B*/+ sizeof(struct ip6_hdr)
	/*ip (20B)*/+ *packet_size/*including the IP option*/;

	if (CONET_DEBUG >= 1)
	fprintf(stderr, "[conet.c: %d] *packet_size =%d\n", __LINE__,
			*packet_size);//&ct

	if (*packet_size < MIN_PACK_LEN) {
		*packet_size = MIN_PACK_LEN;
	}
	unsigned char * buffer = calloc(1, *packet_size);
	if (buffer == NULL) {
		if (CONET_DEBUG >= 2)
		fprintf(stderr, "[conet.c: %d] calloc returned null \n", __LINE__);
		abort();
	}

	//struct sockaddr_ll sock_addr;
	struct ethhdr *eh = (struct ethhdr *) buffer;
	//	struct ip *iphdr = (struct ip *)(buffer + ethhdr_size +vlan_size);
	//	struct ip *iphdr = (struct ip *) (buffer + ethhdr_size);
	struct ip6_hdr *iphdr6 = (struct ip6_hdr *) (buffer + ethhdr_size);
	int ifindex = 0;
	unsigned char dst_mac[6] = DEST_MAC_ADDR;

	unsigned char *src_mac = local_mac;

	memcpy((void *) buffer, (void *) dst_mac, 6);

	memcpy((void *) (buffer + 6), (void *) src_mac, 6);

	//	if (CONET_DEBUG >= 2) fprintf(stderr, "!!!conet.c[%d]: TAG_VLAN : %d \n",__LINE__,TAG_VLAN);

	//	if (TAG_VLAN){ //this should not be used anymore...
	eh->h_proto = htons(0x8100);
	vlanproto = htons(0x86dd);
	memcpy((void *) (buffer + 14), (void *) &vid, 2);
	memcpy((void *) (buffer + 16), (void *) &vlanproto, 2);
	//	}
	//	else {
	eh->h_proto = htons(0x86dd);
	//	}

	//start of ip header
	//struct iphdr *ip = (struct iphdr *) buffer+ethhdr_size;

	//ipv6 header
	if (next_hop->ss_family == AF_INET6) {
		iphdr6->ip6_ctlun.ip6_un1.ip6_un1_flow = htonl((6 << 28) | (0 << 20)
				| 0); //flow control
		iphdr6->ip6_ctlun.ip6_un1.ip6_un1_plen = htons(payload_size); //payload length
		iphdr6->ip6_ctlun.ip6_un1.ip6_un1_nxt = CONET_PROTOCOL_NUMBER; //protocol number
		iphdr6->ip6_ctlun.ip6_un1.ip6_un1_hlim = 255; //time to live
		if (this_hop == NULL) {
			inet_pton(AF_INET6, SRC_IP6_ADDR, &(iphdr6->ip6_src.s6_addr));
		} else {
			iphdr6->ip6_src = ((struct sockaddr_in6*) this_hop)->sin6_addr;
		}
		//inet_pton (AF_UNSPEC, dst_ip, &(iphdr6->ip6_dst));  // &iv: a che serve??
		iphdr6->ip6_dst = ((struct sockaddr_in6*) next_hop)->sin6_addr;
	}

	//	if (next_hop->ss_family == AF_INET) {
	//		//ipv4 header
	//		iphdr->ip_hl = 5 + ipoption_size / 4;
	//		iphdr->ip_v = 4;
	//		iphdr->ip_tos = 16;
	//		iphdr->ip_len = htons(sizeof(struct ip) + payload_size);
	//		iphdr->ip_id = htons(52407);
	//		iphdr->ip_off = 0;
	//		iphdr->ip_ttl = 65;
	//		iphdr->ip_p = CONET_PROTOCOL_NUMBER;
	//		iphdr->ip_sum = 0;
	//	}
	if (this_hop == NULL) {
		//		if (next_hop->ss_family == AF_INET)
		//			inet_pton(AF_INET, local_ip, &(iphdr->ip_src.s_addr));
		if (next_hop->ss_family == AF_INET6)
		inet_pton(AF_INET6, SRC_IP6_ADDR, &(iphdr6->ip6_src.s6_addr));
	} else {
		//		if (this_hop->ss_family == AF_INET)
		//			iphdr->ip_src.s_addr
		//					= ((struct sockaddr_in*) this_hop)->sin_addr.s_addr;
		if (this_hop->ss_family == AF_INET6)
		iphdr6->ip6_src = ((struct sockaddr_in6*) this_hop)->sin6_addr;
	}

	//	if (next_hop->ss_family == AF_INET){
	//		iphdr->ip_dst.s_addr = ((struct sockaddr_in*) next_hop)->sin_addr.s_addr;
	//		memcpy(buffer + headers_size, packet, payload_size);
	//	}
	if (next_hop->ss_family == AF_INET6) {
		iphdr6->ip6_dst = ((struct sockaddr_in6*) next_hop)->sin6_addr;
		memcpy(buffer + headers_size6, packet, payload_size);
	}
	//	if (CONET_DEBUG >= 2) fprintf(stderr, "RAW");

	//	iphdr->ip_sum = csum((unsigned short int *)(buffer+ethhdr_size + vlan_size), iphdr->ip_hl<<1 );
	//	iphdr->ip_sum = csum((unsigned short int *) (buffer + ethhdr_size),
	//			iphdr->ip_hl << 1);
	free(packet);
	return buffer;

}

#else
unsigned char* setup_ipeth_headers(struct sockaddr_in* this_hop,
		struct sockaddr_in* next_hop, unsigned char * packet,
		size_t * packet_size, size_t ipoption_size) {

	if (CONET_DEBUG >= 2)
		fprintf(
				stderr,
				"[conet.c: %d] !!!setup_eth_headers, MIN_PACK_LEN=%d, CONET_VLAN_ID=%d\n",
				__LINE__, MIN_PACK_LEN, CONET_VLAN_ID);//&ct
	unsigned int ethhdr_size = 14;
	unsigned short int vid = htons(CONET_VLAN_ID), vlanproto;
	//	unsigned int vlan_size = (TAG_VLAN)?4:0;
	//	unsigned int headers_size = ethhdr_size + vlan_size + sizeof (struct iphdr);
	unsigned int headers_size = ethhdr_size + sizeof(struct iphdr);

	if (CONET_DEBUG >= 2)
		fprintf(stderr, "[conet.c: %d] *packet_size =%d\n", __LINE__,
				*packet_size);//&ct

	unsigned int payload_size = *packet_size; //we call it payload size, but it includes the CONET ip option


	//	(*packet_size) = ethhdr_size/*ETH 14B*/ + vlan_size  + sizeof (struct iphdr)/*ip (20B)*/ + *packet_size/*including the IP option*/;
	(*packet_size) = ethhdr_size/*ETH 14B*/+ sizeof(struct iphdr)
	/*ip (20B)*/+ *packet_size/*including the IP option*/;

	if (CONET_DEBUG >= 2)
		fprintf(stderr, "[conet.c: %d] *packet_size =%d\n", __LINE__,
				*packet_size);//&ct

	if (*packet_size < MIN_PACK_LEN) {
		*packet_size = MIN_PACK_LEN;
	}
	unsigned char * buffer = calloc(1, *packet_size);
	if (buffer == NULL) {
		if (CONET_DEBUG >= 2)
			fprintf(stderr, "[conet.c: %d] calloc returned null \n", __LINE__);
		abort();
	}

	//struct sockaddr_ll sock_addr;
	struct ethhdr *eh = (struct ethhdr *) buffer;
	//	struct ip *iphdr = (struct ip *)(buffer + ethhdr_size +vlan_size);
	struct ip *iphdr = (struct ip *) (buffer + ethhdr_size);
	int ifindex = 0;
	unsigned char dst_mac[6];
	memset(dst_mac, 0, 6);
	int ret_mac = get_mac_from_ip(dst_mac, next_hop, &ifindex)/*{ 0x00, 0x1b, 0x38, 0x45, 0xae, 0x3e }*/;
	if (ret_mac != 0) {
		if (CONET_DEBUG >= 1)
			fprintf(stderr,
					"conet.c[%d] error in getting MAC, dst MAC set to 00\n",
					__LINE__);
	}

	unsigned char *src_mac = local_mac;

	memcpy((void *) buffer, (void *) dst_mac, 6);

	memcpy((void *) (buffer + 6), (void *) src_mac, 6);

	//	if (CONET_DEBUG >= 2) fprintf(stderr, "!!!conet.c[%d]: TAG_VLAN : %d \n",__LINE__,TAG_VLAN);

	//	if (TAG_VLAN){ //this should not be used anymore...
	//SS the following bytes are rewritten by iphdr
	eh->h_proto = htons(0x8100);
	vlanproto = htons(0x0800);
	memcpy((void *) (buffer + 14), (void *) &vid, 2);
	memcpy((void *) (buffer + 16), (void *) &vlanproto, 2);
	//	}
	//	else {
	eh->h_proto = htons(0x0800);
	//	}

	//start of ip header
	//struct iphdr *ip = (struct iphdr *) buffer+ethhdr_size;

	iphdr->ip_hl = 5 + ipoption_size / 4;
	iphdr->ip_v = 4;
	iphdr->ip_tos = 16;
	iphdr->ip_len = htons(sizeof(struct ip) + payload_size);
	iphdr->ip_id = htons(52407);
	iphdr->ip_off = 0;
	iphdr->ip_ttl = 65;
	iphdr->ip_p = CONET_PROTOCOL_NUMBER;
	iphdr->ip_sum = 0;
	if (this_hop == NULL) {
		inet_pton(AF_INET, local_ip, &(iphdr->ip_src.s_addr));
	} else {
		iphdr->ip_src.s_addr = this_hop->sin_addr.s_addr;
	}

	iphdr->ip_dst.s_addr = next_hop->sin_addr.s_addr;

	//	if (CONET_DEBUG >= 2) fprintf(stderr, "RAW");
	memcpy(buffer + headers_size, packet, payload_size);
	//	iphdr->ip_sum = csum((unsigned short int *)(buffer+ethhdr_size + vlan_size), iphdr->ip_hl<<1 );
	iphdr->ip_sum = csum((unsigned short int *) (buffer + ethhdr_size),
			iphdr->ip_hl << 1);
	free(packet);
	return buffer;

}
#endif
/**
 * Send interest carrier-packet for completing a chunk,
 * if a chunk is complete but not the congestion window it try to setup a send for another chunk
 * @return -1 if an error occurred during setting socket or sending bytes else the number of byte sent
 */
int conet_send_interest_cp(struct ccnd_handle* h, struct conet_entry* ce,
		struct chunk_entry* ch, short rescheduled_send) {//XXX NB: the window size must not be greater than an int because of the return value

	int i_bitmap_index = 0;
	int res = 0;
	int sent = 0;
	int segment_sent = 0;

	if (CONET_DEBUG == -3) { //whr debug
		fprintf(stderr, "[conet.c inizio send interest:");
		conet_print_stats(ce, ch, __LINE__);
	}

	unsigned int not_sent_on_starting = ch->m_segment_size
			- ch->starting_segment_size;

	if (ce->chunk_expected == NULL) {
		ce->chunk_expected = ch;
	}

	if (ce->cwnd - ce->in_flight <= 0 && rescheduled_send == 0) {
		//i have sent all the possible interest for the window, i dont send anymore.
		return 0;
	}

	int start_index = ch->last_interest;

	if (rescheduled_send == 1) {
		//in case of a send after a timeout i start from last in sequence segment
		start_index = ch->last_in_sequence;
		ch->is_oos = 1;
		if (CONET_DEBUG >= 2)
			fprintf(stderr, "start index %d \n", start_index);
	}
	if (CONET_DEBUG >= 2)
		fprintf(stderr, "[conet.c: %d] li:%d lis:%d lpi:%d start_index:%d\n",
				__LINE__, ch->last_interest, ch->last_in_sequence,
				ch->last_possible_interest, start_index);

	for (i_bitmap_index = start_index; i_bitmap_index
			< ch->last_possible_interest && i_bitmap_index < ch->bitmap_size; i_bitmap_index++) {

		if (ch->expected[i_bitmap_index].bit == 1) {
			if (rescheduled_send == 0
					&& ch->sending_interest_table[i_bitmap_index].bit == 1) {
				//this interest has been already sent and so it is expected, but this is not a retransmission, so go ahead
				continue;
			}
			if (ce->cwnd - ce->in_flight <= 0 && rescheduled_send == 0) {
				//i have sent all the possible interest for the window, i dont send anymore.
				return 0;
			}
			//else: we are in resending or we send a new interest
			if (rescheduled_send && CONET_DEBUG >= 2)
				fprintf(
						stderr,
						"[conet.c: %d] bitmap index: %d seg_size %d chunk_size %llu\n",
						__LINE__, i_bitmap_index, ch->m_segment_size,
						ce->chunk_size);
			if (((ch->m_segment_size * i_bitmap_index) - not_sent_on_starting)
					< ce->chunk_size || ce->chunk_size == 0) {//TODO verify the operator
				unsigned int left_edge = ch->m_segment_size * i_bitmap_index
						- not_sent_on_starting;

				unsigned int right_edge = ch->m_segment_size * (i_bitmap_index
						+ 1) - 1 - not_sent_on_starting;
				if (CONET_DEBUG >= 2)
					fprintf(
							stderr,
							"[conet.c:%d]left_edge=%u; right_edge=%u= %u[ch->m_segment_size] * ( %d[i_bitmap_index] +1) - 1 - %u[not_sent_on_starting]\n",
							__LINE__, left_edge, right_edge,
							ch->m_segment_size, i_bitmap_index,
							not_sent_on_starting);
				if (CONET_SEVERE_DEBUG && right_edge <= left_edge)
					exit(-873);

#ifdef	IS_CACHE_SERVER //&ct:inizio
				cp_descriptor->l_edge = left_edge; //&ct
				cp_descriptor->segment_size
				= (unsigned long long) ch->m_segment_size;//&ct
#endif				//&ct:fine
				unsigned char* packet;

				/**
				 * Deliver through direct socket udp,
				 * TODO in future we use only CONET_USE_OUTBOUND
				 *
				 */
				int sock = 0;
				int use_raw = 0;

				if (left_edge == 0 && rescheduled_send == 0) {
					//for first segment of a chunk requested, we set the send_started for timeout calculation
					ch->send_started = get_time();
				}

				struct sockaddr* next_hop;
				socklen_t addr_size;
				/* Construct the server sockaddr_in structure */
				//				struct sockaddr_in next_hop_in;
				struct sockaddr_storage next_hop_storage;
				struct sockaddr* to_addr;

				if (ch->outbound != NULL && ch->outbound->n > 0) {
					unsigned faceid = 0;

					faceid = ch->send_faceid;
					if (faceid < 0) {
						faceid = h->ipv4_faceid;
					}

					struct face *face = face_from_faceid(h, faceid);
					if (face->addr == NULL) {
						face = face_from_faceid(h, h->ipv4_faceid);
					}

					if (face != NULL && (face->flags & CCN_FACE_NOSEND) == 0) {
						next_hop = face->addr;
						int next_hop_port =
								((struct sockaddr_in*) next_hop)->sin_port;
						if (next_hop_port == 58149) { //XXX corresponds to 9699
							if (CONET_DEBUG >= 2)
								fprintf(stderr,
										"[conet.c: %d]next_hop_port=%d\n",
										__LINE__, next_hop_port);
							use_raw = 1;
							memset(&next_hop_storage, 0,
									sizeof(next_hop_storage));
							next_hop_storage.ss_family = next_hop->sa_family;

							if (next_hop_storage.ss_family == AF_INET6) {
								((struct sockaddr_in6 *) &next_hop_storage)->sin6_addr
										= ((struct sockaddr_in6*) next_hop)->sin6_addr;
								//									char src_addr6[INET6_ADDRSTRLEN];
								//									inet_ntop(AF_INET6, ((struct sockaddr_in6*) &next_hop_storage)->sin6_addr.s6_addr, src_addr6, INET6_ADDRSTRLEN);
								//									fprintf(stderr,"[conet.c: %d] sin6_addr.s6_addr%s\n",__LINE__, src_addr6);
							} else if (next_hop_storage.ss_family == AF_INET) {
								((struct sockaddr_in *) &next_hop_storage)->sin_addr.s_addr
										= ((struct sockaddr_in*) next_hop)->sin_addr.s_addr; /* IP address */
							} else {
								fprintf(stderr, "[conet.c: %d] NORD KOREA \n",
										__LINE__);
							}
							addr_size = 20;//solo ipv4 adattare anche ad ipv6// sizeof(next_hop_storage);
						}
						addr_size = face->addrlen;
						if (!use_raw) {
							sock = sending_fd(h, face);
						} else {
							sock = ce->send_fd; //SS sceglie il socket raw da scrivere
						}
						if (sock <= 0) {
							sock = ce->send_fd;
						}
						if (sock <= 0) {
							if (!use_raw) {
								if (CONET_DEBUG >= 2)
									fprintf(
											stderr,
											"[conet.c:%d] No fd for faceid %d, create socket\n",
											__LINE__, face->faceid);
								if ((sock = socket(AF_INET, SOCK_DGRAM,
										IPPROTO_UDP)) < 0) {
									perror("[CONET]: Failed to create socket");
									return -1;
								}
							} else { //use_raw

								if (CONET_DEBUG >= 2)
									fprintf(
											stderr,
											"[conet.c:%d] No fd for faceid %d, create a RAW socket\n",
											__LINE__, face->faceid);
								raw_sock = socket(PF_PACKET, SOCK_RAW, htons(
										ETH_P_ALL));
								if (CONET_DEBUG >= 2)
									fprintf(
											stderr,
											"[conet.c:%d] RAW socket: %d created\n",
											__LINE__, raw_sock);
								//perror("[CONET]: Failed to create RAW socket");
								if (raw_sock < 0) {
									perror(
											"[conet]: Failed to create RAW socket");
									return -1;
								}
								sock = raw_sock;   //SS cambio da socket normale a raw appena creato
							}
							face->recv_fd = sock;
							ce->send_fd = sock;  //SSlo metto anche in ce->send_fd

						}
					}
				} else { //ch->outbound==NULL || ch->outbound->n <=0
					if (CONET_DEBUG >= 2)
						fprintf(
								stderr,
								"[conet.c:%d] Error in get the correct face for this send, return -1\n",
								__LINE__);
					return -1;
				}
				size_t packet_size = 0;

				if (use_raw) {
					//define non necessaria
#ifdef RAW_IPV6
					packet_size = (size_t) build_ip_option(&packet,
							CONET_INTEREST_CIU_TYPE_FLAG, CONET_CIU_CACHE_FLAG,
							ce->nid, ch->chunk_number, next_hop->sa_family);//whr

#else

#ifndef CONET_TRANSPORT
					packet_size = (size_t) build_ip_option(&packet,
							CONET_INTEREST_CIU_TYPE_FLAG, CONET_CIU_CACHE_FLAG,
							ce->nid, ch->chunk_number, NULL);
#endif
#endif
					unsigned int ipoption_size = (unsigned int) packet_size;

					if (CONET_DEBUG >= 2)
						fprintf(stderr, "[conet.c: %d] setting openflow_tag\n",
								__LINE__);//&ct
#ifdef IPOPT
					packet_size = (size_t) set_openflow_tag(&packet,
							(unsigned int) packet_size,
							CONET_INTEREST_CIU_TYPE_FLAG, ce->nid,
							ch->chunk_number);
#endif
					packet_size = build_carrier_packet(&packet, packet_size,
							CONET_INTEREST_CIU_TYPE_FLAG, CONET_CIU_CACHE_FLAG,
							ce->nid, ch->chunk_number, left_edge, right_edge,
							0, ce->chunk_size);

					packet = setup_ipeth_headers(NULL, &next_hop_storage,
							packet, &packet_size, ipoption_size);
					to_addr = setup_raw_sockaddr(&next_hop_storage);
					addr_size = 20; //solo ipv4 adattare anche ad ipv6 // sizeof(next_hop_storage);

					//conet_print_ccnb(packet, packet_size);
				} else { //not use raw
					packet_size = build_carrier_packet(&packet, 0,
							CONET_INTEREST_CIU_TYPE_FLAG, CONET_CIU_CACHE_FLAG,
							ce->nid, ch->chunk_number, left_edge, right_edge,
							0, ce->chunk_size);
				}

				if (CONET_DEBUG >= 2 && use_raw)
					fprintf(
							stderr,
							"sending interest to socket raw: %d packet_size: %d\n",
							sock, packet_size);
				if (CONET_DEBUG >= 2 && use_raw != 1)
					fprintf(stderr, "sending interest to socket plain: %d\n",
							sock);
				if (CONET_DEBUG == -3) { //whr debug
					fprintf(
							stderr,
							"[conet.c:%d]: Sending an interest for %s/%llu /%d \n",
							__LINE__, ce->nid, ch->chunk_number, i_bitmap_index);
					//					if (!ce->in_fast_rec) {
					//						recover_chunk = ch->chunk_number;//whr qui per nwe reno
					//						recover_cp = i_bitmap_index;//whr
					//					}
				}

				if (CONET_DEBUG == 2 && use_raw) {
					fprintf(stderr, "protocol: %d ",
							((struct sockaddr_ll*) to_addr)->sll_protocol);
					fprintf(stderr, "if_index: %d ",
							((struct sockaddr_ll*) to_addr)->sll_ifindex);
					fprintf(stderr, "hatype: %d ",
							((struct sockaddr_ll*) to_addr)->sll_hatype);
					fprintf(stderr, "pkttype: %d ",
							((struct sockaddr_ll*) to_addr)->sll_pkttype);
					fprintf(stderr, "halen %d ",
							((struct sockaddr_ll*) to_addr)->sll_halen);
					fprintf(stderr, "dest MAC: ");
					int i;
					for (i = 0; i < 6; i++) {
						if (CONET_DEBUG >= 2)
							fprintf(
									stderr,
									"%02X ",
									((struct sockaddr_ll*) to_addr)->sll_addr[i]);
					}
					fprintf(stderr, "\n");
					fprintf(stderr, "Sending bytes:\n");
					for (i = 0; i < packet_size; i++) {
						if (i % 16 == 0) {
							fprintf(stderr, "%04X  ", i);
						}
						fprintf(stderr, "%02X ", packet[i]);
						if (i % 16 == 15)
							fprintf(stderr, "\n");
					}
					fprintf(stderr, "\n");

				}
				if (rescheduled_send && CONET_DEBUG >= 2)
					fprintf(
							stderr,
							"[conet.c:%d]: RESCHEDULED SEND for %s/%llu left edge:%u, right edge:%u\n",
							__LINE__, ce->nid, ch->chunk_number, left_edge,
							right_edge);

				//we're in conet_send_interest_cp
				if (use_raw)
					sent = sendto(sock, packet, packet_size, 0, to_addr, 
							addr_size);
							//SS qui scrive nel socket raw
				else
					sent = sendto(sock, packet, packet_size, 0, next_hop,
							addr_size);

				if (CONET_DEBUG >= 2 && use_raw)
					fprintf(stderr, "[conet.c:%d] Sent %d byte to %d\n",
							__LINE__, sent, sock);

				if (CONET_LOG_TO_FILE) {
					if (out_file_si == 0) {
						out_file_si = (unsigned int) fopen("out_si.csv", "wt");
					}
					log_to_file((FILE*)out_file_si, ch->chunk_number, left_edge,
							right_edge);
				}

				if (sent != packet_size) {
					perror(
							"[CONET]:Send interest, Mismatch in number of sent bytes"); //bug con raw:in alcuni casi, manca il dest ethernet (da sistemare!!!)
					fprintf(stderr, "\n");
					if (sent == -1) {
						if (CONET_DEBUG >= 2)
							fprintf(stderr,
									"[conet.c:%d] Sendto error: %s, sock:%d\n",
									__LINE__, strerror(errno), sock);
					}
					udp_sock = 0;
					raw_sock = 0;
					close(sock);
					res = -1;
					free(packet);
					if (use_raw)
						free(to_addr);
					break;
				}

				segment_sent++;
				if (rescheduled_send == 0) {
					ce->in_flight++;
				}
				if (CONET_DEBUG >= 2)
					fprintf(
							stderr,
							"[conet.c: %d] Set ch->sending_interest_table[%d].bit=1\n",
							__LINE__, i_bitmap_index);

				ch->sending_interest_table[i_bitmap_index].bit = 1;
				free(packet);
				if (use_raw)
					free(to_addr);
			} else {
				//no other interest to send
				break;
			}
		}
	}
	if (CONET_DEBUG >= 1)
		fprintf(stderr, "[conet.c: %d] segment sent: %d\n", __LINE__,
				segment_sent);
	if (segment_sent > 0 && rescheduled_send == 0) {
		if (ch->to == NULL) {
			timeout_list_append(ce, ch);
		} else {
			reschedule(ch->to);
		}
	}

	if (CONET_DEBUG == -3) { //whr debug
		fprintf(stderr, "[conet.c fine send interest:");
		conet_print_stats(ce, ch, __LINE__);

	}
	return res;
}

/**
 * This function retransmit interest for some outofsequence packet
 */
//ipv6b fare la retransmit
int retransmit(struct ccnd_handle* h, struct conet_entry* ce,
		struct chunk_entry * ch) {
	//	fprintf(stderr,	"[conet.c:%d] !!!!!!!!!!!!!!!!!!!retransmit!!!!!!!!!!!!!!!!!!!\n",__LINE__);
//	int res = 0;
	int sent = 0;
	unsigned int not_sent_on_starting = ch->m_segment_size
			- ch->starting_segment_size; //TODO  ch->starting_segment_size = ch->m_segment_size  sul costruttore
	int sock = 0;
	const struct sockaddr* next_hop;
	/* Construct the server sockaddr_in structure */
	struct sockaddr_in next_hop_in;
	struct sockaddr* to_addr;
	size_t packet_size;
	socklen_t addr_size;
	int i = ch->last_in_sequence;

	if (ch->out_of_sequence_received[i].bit == 0) {
		if (((ch->m_segment_size * i) - not_sent_on_starting) < ce->chunk_size) {
			ch->is_oos = 1;
			unsigned int left_edge = ch->m_segment_size * i
					- not_sent_on_starting;
			unsigned int right_edge = ch->m_segment_size * (i + 1) - 1
					- not_sent_on_starting;
			unsigned char* packet;//= malloc(sizeof(unsigned char));
			int use_raw = 0;
			/**
			 * Spedizione tramite socket udp, TODO successivamente andrÃ  specificato il next_hop con lookup and cache e
			 * il socket forse andra' mantenuto aperto?
			 *
			 */
			if (ch->outbound != NULL && ch->outbound->n > 0) {
				unsigned faceid = 0;

				faceid = ch->send_faceid;
				if (faceid < 0) {
					faceid = h->ipv4_faceid;
				}

				struct face *face = face_from_faceid(h, faceid);
				if (face->addr == NULL) {
					face = face_from_faceid(h, h->ipv4_faceid);
				}

				if (face != NULL && (face->flags & CCN_FACE_NOSEND) == 0) {
					next_hop = face->addr;
					int next_hop_port =
							((struct sockaddr_in*) next_hop)->sin_port;
					if (CONET_DEBUG >= 2)
						fprintf(stderr, "\n nexthop port %d", next_hop_port);
					if (next_hop_port == 58149) { //XXX corresponds to 9699
						if (CONET_DEBUG >= 2)
							fprintf(stderr, "[conet.c: %d]next_hop_port=%d\n",
									__LINE__, next_hop_port);
						use_raw = 1;
						memset(&next_hop_in, 0, sizeof(next_hop_in)); /* Clear struct */
						next_hop_in.sin_family = AF_INET; /* Internet/IP */
						next_hop_in.sin_addr.s_addr
								= ((struct sockaddr_in*) next_hop)->sin_addr.s_addr; /* IP address */
						next_hop_in.sin_port
								= ((struct sockaddr_in*) next_hop)->sin_port; /* server port */
						addr_size = sizeof(next_hop_in);
						//to_addr=(struct sockaddr*) &next_hop_in;

					}
					addr_size = face->addrlen;
					if (!use_raw) {
						sock = sending_fd(h, face);
					} else {
						sock = ce->send_fd;
					}
					if (sock <= 0) {
						sock = ce->send_fd;
					}
					if (sock <= 0) {
						if (!use_raw) {
							if (CONET_DEBUG >= 2)
								fprintf(
										stderr,
										"[conet.c:%d] No fd for faceid %d, create socket\n",
										__LINE__, face->faceid);
							if ((sock
									= socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))
									< 0) {
								perror("[CONET]: Failed to create socket");
								return -1;
							}
						} else { //use_raw

							if (CONET_DEBUG >= 2)
								fprintf(
										stderr,
										"[conet.c:%d] No fd for faceid %d, create a RAW socket\n",
										__LINE__, face->faceid);
							raw_sock = socket(PF_PACKET, SOCK_RAW, htons(
									ETH_P_ALL));
							if (CONET_DEBUG >= 2)
								fprintf(
										stderr,
										"[conet.c:%d] RAW socket: %d created\n",
										__LINE__, raw_sock);
							//perror("[CONET]: Failed to create RAW socket");
							if (raw_sock < 0) {
								perror("[conet]: Failed to create RAW socket");
								return -1;
							}
//							int z_stop = 0;
							sock = raw_sock;
						}
						face->recv_fd = sock;
						ce->send_fd = sock;

					}
					//							if (CONET_DEBUG >= 2) fprintf(stderr, "send via faceid:%d fd:%d \n", face->faceid, sock);
					//if (CONET_DEBUG >= 2) fprintf(stderr, "send via faceid:%d fd:%d \n", face->faceid, sock);
				}
				//if (CONET_DEBUG >= 2) fprintf(stderr, "send via faceid:%d fd:%d \n", face->faceid, sock);
			}

			if (use_raw) {
#ifndef CONET_TRANSPORT
				packet_size = (size_t) build_ip_option(&packet,
						CONET_INTEREST_CIU_TYPE_FLAG, CONET_CIU_CACHE_FLAG,
						ce->nid, ch->chunk_number, AF_INET);
#endif
				unsigned int ipoption_size = (unsigned int) packet_size;

				if (CONET_DEBUG >= 2)
					fprintf(stderr, "[conet.c: %d] setting openflow_tag\n",
							__LINE__);//&ct
#ifdef IPOPT
				packet_size
						= (size_t) set_openflow_tag(&packet,
								(unsigned int) packet_size,
								CONET_INTEREST_CIU_TYPE_FLAG, ce->nid,
								ch->chunk_number);
#endif
				packet_size = build_carrier_packet(&packet, packet_size,
						CONET_INTEREST_CIU_TYPE_FLAG, CONET_CIU_CACHE_FLAG,
						ce->nid, ch->chunk_number, left_edge, right_edge, 0,
						ce->chunk_size);

				packet = setup_ipeth_headers(NULL, &next_hop_in, packet,
						&packet_size, ipoption_size);
				to_addr = setup_raw_sockaddr(&next_hop_in);
				addr_size = 20;//solo ipv4 adattare anche ad ipv6  //sizeof(next_hop);
			} else { //not use raw
				packet_size = build_carrier_packet(&packet,0,
						CONET_INTEREST_CIU_TYPE_FLAG, CONET_CIU_CACHE_FLAG,
						ce->nid, ch->chunk_number, left_edge, right_edge, 0,
						ce->chunk_size);

			}
			//we're in retransmit
			if (use_raw)
				sent = sendto(sock, packet, packet_size, 0, to_addr, addr_size);
			else
				sent
						= sendto(sock, packet, packet_size, 0, next_hop,
								addr_size);

			if (CONET_DEBUG == -3) //whr debug
				fprintf(stderr,
						"[conet.c:%d]: Retransmission of %s/%llu /%d \n",
						__LINE__, ce->nid, ch->chunk_number,
						calculate_seg_index(left_edge, right_edge,
								ch->m_segment_size, ch->starting_segment_size,
								ch->bytes_not_sent_on_starting));
			if (CONET_LOG_TO_FILE) {
				if (retrasmit_file == 0) {
					retrasmit_file = (unsigned int) fopen("retransmit.csv", "wt");
				}
				log_to_file((FILE*) retrasmit_file, ch->chunk_number, left_edge,
						right_edge);
			}

			if (sent != packet_size) {
				perror("[CONET]: Retransmit, Mismatch in number of sent bytes");
				if (sent == -1) {
					if (CONET_DEBUG >= 2)
						fprintf(stderr, "[conet.c:%d] Sendto error: %s\n",
								__LINE__, strerror(errno));
				}
				udp_sock = 0;
				raw_sock = 0;
				close(sock);
//				res = -1;
				//return -1;
			}

			if (CONET_DEBUG >= 2)
				fprintf(
						stderr,
						"[conet.c: %d:retransmit] Set ch->sending_interest_table[%d].bit=1\n",
						__LINE__, i);

			ch->sending_interest_table[i].bit = 1; //TODO may be done here. verify.
			ch->expected[i].bit = 1;
			conet_retransmit_stat++;

			//else end of chunk reached

			/*//TODO verify and remove: else {
			 ch->last_possible_interest++;
			 }*/
			free(packet);

		}
	}
	//	return sent; //controllare il ritorno
	return 0; //perche???????
}



int conet_compare_names(const unsigned char *a, size_t asize,
		const unsigned char *b, size_t bsize) {
	struct ccn_buf_decoder a_decoder;
	struct ccn_buf_decoder b_decoder;
	struct ccn_buf_decoder *aa = ccn_buf_decoder_start_at_components(
			&a_decoder, a, asize);
	struct ccn_buf_decoder *bb = ccn_buf_decoder_start_at_components(
			&b_decoder, b, bsize);
	aa = &a_decoder;
	bb = &b_decoder;
	const unsigned char *acp = NULL;
	const unsigned char *bcp = NULL;
	size_t acsize;
	size_t bcsize;
	size_t ccsize;
	int cmp = 0;
	//	int cmp1 = 0;
	int more_a;
	//	int more_c;
	int matches = -1;
	for (;;) {
		more_a = ccn_buf_match_dtag(aa, CCN_DTAG_Component);
		cmp = more_a - ccn_buf_match_dtag(bb, CCN_DTAG_Component);
		if (more_a == 0 || cmp != 0) {
			break;
		}
		ccn_buf_advance(aa);
		ccn_buf_advance(bb);
		acsize = bcsize = ccsize = 0;
		if (ccn_buf_match_blob(aa, &acp, &acsize))
			ccn_buf_advance(aa);
		if (ccn_buf_match_blob(bb, &bcp, &bcsize))
			ccn_buf_advance(bb);
		cmp = acsize - bcsize;
		if (cmp != 0) {
			break;
		}
		cmp = memcmp(acp, bcp, acsize);
		if (cmp != 0) {
			break;
		}
		matches++;
		ccn_buf_check_close(aa);
		ccn_buf_check_close(bb);
	}
	return (cmp);
}


/**
 * Find a content given its nid and chunk number
 */

struct content_entry * find_ccn_content(struct ccnd_handle *h,
		struct ccn_charbuf* name) {

	int i;
	int n = h->skiplinks->n;
	struct ccn_indexbuf *c;
	struct content_entry *content;
	int order;
	size_t start;
	size_t end;
	struct ccn_indexbuf *ans[30] = { NULL };

	c = h->skiplinks;

	for (i = n - 1; i >= 0; i--) {
		for (;;) {
			if (c->buf[i] == 0)
				break;
			content = content_from_accession(h, c->buf[i]);
			if (content == NULL)
				abort();
			start = content->comps[0];
			end = content->comps[content->ncomps - 1];
			order = conet_compare_names(content->key + start - 1, end - start
					+ 2, name->buf, name->length);
			if (order > 0) {
				break;
			}
			if (order == 0)
				break;
			if (content->skiplinks == NULL || i >= content->skiplinks->n)
				abort();
			c = content->skiplinks;
		}
		ans[i] = c;
	}

	//TODO check if this '0' must be 'i', what should i choose? the first match or the last? How many choices can i have? Can i have multiple choices?
	return (ans[0] != NULL) ? content_from_accession(h, ans[0]->buf[0]) : NULL;
}

/**
 * Send the chunk just completed for saving in ccnx
 */
void conet_send_chunk_to_ccn(struct ccnd_handle* h, struct chunk_entry* ch,
		unsigned int chunk_size) {

	struct face* f = face_from_faceid(h, ch->return_faceid);
	if (CONET_DEBUG >= 2)
		ccnd_debug_ccnb(h, __LINE__, " CHUNK TO CCN ", NULL, ch->chunk,
				chunk_size);
	call_process_incoming_content(h, f, ch->chunk, chunk_size);
	if (CONET_DEBUG >= 2)
		fprintf(stderr,
				"[conet.c:%d]!!!!!!!!!!!!!!!!!!!!!!!! CHUNK COMPLETED\n",
				__LINE__);
}

/**
 * extimate the segment size based upon the maximum overhead
 * of carrier packet, and the mtu of the transimission link.
 * mtu can be passed or fixed (last param set to 0).
 */
unsigned short get_segment_size(char * nid, unsigned long long csn,
		int chunk_size, int mtu) {
	short overhead = 1 + 2 + 4 + 10; //2 option type and    aaalen and 4 openflow tag 10 is for me

	if (strlen(nid) == CONET_DEFAULT_NID_LENGTH) {
		overhead += 16;
	} else {
		overhead += strlen(nid) + 1;
	}

	if (csn < 128) {
		overhead += 1;
	} else if (csn < 16384) {
		overhead += 2;
	} else if (csn < 2097152) {
		overhead += 3;
	} else if (csn < 268435456U) {
		overhead += 4;
	} else if (csn < 2147483648U) {
		overhead += 5;
	} else if (csn < 549755813888U) {
		overhead += 6;
	}
	/**
	 * 	  LEFT AND RIGHT EDGE FIELD
	 *    we suppose the knowledge of chunk size, if not we add the maximum len.
	 *
	 */
	if (chunk_size < 128) {
		overhead += 1 * 2;
	} else if (chunk_size < 16384) {
		overhead += 2 * 2;
	} else if (chunk_size < 2097152) {
		overhead += 3 * 2;
	} else if (chunk_size < 268435456U) {
		overhead += 4 * 2;
	}

	overhead += 1;
	if (mtu == 0) {
		mtu = CONET_DEFAULT_MTU;
	}

	unsigned short m_segment_size = mtu - overhead;
	if (CONET_DEBUG >= 2)
		fprintf(stderr,
				"[conet.c:%d]m_segment_size=mtu(%d)-overhead(%hd)=%hu\n",
				__LINE__, mtu, overhead, m_segment_size);

	if (m_segment_size == 0) {
		fprintf(stderr, "[conet.c:%d] m_segmnent_size can not be 0\n", __LINE__);
	}

	return m_segment_size;

}
/**
 * Scheduled function to resend interest if there are no response for TIMEOUT seconds UNUSED
 */
int conet_rescheduled_send(struct ccn_schedule *sched, void *clienth,
		struct ccn_scheduled_event *ev, int flags) {

	if ((flags & CCN_SCHEDULE_CANCEL) != 0) { //this call came from ccn_schedule_cancel() so we need to return a value < 0 to cancel the event from the schedule

		return -1; //must return a value < 0 to let event be deleted from schedule
	}
	struct conet_sched_param *param = ev->evdata;
	if (param == NULL || ev->evint != 2048 || param < 1024) { // sistemato warning cast ad int (da controllare)
		return -1;
	}
	struct ccnd_handle *h = clienth;
	unsigned clock_micros_per_base = 1000000; //if it was possible to dereference sched we could use sched->clock->micros_per_base
	struct conet_entry* ce = param->ce;
	struct chunk_entry* ch = param->ch;
	if (ce == NULL || ch == NULL) {
		return -1;
	}
	if (ch->last_possible_interest == 0) {
		return -1;
	}
	struct timeval time = { 0 };
	gettimeofday(&time, NULL);

	long timeout_sec = time.tv_sec - ch->timeout->tv_sec;
	long timeout_microsec = time.tv_usec + (clock_micros_per_base
			- ch->timeout->tv_usec);
	long timeout = (timeout_sec * clock_micros_per_base + timeout_microsec);

	if (CONET_DEBUG >= 2)
		fprintf(
				stderr,
				"[conet.c:%d] TIMEOUT rescheduling operation start, chunk: %llu timeout at (%lu)\n",
				__LINE__, ch->chunk_number, timeout);

	if (timeout > 5 * clock_micros_per_base) {
		if (CONET_DEBUG >= 2)
			fprintf(stderr, "[conet.c:%d] timeout for %s/%llu  \n", __LINE__,
					ce->nid, ch->chunk_number);
		//TODO last time i try!
		ch->never_arrived = 1;
		//conet_send_interest_cp(h, ce, ch, 1);
		if (ccn_schedule_cancel(sched, ev) == -1) {
			if (CONET_DEBUG >= 2)
				fprintf(
						stderr,
						"[conet.c:%d]: Attempt to cancel a scheduled resend operation failed\n",
						__LINE__);
		} else {
			if (CONET_DEBUG >= 2)
				fprintf(
						stderr,
						"[conet.c:%d]: Attempt to cancel a scheduled resend operation success!\n",
						__LINE__);
		}
		//must return a value > 0 to let rescheduling with the new action( ccn_schedule_cancelled_event )
		return 1;
	}

	conet_send_interest_cp(h, ce, ch, 1);

	return 1 * clock_micros_per_base;
}
/**
 * Manage the congestion window, in slow start and also in collision avoidance
 */
void window_control(struct conet_entry * ce, struct chunk_entry *ch) {
	if (ce->in_fast_rec == 1 && ce->out_of_sequence_counter == 3) {
		ce->threshold = (ce->in_flight / 2 < 2) ? 2 : ce->in_flight / 2;
		ce->cwnd = ce->threshold + 3;
#ifndef TCP_BUG_FIX
		if (ce->in_flight > ce->cwnd) {
			//			fprintf(stderr,"[CONET] ce->in_flight: %d ce->cwnd: %d \n",ce->in_flight,ce->cwnd);
			ce->in_flight = ce->cwnd;
		}
#endif
	}
	if (ce->cwnd < ce->threshold) { //slow start
		ce->cwnd++;
	} else { //congestion avoidance
		ce->wnd_adder += (double) 1 / (double) ce->cwnd;
		if (ce->wnd_adder >= 1 && ce->cwnd < MAX_THRESHOLD) {
			ce->cwnd++;
			ce->wnd_adder = 0;
		}
	}

}

/* conet_rescheduled_send con passaggio di nid e csn nella struct conet_sched_param per la gestione sicura tramite hashtb piuttosto che con il puntatore diretto alla conet_entry ed alla chunk_entry (ce, ch)
 int conet_rescheduled_send(struct ccn_schedule *sched, void *clienth, struct ccn_scheduled_event *ev, int flags){

 struct conet_sched_param *param = ev->evdata;
 if((flags & CCN_SCHEDULE_CANCEL) != 0){ //this call came from ccn_schedule_cancel() so we need to return a value < 0 to cancel the event from the schedule
 if(param != NULL){
 if(param->nid != NULL)
 free(param->nid);
 free(param);
 }
 return -1; //must return a value < 0 to let event be deleted from schedule
 }
 unsigned clock_micros_per_base = 1000000; //if it was possible to dereference sched we could use sched->clock->micros_per_base
 char* nid = param->nid;

 struct timeval time = {0};
 gettimeofday(&time, NULL);

 struct hashtb_enumerator ee;
 struct hashtb_enumerator *e = &ee;
 hashtb_start( ((struct ccnd_handle*)(clienth))->conetht, e);
 hashtb_seek(e, nid, strlen(nid), 0);
 struct conet_entry* ce = e->data;

 struct hashtb_enumerator ee1;
 struct hashtb_enumerator *e1 = &ee1;
 hashtb_start(ce->chunks, e1);
 if(hashtb_seek(e1, &(param->csn), sizeof(int),0) != 0){//if this entry does no more exist cancel this event
 //ccn_schedule_cancel(sched, ev);
 if( hashtb_n(ce->chunks) == 0){
 hashtb_destroy(&(ce->chunks));
 hashtb_delete(e);
 }
 hashtb_end(e1);
 hashtb_end(e);
 if(ccn_schedule_cancel(sched, ev) == -1){
 if (CONET_DEBUG >= 2) fprintf(stderr, "[CONET]: Attempt to cancel a scheduled resend operation failed\n");
 }
 else{
 if (CONET_DEBUG >= 2) fprintf(stderr, "[CONET]: Attempt to cancel a scheduled resend operation success!\n");
 }
 return 1;//must return a value > 0 to let rescheduling with the new action( ccn_schedule_cancelled_event )
 }

 struct chunk_entry *ch = e1->data;

 long timeout_sec = time.tv_sec - ch->timeout->tv_sec;
 long timeout_microsec = time.tv_usec + (clock_micros_per_base - ch->timeout->tv_usec);

 //TODO XXX the following if never happens in in conet_send_interest_cp we update ch->last_sent every times it's called (5 sec)
 if (CONET_DEBUG >= 2) fprintf(stderr,"[CONET] (%u sec , %u microsec) rescheduling operation start, last interest sent at (%lu sec , %lu microsec), timeout at (%lu sec , %lu microsec)\n", time.tv_sec, time.tv_usec, ch->timeout->tv_sec, ch->timeout->tv_usec, timeout_sec, timeout_microsec );
 if ( ((time.tv_sec * clock_micros_per_base + time.tv_usec) - (ch->last_sent->tv_sec * clock_micros_per_base + ch->last_sent->tv_usec)) > (30 * clock_micros_per_base )){//TODO maybe x2 or x3 the following timeout so that it should happen after 3 retransmissions
 if (CONET_DEBUG >= 2) fprintf(stderr,"[CONET] timeout for chunk %llu for nid: %s  \n", ch->chunk_number, ce->nid);
 //TODO make a function to delete ch!
 hashtb_end(e1);
 hashtb_end(e);
 if(ccn_schedule_cancel(sched, ev) == -1){
 if (CONET_DEBUG >= 2) fprintf(stderr, "[CONET]: Attempt to cancel a scheduled resend operation failed\n");
 }
 else{
 if (CONET_DEBUG >= 2) fprintf(stderr, "[CONET]: Attempt to cancel a scheduled resend operation success!\n");
 }
 return 1;//must return a value > 0 to let rescheduling with the new action( ccn_schedule_cancelled_event )
 }
 if ( ((time.tv_sec * clock_micros_per_base + time.tv_usec) - (ch->timeout->tv_sec * clock_micros_per_base + ch->timeout->tv_usec)) > (5 * clock_micros_per_base)){
 conet_send_interest_cp(ce, ch, 1);
 if (CONET_DEBUG >= 2) fprintf(stderr,"[CONET] retransmission of interest\n");
 }
 else{
 if (CONET_DEBUG >= 2) fprintf(stderr,"[CONET] nothing to do on scheduled retransmission of interest...\n");
 }
 hashtb_end(e1);
 hashtb_end(e);
 return  5 * clock_micros_per_base;


 }
 */

/**
 * Update the timeouts.
 */
void update_timeout(struct conet_entry * ce, struct chunk_entry *ch,
		short rescheduled_send) {

	//	if(rescheduled_send == 0)
	//		gettimeofday(ch->last_sent, NULL);
	//
	//	gettimeofday(ch->timeout, NULL);
}
/**
 * Utility function to setup bitmaps for a new chunk
 */
void setup_chunk_bitmaps(struct conet_entry* ce, struct chunk_entry* ch) {
	unsigned int number_of_remaining_interests = (ce->chunk_size
			/ ch->m_segment_size)
			+ ((ce->chunk_size % ch->m_segment_size > 0) ? 1 : 0); //this is the number of interest we need to send for this chunk less 1 (the already sent one)

	ch->sending_interest_table = realloc(ch->sending_interest_table,
			sizeof(conet_bit) * number_of_remaining_interests);
	ch->expected = realloc(ch->expected, sizeof(conet_bit)
			* number_of_remaining_interests);
	ch->out_of_sequence_received = realloc(ch->out_of_sequence_received,
			sizeof(conet_bit) * number_of_remaining_interests);

	ch->bitmap_size = number_of_remaining_interests;

	unsigned int bitmap_it;
	/*setting initial state for new bitmaps positions associated with this chunk*/
	for (bitmap_it = 0; bitmap_it < number_of_remaining_interests; bitmap_it++) {
		//the first position must not be cleared beacause it has been already used
		ch->sending_interest_table[bitmap_it].bit = 0;
		ch->expected[bitmap_it].bit = 1;
		ch->out_of_sequence_received[bitmap_it].bit = 0;
	}
}
/**
 * Get the next chunk if exist.
 */
struct chunk_entry* chunk_change(struct ccnd_handle* h, struct conet_entry* ce,
		struct chunk_entry* ch, int remaining) {
	struct hashtb_enumerator ee1;
	struct hashtb_enumerator *e = &ee1;

	if (ce->cwnd < ce->in_flight) {
		return NULL;
	}

	int key = ch->chunk_number + 1;
	struct chunk_entry* next_ch = hashtb_lookup(ce->chunks, &key, sizeof(int));
	//	if (CONET_DEBUG >= 2) fprintf(stderr, "\nnext_ch %d\n", key);
	if (next_ch != NULL && next_ch->last_possible_interest > 0) {
		//next_ch->last_possible_interest=next_ch->last_possible_interest + remaining;
		//conet_send_interest_cp(h,ce, next_ch, 0);
		ce->last_processed_chunk = next_ch->chunk_number;
		return next_ch;
	} else {
		if (CONET_PREFETCH == 1) {
			int chunk_number = ce->last_processed_chunk + 1;
			ce->last_processed_chunk++;
			hashtb_start(ce->chunks, e);
			int resc = hashtb_seek(e, &chunk_number, sizeof(int), 0);
			struct chunk_entry* new_ch = e->data;
			if (resc == HT_NEW_ENTRY) {
				new_ch->chunk_number = chunk_number;
				new_ch->m_segment_size = get_segment_size(ce->nid,
						chunk_number, ce->chunk_size, CONET_DEFAULT_MTU);
				new_ch->starting_segment_size = new_ch->m_segment_size;
				new_ch->bytes_not_sent_on_starting = 0;
				new_ch->last_interest = 0;
				new_ch->last_in_sequence = 0;
				new_ch->send_started = 0;
				//last possible interest is updated in fill_the_cwnd
				new_ch->last_possible_interest = 0;
				setup_chunk_bitmaps(ce, new_ch);
				new_ch->return_faceid = ch->return_faceid;
				new_ch->send_faceid = ch->send_faceid;
				new_ch->current_size = 0;
				new_ch->timeout = calloc(1, sizeof(struct timeval));
				new_ch->timeout->tv_sec = 0;
				new_ch->timeout->tv_usec = 0;
				new_ch->last_sent = calloc(1, sizeof(struct timeval));
				new_ch->last_sent->tv_sec = 0;
				new_ch->last_sent->tv_usec = 0;
				new_ch->reschedule_send_ev = NULL;
				new_ch->tries = 0;
				new_ch->never_arrived = 0;
				new_ch->arrived = 0;
				new_ch->is_oos = 0;
				new_ch->to = NULL;

				//				new_ch->outbound = malloc(sizeof(ch->outbound));
				//				memcpy(new_ch->outbound, ch->outbound,
				//						sizeof(ch->outbound));

				new_ch->outbound = ch->outbound;
				new_ch->final_segment_received = 0;
				if (CONET_DEBUG >= 2)
					fprintf(stderr,
							"[conet.c:%d] chunk change, new chunk is %llu\n",
							__LINE__, new_ch->chunk_number);
				//conet_send_interest_cp(h,ce, new_ch, 0);

			} else {
				hashtb_end(e);
				return NULL;
			}

			hashtb_end(e);

			return new_ch;
		}
	}
	return NULL;
}
/**
 * Utility function to print the content in byte of ccnb
 */
void conet_print_ccnb(unsigned char* msg, unsigned int size) {

	int i = 0;
	for (i = 0; i < size; i++) {

		if (i % 40 == 39) {
			if (CONET_DEBUG >= 2)
				fprintf(stderr, "\n ");
		}
		if (CONET_DEBUG >= 2)
			fprintf(stderr, "%02X ", msg[i]);
	}
	if (CONET_DEBUG >= 2)
		fprintf(stderr, "\n");

}

int is_eof(struct ccnd_handle * h, const unsigned char* ccnb, unsigned int size) {
	// XXX The test below should get refactored into the library

	struct ccn_parsed_ContentObject pco = { 0 };
	struct ccn_indexbuf *comps = indexbuf_obtain(h);
	ccn_parse_ContentObject(ccnb, size, &pco, comps);

//	unsigned int ccnb_size = pco.offset[CCN_PCO_E];
	if (pco.offset[CCN_PCO_B_FinalBlockID]
			!= pco.offset[CCN_PCO_E_FinalBlockID]) {
		const unsigned char *finalid = NULL;
		size_t finalid_size = 0;
		const unsigned char *nameid = NULL;
		size_t nameid_size = 0;
		struct ccn_indexbuf *cc = comps;
		ccn_ref_tagged_BLOB(CCN_DTAG_FinalBlockID, ccnb,
				pco.offset[CCN_PCO_B_FinalBlockID],
				pco.offset[CCN_PCO_E_FinalBlockID], &finalid, &finalid_size);
		if (cc->n < 2)
			abort();
		ccn_ref_tagged_BLOB(CCN_DTAG_Component, ccnb, cc->buf[cc->n - 2],
				cc->buf[cc->n - 1], &nameid, &nameid_size);
		if (finalid_size == nameid_size && 0 == memcmp(finalid, nameid,
				nameid_size))
			return (1);
	}
	return (0);
}

void close_chunk_transmission(struct ccnd_handle * h, struct conet_entry* ce) {

	int chunk_num = 0;
	struct chunk_entry* next_ch = NULL;
	if (ce->chunk_expected == NULL) {
		if (CONET_DEBUG >= 2)
			fprintf(stderr, "[conet.c:%d] Chunk expected is null\n", __LINE__);
		return;
	}
	if (ce->final_chunk_number != ce->chunk_expected->chunk_number) {
		for (chunk_num = ce->chunk_expected->chunk_number; chunk_num
				<= ce->last_processed_chunk; chunk_num++) {
			next_ch = hashtb_lookup(ce->chunks, &chunk_num, sizeof(int));
			if (next_ch != NULL) {
				if (next_ch->last_possible_interest <= 0
						|| next_ch->m_segment_size == 0) {
					if (CONET_DEBUG >= 2)
						fprintf(
								stderr,
								"[conet.c:%d] ERROR: this chunk is not instantiated well. Give up ch num: %d lcp: %d\n",
								__LINE__, chunk_num, ce->last_processed_chunk);
					continue;

				}
				if (next_ch->arrived == 0 && next_ch->chunk_number
						< ce->final_chunk_number) {
					if (CONET_DEBUG >= 2)
						fprintf(
								stderr,
								"[conet.c:%d] Chunk %llu wait for its completion. \n",
								__LINE__, next_ch->chunk_number);
					if (next_ch->to != NULL && CONET_DEBUG >= 2)
						fprintf(stderr, "[conet.c:%d] remain %f", __LINE__,
								(get_time() - next_ch->to->time));
					if (CONET_DEBUG >= 2)
						fprintf(stderr, "[conet.c:%d] Chunk %llu "
							"\n \t last interest %d "
							"\n \t last in seq %d \n", __LINE__,
								next_ch->chunk_number, next_ch->last_interest,
								next_ch->last_in_sequence);
				}
			}
		}
	} else {
		close(ce->send_fd);
	}
}

//TODO: andrea.araldo@gmail.com: a cosa serve questa funzione?
int content_match(struct content_entry *content, struct ccn_charbuf* name) {
	int start = content->comps[0];
//	int end = content->comps[content->ncomps - 1];
	const unsigned char* a = content->key + start - 1;
	unsigned char* b = name->buf;
	int bsize = name->length - 1;
	return memcmp(a, b, bsize);
}

int content_match_per_debug(struct content_entry *content,
		struct ccn_charbuf* name) {
	int start = content->comps[0];
//	int end = content->comps[content->ncomps - 1];
	const unsigned char* a = content->key + start - 1;
	unsigned char* b = name->buf;
	int bsize = name->length - 1;
	if (CONET_DEBUG >= 2) {
		unsigned char string_a[bsize + 1], string_b[bsize + 1];
		memset(string_a, '\0', bsize + 1);
		memset(string_b, '\0', bsize + 1);
		memcpy(string_a, a, bsize);
		memcpy(string_b, b, bsize);
		fprintf(stderr, "[conet.c:%d]Comparing \"%s\" with \"%s\"\n", __LINE__,
				string_a, string_b);
		fprintf(stderr, "[conet.c:%d]Comparation bit a bit: ", __LINE__);
		int y;
		for (y = 0; y < bsize; y++)
			fprintf(stderr, "%X - %X;  ", string_a[y], string_b[y]);
		fprintf(stderr, "\n");
	}
	int comparation = memcmp(a, b, bsize);
	//	if (CONET_SEVERE_DEBUG && comparation!= 0)  exit(-11);
	return comparation;
}
/**
 * Print some stats
 */
void conet_print_stats(struct conet_entry* ce, struct chunk_entry* ch,
		unsigned int line) {
	fprintf(stderr, "%d]"
		"cwnd: %d \t"
		"threshold: %d \t"
		"in_flight: %d \t"
		"in fast rec: %d \t"
		"oos: %d \t"
		"retrasmit: %d \t"
		"\n", line, ce->cwnd, ce->threshold, ce->in_flight, ce->in_fast_rec,
			conet_oos_stat, conet_retransmit_stat);
}

int check_for_internal_ccn_interest(unsigned char* interest, unsigned int size) {
	char *reserved_nid[] = { "/%C1.M.S.localhost/", "/%C1.M.S.neighborhood",
			"/ccnx/", "/ccnx.org", "/parc.com" };
	int reserved_size = 3;
	int i;
	struct ccn_charbuf *c = ccn_charbuf_create();
	ccn_uri_append(c, interest, size, 0);

	char *uri = ccn_charbuf_as_string(c);
	//ccn_charbuf_destroy(&c);
	for (i = 0; i < reserved_size; i++) {
		//		if (CONET_DEBUG >= 2) fprintf(stderr, "\nIS %s == %s? %d\n", reserved_nid[i], uri, strncmp(uri,reserved_nid[i], strlen(reserved_nid[i])-1));

		if (strncmp(uri, reserved_nid[i], strlen(reserved_nid[i]) - 1) == 0) {
			if (CONET_DEBUG >= 2)
				fprintf(stderr,
						"\n%s is a reserved ccnx address don't use conet\n",
						reserved_nid[i]);
			ccn_charbuf_destroy(&c);
			return 0;
		}
	}
	ccn_charbuf_destroy(&c);
	return -1;
}

void log_to_file(FILE* fd, int a1, int a2, int a3) {
	double now = get_time();
	if (start_time == 0) {
		start_time = now;
	}
	int ret = fprintf(fd, "%f \t %d \t %d \t %d \n ", (now - start_time), a1,
			a2, a3);
	fflush(fd);
	if (ret < 0) {
		perror("Error in write to log file. Abort");
		abort();
	}
}

void remove_chunk(struct conet_entry *ce, struct chunk_entry* ch) {
	if (ce->chunk_expected != NULL && ce->chunk_expected->chunk_number
			== ch->chunk_number) {
		ce->chunk_expected = NULL;
	}
	if (ch->chunk != NULL) {
		free(ch->chunk);
		ch->chunk = NULL;
	}
	if (ch->expected != NULL) {
		free(ch->expected);
		ch->expected = NULL;
	}
	if (ch->out_of_sequence_received != NULL) {
		free(ch->out_of_sequence_received);
		ch->expected = NULL;
	}
	if (ch->sending_interest_table != NULL) {
		free(ch->sending_interest_table);
		ch->sending_interest_table = NULL;
	}
	if (ch->timeout != NULL) {
		free(ch->timeout);
		ch->timeout = NULL;
	}
	if (ch->last_sent != NULL) {
		free(ch->last_sent);
		ch->last_sent = NULL;
	}
	if (ch->to != NULL) {
		remove_entry(ch->to);
		ch->to->ce = NULL;
		ch->to->ch = NULL;
		free(ch->to);
	}
	struct hashtb_enumerator ee;
	struct hashtb_enumerator *e1 = &ee;

	hashtb_start(ce->chunks, e1);
	int r = hashtb_seek(e1, &(ch->chunk_number), sizeof(int), 0);
	if (r == HT_OLD_ENTRY) {
		hashtb_delete(e1);
	}

	hashtb_end(e1);
}

int get_uri_from_ccn(unsigned char* ccnb, int size) {
	int ncomp = 0;
	const unsigned char *comp = NULL;
	size_t compsize = 0;
	struct ccn_buf_decoder decoder;
	struct ccn_buf_decoder *d = ccn_buf_decoder_start(&decoder, ccnb, size);
	struct ccn_charbuf *c = ccn_charbuf_create();
	if (CONET_DEBUG >= 2)
		fprintf(stderr, "BBBB00:  \n");

	if (ccn_buf_match_dtag(d, CCN_DTAG_Interest) || ccn_buf_match_dtag(d,
			CCN_DTAG_ContentObject)) {
		ccn_buf_advance(d);
		if (ccn_buf_match_dtag(d, CCN_DTAG_Signature))
			ccn_buf_advance_past_element(d);
	}
	if (!ccn_buf_match_dtag(d, CCN_DTAG_Name))
		return -1;

	ccn_buf_advance(d);

	int k = 0;

	while (ccn_buf_match_dtag(d, CCN_DTAG_Component)) {
		ccn_buf_advance(d);
		compsize = 0;
		if (ccn_buf_match_blob(d, &comp, &compsize)) {
			if (CONET_DEBUG >= 2)
				fprintf(stderr, "BLOB \n");
			ccn_buf_advance(d);
		}
		ccn_buf_check_close(d);
		if (d->decoder.state < 0)
			return (d->decoder.state);
		ncomp += 1;
		ccn_charbuf_append(c, "/", 1);
		ccn_uri_append_percentescaped(c, comp, compsize);
		k++;
		if (CONET_DEBUG >= 2)
			fprintf(stderr, "BBBB%d: %s  %d\n", ncomp,
					ccn_charbuf_as_string(c), k);
	}

	if (CONET_DEBUG >= 2)
		fprintf(stderr, "BBBBXXX: %d \n", k);

	struct ccn_buf_decoder decoder1;
	struct ccn_buf_decoder *d1 = ccn_buf_decoder_start(&decoder1, ccnb, size);
	struct ccn_charbuf *c1 = ccn_charbuf_create();
	if (CONET_DEBUG >= 2)
		fprintf(stderr, "BBBB00:  \n");

	if (ccn_buf_match_dtag(d1, CCN_DTAG_Interest) || ccn_buf_match_dtag(d1,
			CCN_DTAG_ContentObject)) {
		ccn_buf_advance(d1);
		if (ccn_buf_match_dtag(d1, CCN_DTAG_Signature))
			ccn_buf_advance_past_element(d1);
	}
	if (!ccn_buf_match_dtag(d1, CCN_DTAG_Name))
		return -1;

	ccn_buf_advance(d1);

	int k1 = 0;

	while (ccn_buf_match_dtag(d1, CCN_DTAG_Component)) {
		ccn_buf_advance(d1);
		compsize = 0;
		if (ccn_buf_match_blob(d1, &comp, &compsize)) {
			if (CONET_DEBUG >= 2)
				fprintf(stderr, "BLOB \n");
			ccn_buf_advance(d1);
		}
		ccn_buf_check_close(d1);
		if (d->decoder.state < 0)
			return (d->decoder.state);
		ncomp += 1;
		ccn_charbuf_append(c1, "/", 1);
		k1++;
		if (k1 == k) {
			int num = *comp;
			if (CONET_DEBUG >= 2)
				fprintf(stderr, "DDDD: %d\n", num);
		}
	}

	if (CONET_DEBUG >= 2)
		fprintf(stderr, "BBBBXXX: %d \n", k1);
	ccn_charbuf_destroy(&c);
	ccn_charbuf_destroy(&c1);
	return ncomp;
}

int ccnb_extract_info(struct ccn_parsed_interest *pi, const unsigned char *msg,
		size_t size, int *chunk_number, struct ccn_charbuf* nid) {
	struct ccn_buf_decoder decoder;
	const unsigned char* ccnb = msg + pi->offset[CCN_PI_B_Name];
	size -= pi->offset[CCN_PI_B_Name];
	struct ccn_buf_decoder *d = ccn_buf_decoder_start(&decoder, ccnb, size);

//	int res = -1;
//	int prec_res = -1;
	const unsigned char* comp = NULL;
	size_t compsize = 0;
	if (ccn_buf_match_dtag(d, CCN_DTAG_Name)) {
		ccn_buf_advance(d);
//		res = d->decoder.token_index; /* in case of 0 components */
		while (ccn_buf_match_dtag(d, CCN_DTAG_Component)) {
			if (compsize != 0 && comp != NULL) {
				ccn_charbuf_append(nid, "/", 1);
				ccn_charbuf_append(nid, comp, compsize);
			}
//			prec_res = res;
//			res = d->decoder.token_index;
			ccn_buf_advance(d);
			compsize = 0;
			if (ccn_buf_match_blob(d,&comp, &compsize))
				ccn_buf_advance(d);

			ccn_buf_check_close(d);
		}
		ccn_buf_check_close(d);
	}
	if (CONET_USE_REPO == 1) {
		int num = 0;
		int i = 0, j = 0;
		for (i = compsize - 1; i >= 0; i--) {
			num += comp[i] << (j * 8);
			j++;
		}

		*chunk_number = num;
	} else {
		*chunk_number = atoi((char*)comp);
	}
	return compsize;
}

//non viene utilizzata e piana di warning
//int extract_comps_from_name(struct ccn_indexbuf* comps,
//		struct ccn_charbuf* name) {
//	unsigned char* n = name->buf;
//	int more_c;
//	const unsigned char* cp;
//	unsigned int cp_size;
//	unsigned int ncomps = 0;
//	for (;;) {
//		more_c = ccn_buf_match_dtag(n, CCN_DTAG_Component);
//		if (more_c == 0) {
//			break;
//		}
//		ccn_buf_advance(n);
//		if (ccn_buf_match_blob(n, &cp, &cp_size)) {
//			ccn_indexbuf_append(comps, cp, cp_size);
//			ccn_buf_advance(n);
//		}
//
//		ncomps++;
//		ccn_buf_check_close(n);
//	}
//	return (ncomps);
//}

#ifdef CONET_MULTI_HOP

#define ETHHDR_SIZE 14
void change_eth_dst_src(struct sockaddr_in* next_hop, unsigned char * packet) {
	int ifindex = 0;
	unsigned char dst_mac[6];
	memset(dst_mac, 0, 6);
	struct ip *iphdr = (struct ip *) (packet + ETHHDR_SIZE);

	int ret_mac = get_mac_from_ip(dst_mac, next_hop, &ifindex);
	if (ret_mac != 0) {
		if (CONET_DEBUG >= 2)
		fprintf(stderr,
				"conet.c[%d] error in getting MAC, dst MAC set to 00\n",
				__LINE__);
	}

	unsigned char *src_mac = local_mac;

	memcpy((void *) packet, (void *) dst_mac, 6);
	memcpy((void *) (packet + 6), (void *) src_mac, 6);
	inet_pton(AF_INET, local_ip, &(iphdr->ip_src.s_addr));
	iphdr->ip_dst.s_addr = next_hop->sin_addr.s_addr;

}

int propagate_interest_cp(struct ccnd_handle* h, char *src_addr, unsigned char* readbuf, unsigned short ll, unsigned short c, unsigned short ask_info, unsigned short is_raw,int size) { // name dovrebbe venire gia giusto per la nameprefix seek

	//prima parte presa dalla process_interest
	struct conet_entry* ce=NULL;
	char* nid;
	unsigned short nid_length;
	unsigned char* iterator;
	unsigned int is_final = 0;
	int res = 0;

	if (c == CONET_CIU_NOCACHE_FLAG) {
		//simply forward the interest without further analyze
		if (CONET_DEBUG >= 2)
		fprintf(
				stderr,
				"[conet.c:%d] Interest cp has flag 'no cache' forwarding \n",
				__LINE__);
		return 0;
	}
	//iterator on the first byte of NID field
	if (is_raw)
	iterator = readbuf + 1 + 2; //ip_opt len +flag + DS&T field
	else
	iterator = readbuf + 1;//caso udp da rivedere
	nid = obtainNID(&iterator, ll);
	//if (CONET_DEBUG >= 2) fprintf(stderr, "[CONET]: NID:%s\n", nid);
	nid_length = strlen(nid);
	if (CONET_DEBUG && ll == 2) {
		if (CONET_DEBUG >= 2)
		fprintf(stderr, "[conet.c:%d]with strlen nid_length=%hu, nid= ",
				__LINE__, nid_length);
		int t;
		for (t = 0; t < nid_length; t++)
		if (CONET_DEBUG >= 2)
		fprintf(stderr, "%.2X ", iterator[1 + t]);
		if (CONET_DEBUG >= 2)
		fprintf(stderr, "\n");

	}
	if (ll == 0) {
		//iterator on the first byte of CSN field
		iterator = iterator + nid_length;
	} else if (ll == 2) {
		//iterator on the first byte of CSN field
		if (CONET_DEBUG >= 2)
		fprintf(stderr, "[conet.c:%d]The one byte length field is %.2X\n",
				__LINE__, iterator[0]);
		iterator = iterator + 1 + nid_length;
	} else {
		if (CONET_DEBUG >= 2)
		fprintf(stderr,
				"[CONET]: Using reserved ll-flag in input message. Not yet supported\n");
		//TODO handle this case
		abort();
	}

	int pos = iterator - readbuf;
	unsigned long long csn = read_variable_len_number(readbuf, &pos);

	if (is_raw) //da controllare
	pos = (int) readbuf[0] - 1; // - ip_opt len

#ifdef IPOPT
	if (is_raw) {
		pos += 4; //jump 4 byte openflow tag
	}
#endif

	unsigned char cp_flag = (unsigned char) *(readbuf + pos);
	if (CONET_DEBUG >= 2)
	fprintf(stderr, "[conet.c:%d]cp_flag=%2X, readbuf[pos]=%2X\n",
			__LINE__, cp_flag, readbuf[pos]);

	pos++;

	//	if ((cp_flag & CONET_ASK_CHUNK_INFO_FLAG) != 0) {
	//		ask_info = 1;
	//	}

	unsigned long long l_edge = read_variable_len_number(readbuf, &pos);
	//if (CONET_DEBUG >= 2) fprintf(stderr, "[CONET]: left edge:%u\n", (unsigned int)l_edge);

	unsigned long long r_edge = read_variable_len_number(readbuf, &pos);
	//if (CONET_DEBUG >= 2) fprintf(stderr, "[CONET]: right edge:%u\n", (unsigned int)r_edge);

	if (CONET_MULTI_HOP_DEBUG)
		fprintf(stderr,"[conet.c:%d]: Received an interest for %s/%llu left edge:%llu, right edge:%llu \n",
			__LINE__, nid, csn, l_edge, r_edge);

	char *uri = calloc(255, sizeof(char));
	sprintf(uri, "%s/%llu", nid, csn);

	//if (CONET_DEBUG >= 2) fprintf(stderr, "[conet.c:%d]: Search %s in cache \n", __LINE__,uri );

	struct ccn_charbuf *name = ccn_charbuf_create();
	struct ccn_charbuf *name_dec = ccn_charbuf_create();

	int res0 = ccn_name_from_uri(name, nid);
	res0 += ccn_name_append_numeric(name, CCN_MARKER_SEQNUM, csn);
	res0 += ccn_name_from_uri(name_dec, uri);

	struct content_entry* content_e = NULL;

	content_e = find_ccn_content(h, name);
	//controllo se ho la route
	if (content_e == NULL || content_e->size - 36 == 0) { // qui forse posso evitare di entrare
		if (1)
		fprintf(stderr,
				"[conet.c:%d]: No content found for %s/%llu. Dropped\n",
				__LINE__, nid, csn);

		//start border node processing
		if (CONET_FORWARDING) {
			if (CONET_DEBUG >= 2)
			fprintf(stderr, "[conet.c:%d]: EXITING !!!\n", __LINE__);
			//			abort(); // so that we do not enter in a not tested part !!!
//			unsigned char * msg_ONLY_FOR_SIGNATURE;
			//			return propagate_interest_cp(h, name, msg_ONLY_FOR_SIGNATURE);
		} else {
			//			ccn_charbuf_destroy(&name);
			//			ccn_charbuf_destroy(&name_dec);
			//			free(nid);
			//			free(uri);
			//			return -1;
		}
	} else {//entro qui se conosco la nid (ho la route)
		//qui devo vedere se ho robba in cache
		if (content_match(content_e, name) == 0) {
			//invio il contenuto come server
			if (CONET_MULTI_HOP_DEBUG)
			fprintf(stderr, "[conet.c:%d]: PRIMO TENTATIVOok %d\n",
					content_e->key_size, __LINE__);
			if (content_e != NULL || content_e->size - 36 != 0) {
				ccn_charbuf_destroy(&name);
				ccn_charbuf_destroy(&name_dec);
				free(nid);
				free(uri);
				return conet_process_interest_cp(h, src_addr, readbuf, ll, c,
						ask_info, is_raw);
			}
		} else {
			content_e = find_ccn_content(h, name_dec);
			if (content_match(content_e, name_dec) == 0) {
				if (1)
				fprintf(stderr, "[conet.c:%d]:SECONDO TENTATIVO ok %d\n",
						content_e->key_size, __LINE__);
				//match al secondo tentativo  invio il contenuto come server
				//			return conet_process_interest_cp(h,src_addr,readbuf,ll,c,ask_info, is_raw) ;
			}
		}
	}

	struct hashtb_enumerator ee;
	struct hashtb_enumerator *e = &ee;

	//qui inserisco nella pit (bo forse prima adesso mi sembra diverso)
//	struct s_pit_entry *spe;
	int seg_index = (r_edge/(r_edge-l_edge))-1;
	char *uri2 = calloc(S_PIT_ENTRY_NOME_SIZE, sizeof(char));
	sprintf(uri2, "%s/%d", uri,seg_index);
	if (CONET_MULTI_HOP_DEBUG)
		fprintf(stderr,"[conet.c:%d]: Received an interest for %s/%llu left edge:%llu, right edge:%llu \n", __LINE__, nid, csn, l_edge, r_edge);

	//inizializzo le cose per la ricostruzione del contenuto
	res = 0;
	hashtb_start(h->conetht, e);
	res = hashtb_seek(e, nid, strlen(nid), 0);

	if (res < 0) {
		perror("[ccnd.c]: seek error in h->conetht\n");
		exit(-1);
	}

	ce = e->data;
	if (res == HT_NEW_ENTRY) { //new entry in the hash table of the nids
		//qui devo fare un controllo , inserisco solo se e' il primo cp
		if (csn == 0 && l_edge ==0 ) {
			if (CONET_MULTI_HOP_DEBUG)
			fprintf(
					stderr,
					"[conet.c:%d]:new entry in the hash table of the nids \n",
					__LINE__);

			ce->nid = nid;
			ce->chunk_expected = NULL;
			ce->last_processed_chunk = 0;
			ce->chunk_counter = 0;
			ce->final_chunk_number = -1;
			ce->send_fd = -1;
			ce->chunks = hashtb_create(sizeof(struct chunk_entry), NULL);
			//			ce->c_addr = inet_addr(src_addr);
		} else { //se ho gia completato il contenuto, quindi rimosso la nid filtro i cp in piu del prefetch

			if (1){
				fprintf(stderr, "[conet.c:%d]:filtro i cp in piu del prefetch \n", __LINE__);
				fprintf(stderr,"[conet.c:%d]: Received an interest for %s/%llu left edge:%llu, right edge:%llu \n", __LINE__, nid, csn, l_edge, r_edge);
			}
			hashtb_delete(e);
			hashtb_end(e);
			return -1;
		}
	} else {
		if (CONET_MULTI_HOP_DEBUG)
		fprintf(stderr,"[conet.c:%d]:old entry in the hash table of the nids \n",__LINE__);
	}
	hashtb_end(e);

	hashtb_start(ce->chunks, e);
	int resc = hashtb_seek(e, &csn, sizeof(int), 0);
	if (resc == -1) {
		if (CONET_DEBUG >= 2)
		fprintf(
				stderr,
				"[ccnd.c:%d]: error in seeking or creating chunks in hash table\n",
				__LINE__);
		hashtb_end(e);
		exit(-1); // i have broken the chunk hashtable!
	}
	struct chunk_entry *ch = e->data;
	if (resc == HT_NEW_ENTRY) { //new entry in the hash table of chunks for a nid
		ch->chunk_number = csn;
		ch->m_segment_size = get_segment_size(nid, csn, ce->chunk_size,
				CONET_DEFAULT_MTU);
		ch->starting_segment_size = ch->m_segment_size;
		ch->return_faceid = 0; // faceid finta whr
		ch->bytes_not_sent_on_starting = 0;
		ch->current_size = 0;
		ch->inviato = 0;
		ch->s_pit = hashtb_create(sizeof(struct s_pit_entry), NULL);

		if (CONET_MULTI_HOP_DEBUG)
		fprintf(stderr,
				"[conet.c:%d]:new entry in the hash table of  CHUNKS \n",
				__LINE__);
		ch->bitmap_size = 4; //per adesso e' harcodato whr

	} else {
		if (CONET_MULTI_HOP_DEBUG)
			fprintf(stderr,
				"[conet.c:%d]:old entry in the hash table of  CHUNKS \n",
				__LINE__);
		//controllo se il chunk contiene gia il cp
		if (ch->cp_bitmap & (1 << seg_index)) {
//			fprintf(stderr, "HO IL CP %s E LO MANDO A  %s \n", uri2,
//					src_addr);
			int segment_size = r_edge - l_edge + 1;

			if (ch->current_size - l_edge < segment_size) {
				//end of chunk with a dimension different from a multiple of segment-dimension reached
				if (ch->current_size == 0) {
					r_edge = 0; //so cazzi?
				} else {
					r_edge = ch->current_size - 1;
					is_final = 1;
				}
				segment_size = r_edge - l_edge + 1;
			}
			return conet_send_data_cp(src_addr, ch->current_size, ch->chunk+l_edge, segment_size,
					nid, csn, l_edge, r_edge, is_final, ask_info, is_raw);//controllare ch->current_size
		}
	}
	hashtb_end(e);
	//qui inserisco nella pit
	//pit integrata nel chunk si puo evitare di usare un hashtable
	struct s_pit_entry *spe1;
	res = 0;
	hashtb_start(ch->s_pit, e);
	res = hashtb_seek(e, &seg_index, sizeof(int), 0);
	if (res < 0) {
		perror("[ccnd.c]: seek error in h->s_pit\n");
		exit(-1);
	}
	spe1 = e->data;
	if (res == HT_NEW_ENTRY) {
		if (CONET_MULTI_HOP_DEBUG)
		fprintf(stderr,
				"[conet.c:%d]: HT_NEW_ENTRY inserisco in s_pit %s \n",
				__LINE__, uri2);
		spe1->seg_index = seg_index;
		spe1->c_addr = hashtb_create(sizeof(in_addr_t), NULL);
	} else if (res == HT_OLD_ENTRY) { //questo cp e' stato gia inviato non lo devo inoltrare
		//come lo distinguo da una ritrasmissione?
		if (CONET_MULTI_HOP_DEBUG)
		fprintf(stderr, "[conet.c:%d]: HT_OLD_ENTRY filtro in s_pit %s \n",
				__LINE__, uri2);
		//		return 0; //forse non lo usavo
	}

	hashtb_end(e);

	in_addr_t clia_addr = inet_addr(src_addr);
	struct addr_entry *addre;
	//ho inserito la pit entry associata al cp adesso inserisco l'indirizzo
	//uso indirizzo per gestire le ritrasmissioni
	//se il cp e' richiesto da un indirizzo gia visto, inoltro la richiesta (ritrasmissione)
	//se il cp e' richiesto da un indirizzo nuovo non inoltro il cp perche' una richiesta simile e' gia'
	//in viaggio

	hashtb_start(spe1->c_addr, e);
	int res1 = hashtb_seek(e, &clia_addr, sizeof(in_addr_t), 0);

	if (res1 == HT_NEW_ENTRY) {// posso decidere se inviare o no
		if (CONET_MULTI_HOP_DEBUG)
		fprintf(
				stderr,
				"[conet.c:%d]: inserisco  in s_pit %s , ind %s per la prima volta\n",
				__LINE__, uri2, src_addr);
		addre = e->data;
		addre->clia_addr = clia_addr;
		if (!(res1 == HT_NEW_ENTRY && res == HT_NEW_ENTRY)) {
			if (CONET_MULTI_HOP_DEBUG)
			fprintf(stderr,
					"[conet.c:%d]: !!!!!!!!!FILTRATO cp   %s , ind %s\n",
					__LINE__, uri2, src_addr);
			return 0;
		}
	} else if (res1 == HT_OLD_ENTRY) { //e' una ritrasmissione quindi invio
		if (CONET_MULTI_HOP_DEBUG)
		fprintf(stderr,
				"[conet.c:%d]: ******RITRASMISSIONE cp   %s , ind %s\n",
				__LINE__, uri2, src_addr);
		//dovrei poter saltare la ricostruzione del contenuto (bo)
	} else {
		perror("[ccnd.c]: seek error in spe->c_addr\n");
		exit(-1);
	}
	hashtb_end(e);

	// da qui in poi cerco nella fib e mando l'interest

	struct nameprefix_entry *npe = NULL; //whr
	struct ccn_buf_decoder decoder;
	struct ccn_buf_decoder *d = ccn_buf_decoder_start(&decoder, name_dec->buf,name_dec->length);

	hashtb_start(h->nameprefix_tab, e);

	struct ccn_indexbuf *comps = ccn_indexbuf_create();
//	int ncomps = ccn_parse_Name(d, comps);
	ccn_parse_Name(d, comps);
	res = nameprefix_seek(h, e, name_dec->buf, comps, comps->n - 3); //uno a cazzo uno e' /mb1 e uno e' il chunck number
	npe = e->data; //whr
	hashtb_end(e);
	ccn_charbuf_destroy(&name);
	ccn_charbuf_destroy(&name_dec);

	if (npe != NULL) {
		if (CONET_MULTI_HOP_DEBUG)
		fprintf(stderr, "[FORWARDING] trovata entry per la nid \n");
	} else {

		free(nid);
		free(uri);
		return -1;
	}

	update_forward_to(h, npe); //fondamentale senno non funziona niente

	//spedisco il messaggio al server
	struct sockaddr_in next_hop_in;
	struct sockaddr* to_addr;
	socklen_t addr_size;
	int sock = 0;
	int sent = 0;

	unsigned faceid = npe->forward_to->buf[0]; //decidere cosa fare in caso di piu face
	struct face *face = face_from_faceid(h, faceid);

	if (is_raw) {
		if (raw_toser_sock == 0) {
			if ((sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
				perror("[CONET]: Failed to create socket");
				free(nid);
				free(uri);
				return -1;
			}
			raw_toser_sock = sock;
		} else {
			sock = raw_toser_sock;
		}
	} else {
		sock = sending_fd(h, face);
	}

	if (sock <= 0) {
		free(nid);
		free(uri);
		return -1;
	}

	memset(&next_hop_in, 0, sizeof(next_hop_in)); /* Clear struct */
	next_hop_in.sin_family = AF_INET; /* Internet/IP */
	next_hop_in.sin_addr.s_addr
	= ((struct sockaddr_in*) face->addr)->sin_addr.s_addr; /* IP address */
	if(is_raw)
	next_hop_in.sin_port = htons(CONET_DEFAULT_RAW_PORT_NUMBER); /* server port */
	else
	next_hop_in.sin_port = htons(atoi(CONET_DEFAULT_UNICAST_PORT)); /* server port */

	if(is_raw) {
//		int i;
		int prev=14 +0 + 20 + 1; //whr
		change_eth_dst_src(&next_hop_in,readbuf-prev);
		/*
		 for (i = 0; i < size; i++) {
		 if (i % 16 == 0) {
		 fprintf(stderr, "%04X  ", i);
		 }
		 fprintf(stderr, "%02X ", (readbuf-prev)[i]);
		 if (i % 16 == 15)
		 fprintf(stderr, "\n");
		 }
		 */
		to_addr = setup_raw_sockaddr(&next_hop_in);
		addr_size = 20; //solo ipv4 adattare anche ad ipv6
		sent = sendto(sock, readbuf-prev, size, 0, to_addr, addr_size);
	} else {
		to_addr = (struct sockaddr*) &next_hop_in;
		addr_size = face->addrlen;
		sent = sendto(sock, readbuf, size, 0, to_addr, addr_size);
	}

	if (sent == -1)
	fprintf(stderr, "[conet.c:%d]: Sendto error %s \n", __LINE__, strerror(errno));
	free(nid);
	free(uri);
	return sent;
}

int propagate_data_cp(struct ccnd_handle* h, char *src_addr,unsigned char* readbuf, unsigned short ll, unsigned short c,unsigned short is_raw, int msg_size)
{
	char* nid;
	unsigned short nid_length;
	unsigned char* iterator;
//	int ret = 0;
	unsigned short is_final = 0;
//	unsigned int retrasmit_start = 0;
	struct conet_entry* ce = NULL;

	if (is_raw)
	iterator = readbuf + 1 + 2; //ip_opt len +flag + DS&T field
	else
	iterator = readbuf + 1;//caso udp da rivedere

	nid = obtainNID(&iterator, ll);
	nid_length = strlen(nid);

	if (ll == 0)
	iterator = iterator + nid_length;//iterator on the first byte of CSN field
	else if (ll == 2)
	iterator = iterator + 1 + nid_length;//iterator on the first byte of CSN field
	else {
		if (CONET_DEBUG >= 2)
		fprintf(stderr, "[conet.c:%d] ll=%d this is an unsupported flag\n",
				__LINE__, ll);
		abort();
	}
	int pos = iterator - readbuf; //pos is the position of the field after NID in byte array readbuf containing the data-cp


	// primo pezzo in cui devo reperire la conet entry e un po di info dal pacchetto

	ce = hashtb_lookup(h->conetht, nid, nid_length);

	if (ce == NULL) {
		//data-cp richiesto da altri decidere se fare cache
		if (CONET_MULTI_HOP_DEBUG)
		fprintf(stderr, "[conet.c:%d] No entry for nid %s discard this carrier packet and return -3\n", __LINE__, nid);
		return -1;
	}

	unsigned long long csn = read_variable_len_number(readbuf, &pos);

	ce->last_processed_chunk =csn;

	if (is_raw) //da controllare
	pos = (int) readbuf[0] - 1; // - ip_opt len

#ifdef IPOPT
	if (is_raw) {
		pos += 4; //jump 4 byte openflow flag
	}
#endif

	unsigned char cp_flag = (unsigned char) *(readbuf + pos);
	pos++;

	unsigned long long l_edge = read_variable_len_number(readbuf, &pos);
	unsigned long long r_edge = read_variable_len_number(readbuf, &pos);

	if ((cp_flag & CONET_FINAL_SEGMENT_FLAG) != 0) {
		if (CONET_DEBUG >= 2)
		fprintf(
				stderr,
				"[conet.c: %d, conet_process_data_cp(..)]: This is a final segment (cp_flag=%d)\n",
				__LINE__, cp_flag);
		is_final = 1;
	} else if (CONET_DEBUG >= 2)
	fprintf(
			stderr,
			"[conet.c: %d, conet_process_data_cp(..)]: This is NOT a final segment (cp_flag=%d)\n",
			__LINE__, cp_flag);

	if (CONET_DEBUG >= 2)
	fprintf(stderr, "[conet.c:%d] Data cp for %s/%d left:%d, right:%d \n",
			__LINE__, nid, (unsigned int) csn, (unsigned int) l_edge, (unsigned int) r_edge);

	//secondo pezzo qui me la vedo col chunk e capisco se l'ho completato se si lo mando a ccnx oppurebooooooooo

	struct chunk_entry* ch;

	ch = hashtb_lookup(ce->chunks, &csn, sizeof(int));

	if (ch == NULL) { //data-cp richiesto da altri decidere se fare cache
		if (CONET_MULTI_HOP_DEBUG)
		fprintf(stderr, "[conet.c:%d] Chunk %u is unexpected return -4 \n", __LINE__, (unsigned int) csn);
		return -1;
	}

	unsigned char chsz_flag = cp_flag >> 4; //first 4 bits of cp_flag contain the chunk size
	if ((unsigned short) (chsz_flag) != CONET_CONTINUATION) {
		if ((unsigned short) (chsz_flag) == CONET_FOLLOW_VAR_CHUNK_SIZE) {
			ce->chunk_size = read_variable_len_number(readbuf, &pos);
		} else {
			ce->chunk_size = 1 << (9 + (unsigned short) chsz_flag); //TODO check (not so useful in ccnx context)
		}

		if (CONET_DEBUG >= 2)
		fprintf(stderr, "[conet.c:%d] Chunk size is: %llu\n", __LINE__,
				ce->chunk_size);

		ch->starting_segment_size = r_edge - l_edge + 1;

		if (ch->starting_segment_size < ce->chunk_size) {
			//this is the chunk size less the received bytes in the first data-cp
			unsigned long long reduced_chunk_size = ce->chunk_size
			- ch->starting_segment_size;
			//get the segment size requestable in the next interest extimating variable fields like left and right (in the worst case adapted to received chunk size)
			if (CONET_DEBUG >= 2)
			fprintf(stderr, "[conet.c:%d] m_segment_size was %u\n",
					__LINE__, ch->m_segment_size);
			ch->m_segment_size = get_segment_size(nid, csn, reduced_chunk_size,
					CONET_DEFAULT_MTU);
			if (CONET_DEBUG >= 2)
			fprintf(stderr, "[conet.c:%d] Now m_segment_size is %u\n",
					__LINE__, ch->m_segment_size);

			if (ch->m_segment_size > ch->starting_segment_size) {
				ch->bytes_not_sent_on_starting = ch->m_segment_size
				- ch->starting_segment_size;
			} else {
				ch->bytes_not_sent_on_starting = 0;
			}
		}
	}

	int seg_index = 0;
	seg_index = calculate_seg_index(l_edge, r_edge, ch->m_segment_size,
			ch->starting_segment_size, ch->bytes_not_sent_on_starting);

	unsigned int seg_size = r_edge - l_edge + 1;
	// robba mia controllo se il cp che mi e' arrivato e' robba nuova o no
	// se si alloco nuovo spazio
	if (r_edge + 1 > ch->current_size) {
		ch->chunk = realloc(ch->chunk, r_edge + 1);
		ch->current_size = r_edge + 1;
	}
	if ((ch->cp_bitmap &(1<<seg_index))==0) {
		// copio la robba  deontro il chunk
		memcpy(ch->chunk + l_edge, readbuf + pos, seg_size);
	}

	struct hashtb_enumerator ee;
	struct hashtb_enumerator *e = &ee;
	int r=0;

	ch->cp_bitmap = ch->cp_bitmap |(1<<seg_index); //segno il cp arrivato

	int is_eof_value=-1;
	if (is_final == 1) { // qui devo controllare anche se sono arrivati i cp precedenti a quello finale

		//ricevo l'ultimo cp non e' detto che il chunk e' completo
		ch->final_segment_received = 1;
		if (CONET_MULTI_HOP_DEBUG)
		fprintf(stderr, "[conet.c:%d] chunk %llu completed!(forse)\n",
				__LINE__, ch->chunk_number);
		is_eof_value = is_eof(h, ch->chunk, ch->current_size);
		if(is_eof_value) {
			ch->bitmap_size = seg_index+1;//questo chunk e' piu piccolo
			ce->completed = 1; // e' arrivato l'ultimo chunk
			ce->final_chunk_number = ch->chunk_number;
			if (CONET_MULTI_HOP_DEBUG)
			fprintf(stderr, "[conet.c:%d] is_eof=%d \n", __LINE__,
					is_eof_value);
		}
		ch->arrived = 1;
	}

	//comincio a controllare se e' arrivato tutto il chunk solo dopo aver ricevuto l'ultimo cp
	if(ch->arrived) {
		//creo la maschera di bit da confrontare con quella del cp
		//si puoi ottimizzare molto
		int i;
		int mask=0;
		for (i=0;i<ch->bitmap_size;i++) {
			mask= mask | (1<<i);
		}
		if (ch->cp_bitmap == mask ) {
			if (CONET_MULTI_HOP_DEBUG)
			fprintf(stderr, "[conet.c:%d] arrivato   ch->current_size %d \n",__LINE__,ch->current_size);
			if (!ch->inviato) {
				conet_send_chunk_to_ccn(h, ch, ch->current_size); //invio il chunk a ccnx
				ch->inviato =1;
				hashtb_start(ce->chunks, e);
				r = hashtb_seek(e, &(ch->chunk_number), sizeof(int), 0);
				if (r == HT_OLD_ENTRY) {
					//hashtb_delete(e1);//qui non cancello ma libero
					free(ch->chunk);
					if (CONET_MULTI_HOP_DEBUG)
					fprintf(stderr,
							"[conet.c:%d] liberto il chunk  %llu \n",
							__LINE__, ch->chunk_number);
					ce->chunk_counter++;
				}
				hashtb_end(e);
			} else {
				if (CONET_MULTI_HOP_DEBUG)
				fprintf(stderr, "[conet.c:%d]  chunk  %llu  gia' inviato,ritorno\n", __LINE__, ch->chunk_number);
				return 0;
			}

			if (ce->completed) { //comincio a controllare se e' arrivato tutto il contenuto solo dopo aver ricevuto l'ultimo chunk
				if (CONET_MULTI_HOP_DEBUG) {
					fprintf(stderr, "[conet.c:%d] ce->final_chunk_number %d\n",__LINE__,ce->final_chunk_number );
					fprintf(stderr, "[conet.c:%d] ce->chunk_counter %d\n",__LINE__,ce->chunk_counter );
				}

				//rimuovo la nid ma prima controllo che non ci siano altri chunk da finire
				if(ce->final_chunk_number == (ce->chunk_counter-1)) {
					hashtb_start(h->conetht, e);
					r = 0;
					r = hashtb_seek(e, nid, strlen(nid), 0);
					if (r == HT_OLD_ENTRY) {
						hashtb_delete(e);
						if (CONET_MULTI_HOP_DEBUG)
						fprintf(stderr, "[conet.c:%d] rimuovo nid \n",__LINE__);
					}
					hashtb_end(e);
				}

			}
		}
	}

	//spedisco il messaggio al client
	struct sockaddr_in next_hop_in;
	struct sockaddr* to_addr;
	socklen_t addr_size;
	int sock =0;
	int sent =0;

	if (is_raw) {
		if (raw_tocli_sock == 0) {
			if ((sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
				perror("[CONET]: Failed to create socket");
				return -1;
			}
			raw_tocli_sock = sock;
		} else {
			sock = raw_tocli_sock;
		}
	} else {
		if (fwd_sock == 0) {
			if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
				perror("[CONET]: Failed to create socket");
				return -1;
			}
			fwd_sock = sock;
		} else {
			sock = fwd_sock;
		}
	}
	memset(&next_hop_in, 0, sizeof(next_hop_in)); /* Clear struct */
	next_hop_in.sin_family = AF_INET; /* Internet/IP */
	//	next_hop_in.sin_addr.s_addr = ce->c_addr; /* IP address */
	next_hop_in.sin_port = htons(atoi(CONET_DEFAULT_UNICAST_PORT)); /* server port */
	/*addr_size = sizeof(next_hop_in);
	 to_addr=(struct sockaddr*) &next_hop_in;*/

	//devo cercare nella s_pit inviare a tutti i client ed eliminare l'enty nella s_pit

	struct s_pit_entry *spe;
	char *nome = calloc(S_PIT_ENTRY_NOME_SIZE, sizeof(char));
	sprintf(nome, "%s/%llu/%d", nid,csn, seg_index);
	if (CONET_MULTI_HOP_DEBUG)
	fprintf(stderr, "[conet.c:%d]: Received  data for %s/ left edge:%llu, right edge:%llu \n",
			__LINE__, nome, l_edge, r_edge);
	int res = 0;

	hashtb_start(ch->s_pit, e);
	res = hashtb_seek(e, &seg_index, sizeof(int), 0);
	if (res < 0) {
		perror("[ccnd.c]: seek error in ch->s_pit\n");
		exit(-1);
	}
	spe = e->data;
	hashtb_delete(e);
	hashtb_end(e);

	if (res == HT_NEW_ENTRY) {
		if (1)
		fprintf(stderr, "[conet.c:%d]: STRANO \n", __LINE__);
		fprintf(stderr,"[conet.c:%d]: filtro  data for %s left edge:%llu, right edge:%llu \n",
				__LINE__, nome, l_edge, r_edge);
	} else if (res == HT_OLD_ENTRY) { // invio a tutti e poi rimuovo

		struct addr_entry *addre;
		hashtb_start(spe->c_addr, e);
		for (; e->data != NULL; hashtb_next(e)) {
			addre= e->data;
			next_hop_in.sin_addr.s_addr = addre->clia_addr;
			if (CONET_MULTI_HOP_DEBUG)
			fprintf(
					stderr,
					"[conet.c:%d]: sending  data for %s left edge:%llu, right edge:%llu  at %d\n",
					__LINE__, nome, l_edge, r_edge, addre->clia_addr);

			if (is_raw) {
//				int i;
				int prev = 14 + 0 + 20 + 1; //whr
				change_eth_dst_src(&next_hop_in, readbuf - prev);
				to_addr = setup_raw_sockaddr(&next_hop_in);
				addr_size = 20; //solo ipv4 adattare anche ad ipv6
				sent = sendto(sock, readbuf - prev, msg_size, 0, to_addr,
						addr_size);
			} else {
				to_addr = (struct sockaddr*) &next_hop_in;
				addr_size = sizeof(next_hop_in);
				sent = sendto(sock, readbuf, msg_size, 0, to_addr, addr_size);
			}
			//hashtb_delete(e);
		}
		hashtb_end(e);
		//		hashtb_destroy(&spe->c_addr);
	}
	//close(sock);
	return sent;

}
#endif
//TODO: passing ch as a value (and not as a reference) could lead to poor performance
int calculate_seg_index(unsigned long long l_edge, unsigned long long r_edge,
		unsigned int m_segment_size, unsigned int starting_segment_size,
		unsigned int bytes_not_sent_on_starting) {
	/*
	 fprintf(stderr,"[conet.c:%d] calculate_seg_index: l_edge=%llu, r_edge=%llu, m_segment_size=%u, starting_segment_size=%u, bytes_not_sent_on_starting=%u\n",
	 __LINE__, l_edge, r_edge, m_segment_size, starting_segment_size, bytes_not_sent_on_starting);
	 */

	int seg_index = -1; //seg_index will be returned
	if (m_segment_size == starting_segment_size) {
		seg_index = (l_edge + 1) / m_segment_size;
	} else if (m_segment_size > starting_segment_size) {
		seg_index = (l_edge + 1 + bytes_not_sent_on_starting) / m_segment_size;
	} else {
		seg_index = (int) (l_edge + 1
				- (starting_segment_size - m_segment_size))
				/ (int) m_segment_size;

		if (seg_index < 0) {
			seg_index = (r_edge - 10) / m_segment_size;
		}
	}
	if (CONET_DEBUG >= 2)
		fprintf(
				stderr,
				"[conet.c: %d] Inside calculate_seg_index: ch.m_segment_size=%d, ch.starting_segment_size=%d, ch.bytes_not_sent_on_starting=%d, seg_index=%d\n",
				__LINE__, m_segment_size, starting_segment_size,
				bytes_not_sent_on_starting, seg_index);

	return seg_index;
}
