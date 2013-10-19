#include <listener.h> //for CLIENT_IP_ADDR, SERVER_IP_ADDR
#include <stdio.h>
#include <ccn/hashtb.h>
#include <conet/conet.h> 			//for CONET_DEFAULT_SERVER_ADDR, conet_send_data_cp(...)
#include <conet/conet_timer.h>
#include <utilities.h>		//for escape_nid(...), from_byte_array_to_number(...)
#include <conet/info_passing.h> 	//for cp_descriptor_t, INTEREST_DESCRIPTOR, DATA_DESCRIPTOR
#include <cacheEngine.h> //for handleChunk
#include <fcntl.h>

//This will be filled when conet_process_data(...), (called from inside conet_process_input(...) ) will be called
cp_descriptor_t* cp_descriptor; //cp_descriptor_t defined in info_passing.h

char* local_ip; //This will be filled by setup_local_addresses;
struct hashtb * conetht;
struct ccnd_handle h;

struct conet_addr {
	unsigned char from_mac[6];
	unsigned char to_mac[6];
	unsigned char from_ip[4];
	unsigned char to_ip[4];
};

//#define CONET_KERNEL
#define FILE_NAME "/root/alien-ofelia-conet-ccnx/cache_server/files/chunks"
// #define SOURCE_MAC_ADDR {0x02, 0x03, 0x00, 0x00, 0x02, 0x44}//my mac 02:03:00:00:02:44
#define URI_LEN 255

#define LOCAL_MAC_ADDR  { 0x02, 0x03, 0x00, 0x00, 0x02, 0x44 }
// #define TO_MAC_ADDR { 0x02, 0x03, 0x00, 0x00, 0x00, 0xb2 }
#define LOCAL_IP_ADDR {0xc0, 0xa8, 0x01, 0xDA}

// #define DEBUG
#ifdef DEBUG
	#define debug_print(fmt, args...) fprintf(stderr, fmt, ## args)
#else
	#define debug_print(fmt, args...) /* not debugging: nothing */
#endif


static inline int get_file_size(char *filename) {
	int f;
	int size;
	f = open(filename, O_RDONLY);
		if (!f) {
			return -1;
		}

	size= lseek(f, 0, SEEK_END); // seek to end of file
	close(f);
	return size;
}

static inline int read_cp_from_file(char *filename, char *buff, int n_bytes,
		int start_pos) {
	int f;
	f = open(filename, O_RDONLY);
	if (!f) {
	        debug_print("a0a0a0a0 qga");

		return -1;
	}
//	if (!lseek(f, start_pos, SEEK_SET)) {
//	        debug_print("a0a0a0a0 letti ");
//		return -1;
//	}
	lseek(f, start_pos, SEEK_SET);

	int bytes_read = read(f, buff, n_bytes);
	close(f);
	return bytes_read;
}

static inline char* cache_obtainNID(unsigned char** buf_iterator,
		unsigned short ll_flag) {
	unsigned short nid_length;
	char* nid = NULL;

	if (ll_flag == CONET_CIU_DEFAULT_NID_LENGTH_FLAG) {
		nid_length = CONET_DEFAULT_NID_LENGTH;
		nid = malloc((nid_length + 1) * sizeof(char));
		nid = strncpy(nid, (const char*) (*buf_iterator), nid_length);
		nid[nid_length] = '\0';
	} else if (ll_flag == CONET_CIU_VARIABLE_NID_LENGTH_FLAG) {
		nid_length = (unsigned short) ((*buf_iterator)[0]);
		nid = malloc((nid_length + 1) * sizeof(char));
		nid = strncpy(nid, (const char*) (*buf_iterator + 1), nid_length);
		nid[nid_length] = '\0';
	} else {
		//TODO this case will never happen. It's controlled by the caller
		debug_print(
				"[Cache_Server]: Ritorno null questo caso non e' ancora gestito \n");
		return NULL;
	}

	return nid;
}

static inline unsigned long long cache_read_variable_len_number(
		unsigned char* buf, int* pos) {
	unsigned long long number = 0;
	unsigned char* byte_scroller = buf + *pos;

	if ((*byte_scroller | 127) == 127) { //number length: 1byte (1bit pattern) |0XXXXXXX|
		number = (unsigned long long) (*byte_scroller); //the pattern was 0XXXXXXX so we don't worry about the sign
		*pos += 1;
	} else if ((*byte_scroller & 192) == 128) { //number length: 2bytes (2bits pattern) |10XXXXXX|XXXXXXXX|
		unsigned char value[2] = { byte_scroller[0] & 127, byte_scroller[1] }; //return the value in network byte order without the pattern byte(2-bit pattern plus remaining 6-bit effective value) in first position
		number = (unsigned long long) ntohs(*((unsigned short*) value));
		*pos += 2;
	} else if ((*byte_scroller & 224) == 192) { //number length: 3bytes (3bits pattern) |110XXXXX|XXXXXXXX|XXXXXXXX|
		unsigned int ret = 0; //clean memory
		unsigned char* ret_scroller = (unsigned char*) (&ret);
		ret_scroller[1] = byte_scroller[0] & 31;
		ret_scroller[2] = byte_scroller[1];
		ret_scroller[3] = byte_scroller[2];
		number = (unsigned long long) ntohl(ret);
		*pos += 3;
	} else if ((*byte_scroller & 240) == 224) { //number length: 4byte (4bit pattern) |1110XXXX|XXXXXXXX|XXXXXXXX|XXXXXXXX|
		//return the value in network byte order with the pattern byte(3-bit pattern plus remaining 5-bit effective value) in first position
		unsigned int ret = *((unsigned int*) byte_scroller);
		unsigned char* ret_scroller = (unsigned char*) (&ret);
		//return the network byte order but without the 4 initial bit pattern
		ret_scroller[0] = byte_scroller[0] & 15;
		number = (unsigned long long) ntohl(ret);
		*pos += 4;
	} else if ((*byte_scroller & 255) == 240) { //number length: 5bytes (8bits pattern) |11110000|XXXXXXXX|XXXXXXXX|XXXXXXXX|
		//TODO
		*pos += 5;
		debug_print(
				"[Cache_Server]: Ritorno -1 size not yet supported for this len number \n");
		return -1;
	} else if ((*byte_scroller & 255) == 241) {//number length: 6bytes (8bits pattern) |11110001|XXXXXXXX|XXXXXXXX|XXXXXXXX|
		//TODO
		*pos += 6;
		debug_print(
				"[Cache_Server]: Ritorno -1 size not yet supported for this len number\n");
		return -1;
	}

	return number;
}

