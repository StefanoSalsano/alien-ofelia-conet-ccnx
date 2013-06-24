#ifndef CONET_H_
#define CONET_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <netinet/ip.h>
#include <stropts.h>
#include <sys/ioctl.h>

#include <linux/if_packet.h>
#include<linux/if_ether.h>
#include<linux/if_packet.h>
#include<linux/if_arp.h>
#include<netinet/ip.h>

// questi include vanno dentro conet.h
#include <netdb.h>
#include <netinet/ip6.h>

#include "../../ccnd/ccnd_private.h"
#include <ccn/ccnd.h>
#include <ccn/charbuf.h>
#include <ccn/coding.h>
#include <ccn/indexbuf.h>
#include <conet/conet_packet.h>
#include <conet/conet_timer.h>
#include <ccn/uri.h>
#include <ccn/hashtb.h>
#include <ccn/ccn.h>
#include <ccn/schedule.h>

#define IS_CLIENT
//#define IS_SERVER
//#define IS_CACHE_SERVER


#define CONET_IFNAME "eth0"
#define CONET_DEFAULT_SERVER_ADDR 	"192.168.1.8" //This is needed by the cache (in listener.c)

//idirizzi per ipv6 hardcodati
//#define DEST_MAC_ADDR		{0x08, 0x00, 0x27, 0x17, 0xcf, 0xed}
#define DEST_MAC_ADDR		{0xbc, 0x5f, 0xf4, 0x40, 0x71, 0x9a} //bc:5f:f4:40:71:9a
#define SRC_IP6_ADDR "fe80::a00:27ff:fe1a:c152"

// ofelia barcelona island server
//#define SOURCE_MAC_ADDR		{0x02, 0x03, 0x00, 0x00, 0x00, 0xb9}

//ofelia barcelona island cache server
//#define SOURCE_MAC_ADDR         {0x02, 0x03, 0x00, 0x00, 0x00, 0xb0}

// stefano's Virtual Box client
//#define SOURCE_MAC_ADDR		{0x08, 0x00, 0x27, 0x2a, 0x38, 0x47}
// stefano's Virtual Box server
//#define SOURCE_MAC_ADDR          {0x08, 0x00, 0x27, 0x30, 0x58, 0xc1}

//#define TAG_VLAN_DEFAULT	0	//&ct - instead of 1
#define MIN_PACK_LEN_DEFAULT 100

//#define CONET_CLIENT_IP "192.168.1.65" //2012 05 20 SS: it is only used by setup_ip_header which in turn is never used !

#define TCP_BUG_FIX
//#define K_MODULE
//#define CONET_TRANSPORT
#define IPOPT

#define CONET_USE_REPO	1
#define CONET_DEBUG		0 //adesso due livelli di debug 1, 2, (-3)
#define CONET_MULTI_HOP_DEBUG		0
#define CONET_USE_TIME_TO_STALE  //define to use time_to_stale on content, to change time_to_stale go in ccnd.c
//#define CONET_MULTI_HOP //do not define, we are still working on it
#ifdef CONET_MULTI_HOP
#undef CONET_USE_TIME_TO_STALE
#endif
//if set to 1, program may exit when an unexpected error occurs. 
//This may help in locating errors.
#define CONET_SEVERE_DEBUG	1

#define CONET_FORWARDING 0
#define CONET_LOG_TO_FILE	0

#define CONET_PROTOCOL_NUMBER 17
#define CONET_VLAN_ID 16
#define MAX_THRESHOLD 	20000

#ifdef IS_SERVER
#define SS_THRESH_DEFAULT 200
#else
#define SS_THRESH_DEFAULT 20
#endif

#define USE_CONET_DEFAULT 1


#define INTEREST_CIU 	1
#define DATA_CIU 		2

#define CONET_IP_OPTION_CODE 158
#define CONET_IPV6_OPTION_CODE 62
//#define RAW_IPV6



#define CONET_DEFAULT_UNICAST_PORT_NUMBER		9698U //[CONET]
#define CONET_DEFAULT_RAW_PORT_NUMBER			9699U
#define CONET_DEFAULT_UNICAST_PORT				"9698"