// qui devo togliere le realloc alloco il pachetto una volta per tutte

static inline unsigned int cache_write_variable_len_number(unsigned char** buf,
		size_t bufsize, unsigned long long number) {
	unsigned char *packet = *buf;

	if (number < 128) {
		//take only the byte of chunk_number setted to a value different from 0: htons((unsigned short)chunk_number)= |0xxxxxxx|00000000|
		packet[bufsize] = (unsigned char) number;
		bufsize += 1;
	} else if (number < 16384) { //2^14
		//TODO taking this value back in the process_input use a short variable!
		unsigned short* num = (unsigned short*) (packet + bufsize);
		bufsize += 2;
		*num = htons((unsigned short) (number | 32768)); //10XXXXXX XXXXXXXX pattern
	} else if (number < 2097152) { // 2^21
		unsigned int num = htonl((unsigned int) (number | 12582912)); //110XXXXX XXXXXXXX XXXXXXXX pattern
		char* num_iterator = (char*) &num;
		//position 0 in csn is the most significant byte and it is 0 for the number in use here
		packet[bufsize] = num_iterator[1];
		packet[bufsize + 1] = num_iterator[2];
		packet[bufsize + 2] = num_iterator[3];
		bufsize += 3;
	} else if (number < 268435456U) {
		unsigned int* num = (unsigned int*) packet + bufsize;
		*num = htonl((unsigned int) (number | 3758096384U)); // 1110XXXX XXXXXXXX XXXXXXXX XXXXXXXX pattern
		bufsize += 4;
	} else if (number < 2147483648U) { // 32 bit field   11110000 XXXXXXXX XXXXXXXX XXXXXXXX pattern
		//TODO
		debug_print(
				" Ritorno -1 size not yet supported for this len number \n");
		return -1;
	} else if (number < 549755813888U) { // 40 bit field   11110001 XXXXXXXX XXXXXXXX XXXXXXXX pattern
		//TODO
		debug_print(
				" Ritorno -1 size not yet supported for this len number \n");
		return -1;
	}

	*buf = packet;
	return bufsize;
}

static inline unsigned int cache_build_carrier_packet(unsigned char** buffer,
		int buffer_pos, int ciu_type, int cache, const char * nid,
		unsigned long long chunk_number, unsigned int left_edge,
		unsigned int right_edge, unsigned short cp_flags, int chunk_size) {
	unsigned char* packet = *buffer;
	int packet_size = buffer_pos;

	if (ciu_type == CONET_NAMED_DATA_CIU_TYPE_FLAG) {
		packet[packet_size] = (unsigned char) cp_flags;
		packet_size++;
	} else {
		packet[packet_size] = CONET_PREFETCH << 4;
		if (chunk_size == 0) {
			packet[packet_size] |= CONET_ASK_CHUNK_INFO_FLAG;
		}
		packet_size++;
	}

	packet_size = cache_write_variable_len_number(&packet, packet_size,
			left_edge);
	packet_size = cache_write_variable_len_number(&packet, packet_size,
			right_edge);

	if (ciu_type == CONET_NAMED_DATA_CIU_TYPE_FLAG && (cp_flags >> 4)
			== CONET_FOLLOW_VAR_CHUNK_SIZE) {
		packet_size = cache_write_variable_len_number(&packet, packet_size,
				(unsigned long long) chunk_size);
	}

	*buffer = packet;
	return packet_size;
}

//merda nuova

unsigned char* cache_setup_ipeth_headers(struct conet_addr *c_addr,
		unsigned char * packet, int * packet_size, unsigned int ipoption_size) {

	unsigned char * buffer;
	struct ethhdr *eh;
	struct iphdr *iphdr;
	unsigned int ethhdr_size = 14;
	unsigned short int vid = htons(CONET_VLAN_ID), vlanproto;
	// unsigned int vlan_size = (TAG_VLAN)?4:0;
//	unsigned int headers_size = ethhdr_size + vlan_size + sizeof(struct iphdr);
	unsigned int headers_size = ethhdr_size + sizeof(struct iphdr);

	unsigned int payload_size = *packet_size; //we call it payload size, but it includes the CONET ip option


	// (*packet_size) = ethhdr_size/*ETH 14B*/ + vlan_size  + sizeof (struct iphdr)/*ip (20B)*/ + *packet_size/*including the IP option*/;
	(*packet_size) = ethhdr_size/*ETH 14B*/+ sizeof(struct iphdr)
			/*ip (20B)*/+ *packet_size/*including the IP option*/;

	if (*packet_size < MIN_PACK_LEN) {
		*packet_size = MIN_PACK_LEN;
	}
	buffer = malloc((*packet_size) * sizeof(char));
	if (buffer == NULL) {
		debug_print( "[Cache_Server]: packet_size malloc fallita");
		return NULL;
	}

	//struct sockaddr_ll sock_addr;
	eh = (struct ethhdr *) buffer;
	// iphdr = (struct iphdr *)(buffer + ethhdr_size +vlan_size);
	iphdr = (struct iphdr *) (buffer + ethhdr_size);
	memcpy((void *) buffer, (void *) c_addr->to_mac, 6);
	memcpy((void *) (buffer + 6), (void *) c_addr->from_mac, 6);

	// if (TAG_VLAN){ //this should not be used anymore...
	// 	eh->h_proto = htons(0x8100);
	// 	vlanproto = htons(0x0800);
	// 	memcpy((void *) (buffer + 14), (void *) &vid, 2);
	// 	memcpy((void *) (buffer + 16), (void *) &vlanproto, 2);
	// }else {
	eh->h_proto = htons(0x0800);
	// }

	//start of ip header
	//struct iphdr *ip = (struct iphdr *) buffer+ethhdr_size;

	iphdr->ihl = 5 + ipoption_size / 4;
	iphdr->version = 4;
	iphdr->tos = 16;
	iphdr->tot_len = htons(sizeof(struct iphdr) + payload_size);
	iphdr->id = htons(52407);
	iphdr->frag_off = 0;
	iphdr->ttl = 65;
	iphdr->protocol = CONET_PROTOCOL_NUMBER;
	iphdr->check = 0;
	// inet_pton(SERVER_ADDR, (unsigned char *)&(iphdr->saddr));
	memcpy((unsigned char *) &(iphdr->saddr), c_addr->from_ip, 4);

	// inet_pton(CLI_ADDR, (unsigned char *)&(iphdr->daddr));
	memcpy((unsigned char *) &(iphdr->daddr), c_addr->to_ip, 4);
	memcpy(buffer + headers_size, packet, payload_size);

//	iphdr->check = csum((unsigned short int *) (buffer + ethhdr_size
//			+ vlan_size), iphdr->ihl << 1);
	 iphdr->check = csum((unsigned short int *) (buffer + ethhdr_size), iphdr->ihl << 1);
	free(packet);
	return buffer;

}

unsigned int cache_build_ip_option(unsigned char** buffer, int ciu_type, int cache,
		const char * nid, unsigned long long chunk_number) {

	unsigned char* packet = *buffer;
	unsigned char* hdrptr = packet;
	int nid_len = strlen(nid);
	int ll_flag = 0;
	int packet_size = 0;
	int toalloc = 0;
	int i = 0;

	if (nid_len != 16) {
		ll_flag = 2;
	}

	packet[0] = CONET_IP_OPTION_CODE;

	packet[1] = 0; //ip option len updated later
	packet[2] = 0;
	packet[2] = (unsigned char) ciu_type << 4 | (unsigned char) ll_flag << 2
			| (unsigned char) cache << 1;//the cast to (unsigned char) is used to support both big and little endian (NB:the shift operation also reflects the endianess)
	packet[3] = 0;
	packet_size += 4;

	debug_print( "[Cache_Server]: ipv4 flags: %d\n", packet[2] & 0xFF);
	if (ll_flag == 0) {
		toalloc = strlen(nid);
		// packet = realloc(packet, packet_size + toalloc);
		packet_size += toalloc;
		hdrptr = packet + 4;
	} else {
		toalloc = strlen(nid) + 1;
		// packet = realloc(packet, packet_size + toalloc);
		packet_size += toalloc;
		hdrptr = packet + 4;
		hdrptr[0] = (unsigned char) (strlen(nid));
		hdrptr++;
	}
	memcpy(hdrptr, nid, strlen(nid));

	//debug_print( "[CONET]: nid: %s\n", hdrptr);

	hdrptr += strlen(nid);

	packet_size = cache_write_variable_len_number(&packet, packet_size,
			chunk_number);

	//debug_print( "[CONET]: left_edge: %d\n", left_edge);
	//debug_print( "[CONET]: right_edge: %d\n", right_edge);

	//cp flag
	if (packet_size % 4 != 0) {
		// packet = realloc(packet, packet_size + (4 - packet_size % 4));

		for (i = 0; i < (4 - packet_size % 4); i++) {
			packet[packet_size + i] = 0x00;
		}
		packet_size = packet_size + (4 - packet_size % 4);
	}
	packet[1] = (unsigned char) packet_size;

	*buffer = packet;
	return packet_size;
}
//