#define CONET_CIU_FLAGS_POS						0U
#define CONET_CIU_DEFAULT_NID_POS				1U
#define CONET_CIU_SHIFTED_NID_POS				2U
#define CONET_CIU_SHIFTED_NID_LENGTH_POS		1U

#define CONET_INTEREST_CIU_TYPE_FLAG			1U
#define CONET_NAMED_DATA_CIU_TYPE_FLAG			2U
#define CONET_CIU_NOCACHE_FLAG					0U
#define CONET_CIU_CACHE_FLAG					1U
#define CONET_CIU_DEFAULT_NID_LENGTH_FLAG		0U
#define CONET_CIU_VARIABLE_NID_LENGTH_FLAG		2U

#define CONET_FINAL_SEGMENT_FLAG                1U

#define CONET_ASK_CHUNK_INFO_FLAG				1U


#define CONET_DEFAULT_NID_LENGTH				16U
#define CONET_MAX_CHUNK_SIZE  262144 //256*1024 //TODO

#define CONET_DEFAULT_MTU						1400 //1468



/**
 * Set this to 1 to use conet carrier packet
 */
extern int USE_CONET ;// set to 1 activate  conet protocol in sending and receiving interest
extern int CONET_PREFETCH ; //set to 1 to activate prefetch
//extern int TAG_VLAN ;
extern int MIN_PACK_LEN;
extern int SS_THRESH;
extern int USER_PARAM_1;

extern struct conet_entry* ce_cache;

typedef struct {
	unsigned short bit:1;
} conet_bit;

#ifdef CONET_MULTI_HOP //simple pit entry (solo una prova)
#define S_PIT_ENTRY_NOME_SIZE 255
struct s_pit_entry{
	int seg_index;
	char nome[S_PIT_ENTRY_NOME_SIZE];
	struct hashtb * c_addr;
};
struct addr_entry{
	in_addr_t clia_addr;
};
#endif


struct chunk_entry{
	//the key
	unsigned long long chunk_number;

	unsigned int last_interest; //it's the last interest received +1
	unsigned int last_possible_interest; // it's the last interest that i want to send +1
	unsigned int last_in_sequence;
	unsigned int last_interest_sent;
	unsigned int final_segment_received;

	unsigned int bitmap_size; //bitarray size, number of cp per chunk

#ifdef CONET_MULTI_HOP
	unsigned int cp_bitmap; //whr
	unsigned short int inviato ;
    struct hashtb *s_pit;			//simple pit (solo per prova) whr
#endif
	//bitarray for carrier packet transmission
	conet_bit* sending_interest_table; 	//&ct(interpretazione): sending_interest_table[i]==1 
						//iff the interest for segment i has already been sent
						//This is used to prevent duplicate transmission

	conet_bit* out_of_sequence_received;
	conet_bit* expected;
	unsigned return_faceid; //used to send back the chunk to ccnx
	unsigned send_faceid;
	unsigned int current_size;
	unsigned int m_segment_size;
	unsigned int starting_segment_size; //set to m_segment_size as default
		//starting_segment_size is the size of the first segment of this chunk
		//This size could be different from other segment size

	unsigned int bytes_not_sent_on_starting; //set to m_segment_size as default
	unsigned int never_arrived;
	unsigned int arrived;
	unsigned int is_oos;
	unsigned int tries; //number of resend (used for stop resend)
	struct timeval* last_sent; //the time of first carrier packet sent.
	struct timeval* timeout;
	double send_started;
	struct timeout_entry* to;
	struct ccn_scheduled_event* reschedule_send_ev;
	struct ccn_indexbuf* outbound;
	//finally, the content of the chunk
	unsigned char*chunk;
};


struct conet_entry {
//	struct conet_entry *prev;
//	struct conet_entry *next;
	//data to recognize a conet_entry
	char* nid;
	unsigned long long chunk_size;
	//data for transmission of segment
	unsigned int cwnd; //window size
	unsigned int in_flight; //number of interest in flight it increase when an interest is sent and decrease when a data cp arrive
	unsigned int in_fast_rec;
	unsigned int threshold;
	unsigned int last_processed_chunk; //numbero of the chunk that has to be completed (conet can start requesting another chunk before this one is completed)
	int final_chunk_number; //-1 if final not arrived
	//unsigned int in_progress; //if -1 no chunk has already asked segment
	double wnd_adder;