static inline char* conet_create_data_cp(struct conet_addr* c_addr,
		int chunk_size, char * segment, unsigned int segment_size,
		const char* nid, unsigned long long csn, unsigned int l_edge,
		unsigned int r_edge, unsigned int is_final,
		unsigned int add_chunk_info, int *cp_size, char *recycled_tag) {
	unsigned char* packet = NULL;
	unsigned char cp_flag = CONET_CONTINUATION << 4;
	unsigned int pckt_size;
	unsigned int ipoption_size;
	// size_t packet_size;

	/*setting-up flags*/
	if (add_chunk_info == 1) {
		//first data-CP response
		//TODO vedere se potenza di 2->codificare con enum altrimenti segue...
		cp_flag = CONET_FOLLOW_VAR_CHUNK_SIZE << 4;
	}
	if (is_final == 1) {
		cp_flag = cp_flag | CONET_FINAL_SEGMENT_FLAG;
	}

	packet = malloc(CONET_DEFAULT_MTU * sizeof(char)); //alloco diretto tutto lo spazio per il pacchetto
	if (packet == NULL) {
		debug_print( "[Cache_Server]: malloc fallita");
		return NULL;
	}

	// #ifndef CONET_TRANSPORT
	pckt_size = cache_build_ip_option(&packet, CONET_NAMED_DATA_CIU_TYPE_FLAG,
			CONET_CIU_CACHE_FLAG, nid, csn);
	// #endif
	// *ip_option_size = pckt_size;
	ipoption_size = pckt_size;

	debug_print( "[Cache_Server]: build_ip_option pckt_size %d bytes", pckt_size);


	// #ifdef IPOPT
	// ricliclare tag!!!!!!!!!!!!11
	packet[pckt_size] = recycled_tag[0];
	packet[pckt_size + 1] = recycled_tag[1];
	packet[pckt_size + 2] = recycled_tag[2];
	packet[pckt_size + 3] = recycled_tag[3];
	pckt_size += 4;
	// #endif

	pckt_size = cache_build_carrier_packet(&packet, pckt_size,
			CONET_NAMED_DATA_CIU_TYPE_FLAG, CONET_CIU_CACHE_FLAG, nid, csn,
			l_edge, r_edge, cp_flag, chunk_size);

	debug_print( "[Cache_Server]: build_carrier_packet pckt_size %d bytes",
			pckt_size);

	if (pckt_size + segment_size > CONET_DEFAULT_MTU) {
		int diff = (pckt_size + segment_size) - CONET_DEFAULT_MTU;
		r_edge = r_edge - diff;
		segment_size = r_edge - l_edge + 1;
		// if (packet != NULL) {
		// 	free(packet);
		// }
		packet = NULL;

		//ricostruisco il pacchetto se non entro dentro l'MTU togliere questo caso
		debug_print(
				"[Cache_Server]: ricostruisco il pacchetto se non entro dentro l'MTU");

		// #ifndef CONET_TRANSPORT
		pckt_size = cache_build_ip_option(&packet, CONET_NAMED_DATA_CIU_TYPE_FLAG,
				CONET_CIU_CACHE_FLAG, nid, csn);
		// #endif
		// #ifdef IPOPT
		// ricliclare tag!!!!!!!!!!!!11
		packet[pckt_size + 1] = recycled_tag[0];
		packet[pckt_size + 2] = recycled_tag[1];
		packet[pckt_size + 3] = recycled_tag[2];
		packet[pckt_size + 4] = recycled_tag[3];
		pckt_size += 4;
		// #endif
		pckt_size = cache_build_carrier_packet(&packet, pckt_size,
				CONET_NAMED_DATA_CIU_TYPE_FLAG, CONET_CIU_CACHE_FLAG, nid, csn,
				l_edge, r_edge, cp_flag, chunk_size);
	}

	memcpy(packet + pckt_size, segment, segment_size);

	// debug_print("[Cache_Server]: build_ip_option + segment pckt_size %d bytes", segment_size+pckt_size);
	//       KDEBUG_HEX(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1, packet, segment_size+pckt_size,  true);

	*cp_size = (pckt_size + segment_size);
	debug_print( "[Cache_Server]: data cp size %d ip_option_size %d\n",
			*cp_size, ipoption_size);

	packet =cache_setup_ipeth_headers(c_addr, packet, cp_size, ipoption_size);
	debug_print( "[Cache_Server]: setup_ipeth_headers pckt_size %d bytes",
			*cp_size);
	//       KDEBUG_HEX(KERN_INFO, "", DUMP_PREFIX_OFFSET, 16, 1, packet, *cp_size,  true);

	debug_print("[Cache_Server]: Created data cp for %s/%llu left edge:%u, right edge:%u \n",
			nid, csn, l_edge, r_edge);
	free(segment);
	return packet; //ritorno il pacchetto creato
}

inline char* cache_conet_process_interest_cp(struct conet_addr *c_addr,
		unsigned char* readbuf, int *conet_payload_size, unsigned short ll,
		unsigned short c, unsigned short ask_info) {
	char* nid;
	unsigned short nid_length;
	unsigned char* iterator;
	unsigned int is_final = 0;
	int pos;
	unsigned long long csn;
	unsigned char cp_flag;
	unsigned int l_edge;
	unsigned int r_edge;
	char *cp = NULL;
	char *uri;
	char *filename;
	char *conet_payload = NULL;
	int segment_size;
	int cp_size = 0;
	int chunk_size;
	unsigned char recycled_tag[4];
	int i;

	if (c == CONET_CIU_NOCACHE_FLAG) {
		//simply forward the interest without further analyze
		return NULL;
	}

	//iterator on the first byte of NID field
	// #ifndef CONET_TRANSPORT

	iterator = readbuf + 1 + 2; //ip_opt len +flag + DS&T field
	// #else
	// iterator = readbuf + 1;
	// #endif
	nid = cache_obtainNID(&iterator, ll);
	debug_print( "[Cache_Server]: NID: %s \n", nid);
	nid_length = strlen(nid);

	if (ll == 0) {
		//iterator on the first byte of CSN field
		iterator = iterator + nid_length;
	} else if (ll == 2) {
		//iterator on the first byte of CSN field
		iterator = iterator + 1 + nid_length;
	} else {
		debug_print(
				"[Cache_Server]: Using reserved ll-flag in input message. Not yet supported\n");
		free(nid);
		return NULL;
	}

	pos = iterator - readbuf;
	csn = cache_read_variable_len_number(readbuf, &pos);

	// debug_print("[Cache_Server]:After reading csn=%llu pos=%d and next bytes are %2X %2X %2X %2X\n", csn, pos, readbuf[pos + 0], readbuf[pos + 1], readbuf[pos+ 2], readbuf[pos + 3]);
	// #ifdef CONET_TRANSPORT
	//Skip the "End of ip option list" character
	pos = (int) readbuf[0] - 1; // - ip_opt len
	// #endif
	//#ifndef CONET_TRANSPORT
	// #ifdef IPOPT
	pos += 4; //jump 4 byte openflow tag
	recycled_tag[0] = readbuf[pos - 4];
	recycled_tag[1] = readbuf[pos - 3];
	recycled_tag[2] = readbuf[pos - 2];
	recycled_tag[3] = readbuf[pos - 1];
	debug_print(
			"[Cache_Server]: Received an interest for tag %02X %02X %02X %02X \n",
			recycled_tag[0], recycled_tag[1], recycled_tag[2], recycled_tag[3]);

	// #endif
	// #endif
	cp_flag = (unsigned char) *(readbuf + pos);
	pos++;

	if ((cp_flag & CONET_ASK_CHUNK_INFO_FLAG) != 0) {
		ask_info = 1;
	}

	l_edge = cache_read_variable_len_number(readbuf, &pos);
	r_edge = cache_read_variable_len_number(readbuf, &pos);

	debug_print("[Cache_Server]: Received an interest for %s/%llu left edge:%u, right edge:%u \n",
			nid, csn, l_edge, r_edge);

	uri = malloc(URI_LEN * sizeof(char));
	if (uri == NULL) {
		debug_print( "[Cache_Server]: malloc fallita");
		goto end;
	}

	sprintf(uri, "%s/%llu", nid, csn);

	for (i = 0; i < nid_length; i++) {
		if (uri[i] == '/') {
			uri[i] = '_';
		}
	}

	//	sprintf(uri, "%s/%llu", nid, csn);

	filename = malloc(URI_LEN * sizeof(char));
	if (filename == NULL) {
		debug_print( "[Cache_Server]: malloc fallita");
		goto end;
	}
	//convertire la nid in un nomefile per adesso si risponde sempre con lo stesso file
	sprintf(filename, "%s/%s.chu", FILE_NAME, uri);
	//	sprintf(filename, "%s_%llu", FILE_NAME2, csn);
	debug_print( "[Cache_Server]: filenem %s\n", filename);
	// da qui in poi becco il contenutto
	chunk_size = get_file_size(filename);
	if (chunk_size == -1 || chunk_size == 0) {
		debug_print( "[Cache_Server]:file not found. Drop\n");
		goto end;
	}

	//leggo il payload del cp dal file
	segment_size = r_edge - l_edge + 1;
	conet_payload = malloc((segment_size + 1) * sizeof(char));
	if (conet_payload == NULL) {
		debug_print( "[Cache_Server]: malloc fallita");
		goto end;
	}
//	*conet_payload_size =readSegmentFromFileSystem(nid,csn,l_edge,segment_size,conet_payload);

	*conet_payload_size = read_cp_from_file(filename, conet_payload,
			segment_size, l_edge);

	if (*conet_payload_size == -1 || *conet_payload_size == 0) {
		debug_print( "[Cache_Server]:file not found. Drop\n");
		goto end;
	}

	debug_print( "[Cache_Server]: bytes letti %d dal file %s \n",
			*conet_payload_size, filename);
	debug_print( "[Cache_Server]: l_edge %u segment_size %d\n", l_edge,
			segment_size);

	if (*conet_payload_size < segment_size) {
		//end of chunk with a dimension different from a multiple of segment-dimension reached
		if (*conet_payload_size == 0) {
			r_edge = 0;
		} else {
			debug_print( "[Cache_Server]: The segment is final\n");
			r_edge = *conet_payload_size + l_edge - 1;
			is_final = 1;
		}
		segment_size = r_edge - l_edge + 1;
	}

	// if (*conet_payload_size - (int) l_edge < 0) {
	// 	debug_print("[Cache_Server]: The segment is shorter than this request. Drop\n");
	// 	return NULL;
	// }

	cp = conet_create_data_cp(c_addr, chunk_size, conet_payload,
			*conet_payload_size, nid, csn, l_edge, r_edge, is_final, ask_info,
			&cp_size, recycled_tag);
	*conet_payload_size = cp_size;
	free(filename);
	end: free(uri);
	free(nid);
	return cp;
}