	int send_fd;

	double srtt;
	double rttvar;
	double rto;

	unsigned int completed ;
	unsigned int out_of_sequence_counter;
	struct chunk_entry* chunk_expected;  //it is the chunk that has to be completed (conet can start requesting another chunk before this one is completed)
	//data for segment treatment
	struct hashtb *chunks;
#ifdef TCP_BUG_FIX
	unsigned long long recover_chunk;
	unsigned int recover_cp;
#endif
#ifdef CONET_MULTI_HOP
	in_addr_t c_addr ; //whr
	unsigned int chunk_counter;
#endif
};
/**
 * This struct is made for caching purpose on 'server side'
 */
struct conet_content_cache{
	char* name;
	unsigned long long chunk_number;
	struct content_entry * ccn_content;
};

struct conet_sched_param {
	struct conet_entry* ce;
	struct chunk_entry* ch;
};

/*Network functions*/

//adapted replica of ccnd_listen_on_wildcards for CoNet server
//Create the default socket on which listening on (datagram socket on port 9697, receiving from any addresses)
//Return -1 if an error occurred during setting-up the CoNet server, 0 otherwise
int conet_listen_on_wildcards(struct ccnd_handle *h);

//adapted replica of ccnd_listen_on_address for CoNet server
//TODO use after implementing support for listening on a different port (see conet_listen_on)
int conet_listen_on_address(struct ccnd_handle *h, const char *addr);

//adapted replica of ccnd_listen_on for CoNet server
//TODO no support for listening on a different port
int conet_listen_on(struct ccnd_handle *h, const char *addrs);

//Retrieve the input from the CoNet port and call the function to process the received message.
//Return the number of read bytes
//Return -1 if receiving input error
int conet_process_input(struct ccnd_handle* h, int fd);

//analyze the flags of the UDP CoNet message and switch between an Interest carrier packet handle or a Data carrier packet handle
#ifdef CONET_MULTI_HOP
void conet_process_input_message(struct ccnd_handle* h, char* src_addr, unsigned char* readbuf, unsigned short is_raw,int size);
void conet_process_raw_input_message(struct ccnd_handle* h, char* src_addr,unsigned char* readbuf, int size);
#else
void conet_process_input_message(struct ccnd_handle* h, char* src_addr, unsigned char* readbuf, unsigned short is_raw);
void conet_process_raw_input_message(struct ccnd_handle* h, char* src_addr, unsigned char* readbuf);
#endif

//analyze the UDP CoNet message as an Interest carrier packet
//Return number of sent bytes for the interest carrier-packet
//Return 0 if no interest has been requested (no cache interest)
//Return -1 if the interest carrier-packet cannot be satisfied
//Return -2 if an error has occurred in allocating memory
//Return -3 if an error has occurred during sending data carrier-packet response
int conet_process_interest_cp(struct ccnd_handle* h, char *src_addr, unsigned char* readbuf, unsigned short ll, unsigned short c, unsigned short ask_info, unsigned short is_raw);

//analyze the UDP CoNet message as a Data carrier packet
//If it's necessary to send interest carrier-packets for the next segment the conet_send function is called and a value > 0 is returned representing the last window size (NB: the new one has already been set by conet_send //TODO)
//Return 0 if the chunk has been completely received and has been forwarded to ccnx
//Return -1 if the data carrier-packet contains reserved flags
//Return -2 if an error occurred searching in hashtable-data-structures
//Return -3 if no interest has been sent for this named-data-content or if it was a mistake delivery
//Return -4 if no interest has been sent for this data carrier-packet or if it was a mistake delivery
//Return -5 if unexpected replication of an old segment received
//Return -6 if a request of new segments failed
int conet_process_data_cp(struct ccnd_handle* h, char *src_addr, unsigned char* readbuf, unsigned short ll, unsigned short c, unsigned short is_raw);

//retrieve the content from the skiplist available with ccnd_handle
struct content_entry * find_ccn_content(struct ccnd_handle *h, struct ccn_charbuf* name);

//send interest carrier-packet for completing a chunk
//Return -1 if an error occurred during setting socket or sending bytes
//TODO set other return-values
//TODO set window control
//TODO set management of out_of_sequence/unexpected packets
int conet_send_interest_cp(struct ccnd_handle* h, struct conet_entry* ce, struct chunk_entry* ch, short rescheduled_send);