int cache_process_data_cp(unsigned char* readbuf, unsigned short ll,
		unsigned short c, int msg_size) {

	char* nid;
	unsigned short nid_length;
	unsigned char* iterator;
	//	int ret = 0;
	unsigned short is_final = 0;
	//	unsigned int retrasmit_start = 0;
	struct conet_entry* ce = NULL;
	unsigned char tag[4];
	iterator = readbuf + 1 + 2; //ip_opt len +flag + DS&T field

	nid = cache_obtainNID(&iterator, ll);
	nid_length = strlen(nid);

	if (ll == 0)
		iterator = iterator + nid_length;//iterator on the first byte of CSN field
	else if (ll == 2)
		iterator = iterator + 1 + nid_length;//iterator on the first byte of CSN field
	else {
		if (CONET_DEBUG >= 2)
			debug_print( "[Cache_Server] ll=%d this is an unsupported flag\n", ll);
		abort();
	}
	int pos = iterator - readbuf; //pos is the position of the field after NID in byte array readbuf containing the data-cp

	// primo pezzo in cui devo reperire la conet entry e un po di info dal pacchetto

	struct hashtb_enumerator ee;
	struct hashtb_enumerator *e = &ee;
	int res = 0;
	hashtb_start(conetht, e);
	res = hashtb_seek(e, nid, nid_length, 0);
	//	ce = hashtb_lookup(conetht, nid, nid_length);
	ce = e->data;
	if (res == HT_NEW_ENTRY) {
		debug_print( "[Cache_Server] nuova entry per %s\n", nid);
		ce->nid = nid;
		//		ce->chunk_expected = NULL;
		//		ce->last_processed_chunk = 0;
		ce->chunk_counter = 0;
		ce->final_chunk_number = -1;
		//		ce->send_fd = -1;
		ce->chunks = hashtb_create(sizeof(struct chunk_entry), NULL);

	}
	hashtb_end(e);

	unsigned long long csn = cache_read_variable_len_number(readbuf, &pos);

	//	ce->last_processed_chunk = csn;

	pos = (int) readbuf[0] - 1; // - ip_opt len

	pos += 4; //jump 4 byte openflow flag

	tag[0] = readbuf[pos - 4];
	tag[1] = readbuf[pos - 3];
	tag[2] = readbuf[pos - 2];
	tag[3] = readbuf[pos - 1];

	unsigned char cp_flag = (unsigned char) *(readbuf + pos);
	pos++;

	unsigned long long l_edge = cache_read_variable_len_number(readbuf, &pos);
	unsigned long long r_edge = cache_read_variable_len_number(readbuf, &pos);

	if ((cp_flag & CONET_FINAL_SEGMENT_FLAG) != 0) {
		debug_print(
				" conet_process_data_cp(..)]: This is a final segment (cp_flag=%d)\n",
				cp_flag);
		is_final = 1;
	} else {
		debug_print(
				" conet_process_data_cp(..)]: This is NOT a final segment (cp_flag=%d)\n",
				cp_flag);
	}
	debug_print( "[Cache_Server] processing data cp for %s/%d left:%d, right:%d \n", nid,
			(unsigned int) csn, (unsigned int) l_edge, (unsigned int) r_edge);

	//secondo pezzo qui me la vedo col chunk e capisco se l'ho completato se si lo mando a ccnx oppurebooooooooo

	struct chunk_entry* ch;
	hashtb_start(ce->chunks, e);
	res = hashtb_seek(e, &csn, sizeof(int), 0);
	//	ch = hashtb_lookup(ce->chunks, &csn, sizeof(int));
	ch = e->data;
	if (res == HT_NEW_ENTRY) {
		debug_print( "[Cache_Server] Chunk %u visto per la prima volta \n",
				(unsigned int) csn);
		ch->chunk_number = csn;
		ch->m_segment_size = get_segment_size(nid, csn, ce->chunk_size,
				CONET_DEFAULT_MTU);
		ch->starting_segment_size = ch->m_segment_size;
		ch->bytes_not_sent_on_starting = 0;
		ch->current_size = 0;
		ch->inviato = 0;
		ch->bitmap_size = 4; //per adesso e' harcodato whr

	}
	hashtb_end(e);

	unsigned char chsz_flag = cp_flag >> 4; //first 4 bits of cp_flag contain the chunk size
	if ((unsigned short) (chsz_flag) != CONET_CONTINUATION) {
		if ((unsigned short) (chsz_flag) == CONET_FOLLOW_VAR_CHUNK_SIZE) {
			ce->chunk_size = cache_read_variable_len_number(readbuf, &pos);
		} else {
			ce->chunk_size = 1 << (9 + (unsigned short) chsz_flag); //TODO check (not so useful in ccnx context)
		}

		debug_print( "[Cache_Server] Chunk size is: %llu\n", ce->chunk_size);

		ch->starting_segment_size = r_edge - l_edge + 1;

		if (ch->starting_segment_size < ce->chunk_size) {
			//this is the chunk size less the received bytes in the first data-cp
			unsigned long long reduced_chunk_size = ce->chunk_size
					- ch->starting_segment_size;
			//get the segment size requestable in the next interest extimating variable fields like left and right (in the worst case adapted to received chunk size)
			debug_print( "[Cache_Server] m_segment_size was %u\n", ch->m_segment_size);
			ch->m_segment_size = get_segment_size(nid, csn, reduced_chunk_size,
					CONET_DEFAULT_MTU);
			debug_print( "[Cache_Server] Now m_segment_size is %u\n", ch->m_segment_size);

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
	if ((ch->cp_bitmap & (1 << seg_index)) == 0) {
		// copio la robba  deontro il chunk
		memcpy(ch->chunk + l_edge, readbuf + pos, seg_size);
	}

	//	struct hashtb_enumerator ee;
	//	struct hashtb_enumerator *e = &ee;
	int r = 0;

	ch->cp_bitmap = ch->cp_bitmap | (1 << seg_index); //segno il cp arrivato

	int is_eof_value = -1;
	if (is_final == 1) { // qui devo controllare anche se sono arrivati i cp precedenti a quello finale

		//ricevo l'ultimo cp non e' detto che il chunk e' completo
		ch->final_segment_received = 1;
		debug_print( "[Cache_Server] chunk %llu is_final!\n", ch->chunk_number);
		is_eof_value = is_eof(&h, ch->chunk, ch->current_size);
		if (is_eof_value) {
			ch->bitmap_size = seg_index + 1;//questo chunk e' piu piccolo
			ce->completed = 1; // e' arrivato l'ultimo chunk
			ce->final_chunk_number = ch->chunk_number;
			debug_print( "[Cache_Server] is_eof=%d \n", is_eof_value);
		}
		ch->arrived = 1;
	}

	//comincio a controllare se e' arrivato tutto il chunk solo dopo aver ricevuto l'ultimo cp
	if (ch->arrived) {
		//creo la maschera di bit da confrontare con quella del cp
		//si puoi ottimizzare molto
		int i;
		int mask = 0;
		for (i = 0; i < ch->bitmap_size; i++) {
			mask = mask | (1 << i);
		}
		if (ch->cp_bitmap == mask) {
			debug_print( "[Cache_Server] arrived ch->chunk_number %llu\n",
					ch->chunk_number);
			if (!ch->inviato) {

				//qui salvo su file
				char *uri = calloc(nid_length + 1, sizeof(char));//per handleChunk
				sprintf(uri, "%s", nid);

				for (i = 0; i < nid_length; i++) {
					if (uri[i] == '/') {
						uri[i] = '_';
					}
				}
				handleChunk(uri, csn, (long) from_byte_array_to_number(tag),
						ch->current_size, (void*) ch->chunk);
				//conet_send_chunk_to_ccn(h, ch, ch->current_size); //invio il chunk a ccnx
				//
				free(uri);

				ch->inviato = 1;
				hashtb_start(ce->chunks, e);
				r = hashtb_seek(e, &(ch->chunk_number), sizeof(int), 0);
				if (r == HT_OLD_ENTRY) {
					//hashtb_delete(e1);//qui non cancello ma libero
					// free(ch->chunk);
					debug_print( "[Cache_Server] liberato il chunk  %llu \n",
							ch->chunk_number);
					ce->chunk_counter++;
				}
				hashtb_end(e);
			} else {
				debug_print( "[Cache_Server] chunk  %llu  gia' inviato,ritorno\n",
						ch->chunk_number);
				return 0;
			}

			if (ce->completed) { //comincio a controllare se e' arrivato tutto il contenuto solo dopo aver ricevuto l'ultimo chunk
				debug_print( "[Cache_Server] ce->final_chunk_number %d\n",
						ce->final_chunk_number);
				debug_print( " [Cache_Server] ce->chunk_counter %d\n", ce->chunk_counter);

				//rimuovo la nid ma prima controllo che non ci siano altri chunk da finire
				if (ce->final_chunk_number == (ce->chunk_counter - 1)) {
					hashtb_start(conetht, e);
					r = 0;
					r = hashtb_seek(e, nid, strlen(nid), 0);
					if (r == HT_OLD_ENTRY) {
						//non dovrei farlo come cache server
						//						hashtb_delete(e);
						//						debug_print( " rimuovo nid \n");
					}
					hashtb_end(e);
				}

			}
		}
	}

}


int main(int argc, char** argv) {

	unsigned char* recvbuf = NULL;
	int raw_sock;
	int raw_send_sock;
	struct conet_addr c_addr;

	// unsigned char to_mac[6] = TO_MAC_ADDR;
	char *conet_payload = NULL;
	int conet_payload_size = 0;

	char* cache_server_ip ;
	char* cache_server_mac ;
	char* controller_ip_address ;
	unsigned short controller_port ;



	char ifname[256];
	char line[256];

	char cache_ip[256];
	char controller_ip[256];
	char cache_mac[256];

	FILE* file;

	unsigned char local_mac[6] = LOCAL_MAC_ADDR;
	unsigned char local_ip[6] = LOCAL_IP_ADDR ;


	if ((file = fopen("./conet.conf", "r")) == NULL) {
		debug_print(
				"Manca il file conet.conf!!!!! utilizzo parametri di default\n");
		conet_ifname = CONET_IFNAME;
		return -1;
	} else {
		while (fgets(line, 256, file) != NULL) {
			char par[256], val[256];

			if (line[0] == '#')
				continue;

			if (sscanf(line, "%s = %s", par, val) != 2 ) {
				//			debug_print( "Syntax error, line %d\n", linenum);
				continue;
			}
			if (strcmp(par, "if_name") == 0){
				sscanf(val, "%s",ifname);
				conet_ifname = ifname;
			}
			if (strcmp(par, "cache_server_ip") == 0){
				sscanf(val, "%s",cache_ip);
				cache_server_ip = cache_ip;
			}
			if (strcmp(par, "controller_ip_address") == 0){
				sscanf(val, "%s",controller_ip);
				controller_ip_address = controller_ip;
			}
			if (strcmp(par, "controller_port") == 0){
				controller_port = atol(val);
			}
			if (strcmp(par, "local_mac") == 0){
				sscanf(val, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &local_mac[0], &local_mac[1], &local_mac[2], &local_mac[3], &local_mac[4], &local_mac[5]);
				sscanf(val, "%s",cache_mac);
				cache_server_mac = cache_mac;
			}
			if (strcmp(par, "local_ip") == 0){
				sscanf(val, "%hhx:%hhx:%hhx:%hhx", &local_ip[0], &local_ip[1], &local_ip[2], &local_ip[3]);
			}
			debug_print("config: %s = %s \n",  par, val);
		}
		fclose(file);
	}






	raw_sock = setup_raw_socket_listener();

	if ((raw_send_sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0)
		return -1;

	cacheEngine_init(cache_server_ip, cache_server_mac, controller_ip_address,
			controller_port);

	//h = malloc(sizeof(struct ccnd_handle));
	h.scratch_indexbuf = NULL ;
	
	conetht = hashtb_create(sizeof(struct conet_entry), NULL);

	while (raw_sock != -1) {

		struct sockaddr addr;
		memset(&addr, 0, sizeof(addr));
		socklen_t addrlen = sizeof(addr);
		int expected_dim = recvfrom(raw_sock, recvbuf, 0, MSG_TRUNC | MSG_PEEK,
				NULL, NULL); //read a packet dimension from the socket without removing data from the queue
		if (expected_dim == 0) {
			perror("[Cache_Server]: Empty UDP packet received.\n");
			continue;
		}
		if (expected_dim < 0) {
			perror("[Cache_Server]: Error in conet_process_input -recvfrom()-");
			continue;
		}
		recvbuf = calloc(expected_dim, sizeof(unsigned char)); //allocate memory for the dimension obtained from the last recvfrom call
		if (recvbuf == NULL) {
			perror("[Cache_Server]: Error in conet_process_input -calloc()-");
			continue;
		}
		int read =
				recvfrom(raw_sock, recvbuf, expected_dim, 0, &addr, &addrlen);//read the whole UDP packet

		if (read == -1) {
			perror("[Cache_Server]: Error in conet_process_input-recvfrom()-");
			goto out;
		}
		char* src_addr;
		src_addr = inet_ntoa(((struct sockaddr_in*) &addr)->sin_addr);

		unsigned short eth_proto = *(unsigned short *) (recvbuf + 12);

		debug_print( "[Cache_Server] ETH PROTO: %04X \n", ntohs(eth_proto));

		unsigned short vlan_oh = 0;
		if (eth_proto == htons(0x8100)) {
			if (CONET_DEBUG >= 2)
				debug_print( "[Cache_Server] ETH PROTO: %04X (VLAN)\n", ntohs(eth_proto));
			vlan_oh = 4;
		}

		if (eth_proto == htons(0x0806)) {
			if (CONET_DEBUG >= 2)
				debug_print( "[Cache_Server] ETH PROTO: %04X (ARP)\n", ntohs(eth_proto));
			if (CONET_DEBUG >= 2)
				debug_print( "[Cache_Server] This is ARP. Drop it.\n");
			goto out;
		}

		if (memcmp(recvbuf + 6, local_mac, 6) == 0) { //source MAC address is local MAC
			if (CONET_DEBUG >= 2)
				debug_print( "[Cache_Server] Source MAC is my local MAC. Drop it.\n");
			goto out;
		}

		if (eth_proto == htons(0x8100)) {
			if (CONET_DEBUG >= 2)
				debug_print( "[Cache_Server] ETH PROTO: %04X (VLAN)\n", ntohs(eth_proto));
			vlan_oh = 4;
			unsigned short eth_proto2 = *(unsigned short*) (recvbuf + 12
					+ vlan_oh);
			if (eth_proto2 == htons(0x0800)) {
				if (CONET_DEBUG >= 2)
					debug_print( "[Cache_Server] ETH PROTO2: %04X (IP)\n",
							ntohs(eth_proto2));
			} else {
				if (CONET_DEBUG >= 2)
					debug_print(
							"[Cache_Server] ETH PROTO2: %04X (NOT IP!!!!). Packet dropped\n",
							ntohs(eth_proto2));
				goto out;
			}
		} else {
			if (eth_proto != htons(0x0800)) {
				if (CONET_DEBUG >= 2)
					debug_print(
							"[Cache_Server] ETH PROTO: %04X (NOT IP!!!!). Packet dropped\n",
							ntohs(eth_proto));
				goto out;
			}
		}

		int ip_src_offset;
		if (eth_proto == htons(0x0800)) {
			ip_src_offset = 14 + vlan_oh + 12;
			struct sockaddr_in src_sockaddr;
			memcpy(&(src_sockaddr.sin_addr), recvbuf + ip_src_offset, 4);
			src_addr = inet_ntoa(src_sockaddr.sin_addr);
		}

		int hdr_offset;
		hdr_offset = 14 + vlan_oh + 20 + 1; //14 byte eth header + 4 byte vlan tag + 20 byte IP header overhead + 1 byte ip option type, now len is procesed in conet_process_data_cp() or conet_process_interest_cp()

		if (recvbuf[hdr_offset - 1] != CONET_IP_OPTION_CODE) {
			goto out;
		}

		debug_print("[Cache_Server] dest mac addr %02X %02X %02X %02X %02X %02X \n", recvbuf[0], recvbuf[1],recvbuf[2],recvbuf[3],recvbuf[4],recvbuf[5]);
		debug_print("[Cache_Server] src mac addr %02X %02X %02X %02X %02X %02X \n", recvbuf[6], recvbuf[7],recvbuf[8],recvbuf[9],recvbuf[10],recvbuf[11]);
		debug_print("[Cache_Server] Source ip addr %02X %02X %02X %02X \n", recvbuf[ip_src_offset], recvbuf[ip_src_offset+1],recvbuf[ip_src_offset+2],recvbuf[ip_src_offset+3]);
		debug_print( "[Cache_Server] Dest ip addr %02X %02X %02X %02X \n", recvbuf[ip_src_offset+4], recvbuf[ip_src_offset+5],recvbuf[ip_src_offset+6],recvbuf[ip_src_offset+7]);


		memcpy(c_addr.to_ip, &recvbuf[ip_src_offset], 4);
		memcpy(c_addr.from_ip, local_ip, 4);
//		memcpy(c_addr.to_mac, to_mac, 6);
		memcpy(c_addr.from_mac,recvbuf, 6);

		unsigned char CIUflags;
		unsigned char pppp;
		unsigned short ll;
		unsigned short c = 1; //default: cache
		unsigned short test_res;

		CIUflags = recvbuf[hdr_offset + 1]; //ok con len

		pppp = CIUflags >> 4;
		if ((test_res = (CIUflags | 243)) == 243) //243 = 11110011, LL='00'
			ll = 0;
		else if (test_res == 251) //251 = 11111011, LL='10'
			ll = 2;
		else {
			if (CONET_DEBUG >= 2)
				debug_print(
						"[Cache_Server]: Using reserved ll-flag in input message. Not yet supported\n");
			//			return; //TODO manage reserved coding
		}
		if ((CIUflags | 253) == 253) //253 = 11111101, c='0' else c='1'
			c = 0;
		if (pppp & CONET_NAMED_DATA_CIU_TYPE_FLAG) {
			//tutto ok ho un dada cp
			//inserire qui whitelist server
			cache_process_data_cp(recvbuf + hdr_offset, ll, c, expected_dim);

		} else if (pppp & CONET_INTEREST_CIU_TYPE_FLAG) {
#ifndef CONET_KERNEL
			struct sockaddr* to_addr;
			struct sockaddr_in to_addr_in;
			memset(&to_addr_in, 0, sizeof(to_addr_in));
			to_addr_in.sin_family = AF_INET;
			to_addr_in.sin_addr.s_addr = inet_addr(src_addr);
			to_addr_in.sin_port = htons(atoi(CONET_DEFAULT_UNICAST_PORT));

			int addr_size = 20;
			int sent=0;
			int if_index = 0;

			get_mac_from_ip(c_addr.to_mac,&to_addr_in ,&if_index);
			debug_print( "[Cache_Server] client ip address: %s\n", inet_ntoa(to_addr_in.sin_addr));
			
			to_addr = setup_raw_sockaddr(&to_addr_in);

			conet_payload = cache_conet_process_interest_cp(&c_addr,
					&recvbuf[hdr_offset], &conet_payload_size, ll, c, 0);
			if (conet_payload != NULL){
				 sent = sendto(raw_send_sock, conet_payload, conet_payload_size, 0, to_addr, addr_size);
		         free(conet_payload);
			}
			debug_print("[Cache_Server] sent %d bytes\n",sent);
#endif
		}
	out:
		free(recvbuf);

	}
	return 0;
}