int setup_raw_socket_listener(void);

int setup_local_addresses(struct ccnd_handle* h);

#ifdef CONET_MULTI_HOP
void change_eth_dst_src(struct sockaddr_in* next_hop, unsigned char * packet);
int propagate_interest_cp(struct ccnd_handle* h, char *src_addr, unsigned char* readbuf, unsigned short ll, unsigned short c, unsigned short ask_info, unsigned short is_raw,int size);
int propagate_data_cp (struct ccnd_handle* h, char *src_addr,unsigned char* readbuf, unsigned short ll, unsigned short c,unsigned short is_raw, int msg_size);
#endif

//send the conet segment (data carrier-packet) to the previous requesting node //TODO set the path state in the next release!
//Return the number of bytes sent
int conet_send_data_cp(char* src_addr, int chunk_size, unsigned char * segment, unsigned int segment_size, const char* nid, unsigned long long csn, unsigned int l_edge, unsigned int r_edge, unsigned int is_final, unsigned int add_chunk_info, unsigned short is_raw);

//send the chunk to CCNx after it was previously reconstructed by data carrier-packet
void conet_send_chunk_to_ccn(struct ccnd_handle* h, struct chunk_entry* ch, unsigned int chunk_size);

/**
 * extimate the segment size based upon the maximum overhead
 * of carrier packet, and the mtu of the transimission link.
 * mtu can be passed or fixed (last param set to 0).
 */
unsigned short get_segment_size(char * nid, unsigned long long csn, int chunk_size, int mtu);

int conet_rescheduled_send(struct ccn_schedule *sched, void *clienth, struct ccn_scheduled_event *ev, int flags);


int retransmit(struct ccnd_handle* h, struct conet_entry* ce, struct chunk_entry * ch);

void window_control(struct conet_entry * ce, struct chunk_entry *ch);

void update_timeout(struct conet_entry * ce, struct chunk_entry *ch, short rescheduled_send);

void setup_chunk_bitmaps(struct conet_entry* ce, struct chunk_entry* ch);

struct chunk_entry* chunk_change(struct ccnd_handle* h, struct conet_entry* ce, struct chunk_entry* ch, int remaining);

void close_chunk_transmission(struct ccnd_handle * h,struct conet_entry* ce);

int is_eof(struct ccnd_handle * h, const unsigned char* ccnb, unsigned int size);

void conet_print_stats(struct conet_entry* ce, struct chunk_entry* ch, unsigned int line);

void remove_chunk(struct conet_entry *ce, struct chunk_entry* ch);

void log_to_file(FILE* fd, int a1, int a2, int a3);

int content_match(struct content_entry *content, struct ccn_charbuf* name);

#ifdef RAW_IPV6
struct sockaddr* setup_raw_sockaddr(struct sockaddr_storage* next_hop);

unsigned char* setup_ip_header(struct sockaddr_storage* this_hop, struct sockaddr_storage* next_hop, unsigned char * packet, size_t packet_size, size_t ipoption_size);

unsigned char* setup_ipeth_headers(struct sockaddr_storage* this_hop, struct sockaddr_storage* next_hop, unsigned char * packet, size_t * packet_size, size_t ipoption_size);

//int get_mac_from_ip(unsigned char * src_mac, struct sockaddr_storage *staddr, int *if_index);
#else
struct sockaddr* setup_raw_sockaddr(struct sockaddr_in* next_hop);

unsigned char* setup_ipeth_headers(struct sockaddr_in* this_hop, struct sockaddr_in* next_hop, unsigned char * packet, size_t * packet_size, size_t ipoption_size);
#endif
int get_mac_from_ip(unsigned char * src_mac, struct sockaddr_in *addr, int *if_index);

/**
 * A chunk is composed by a sequence of segments. Each segment is identified by a s
 * sequence number called seg_index. This function calculates seg_index
 */
int calculate_seg_index (unsigned long long l_edge, 
        unsigned long long r_edge, unsigned int m_segment_size, 
        unsigned int starting_segment_size, unsigned int bytes_not_sent_on_starting);

#endif /* CONET_H_ */
