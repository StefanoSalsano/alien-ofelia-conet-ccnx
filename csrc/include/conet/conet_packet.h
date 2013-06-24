#ifndef CONET_PACKET_H_
#define CONET_PACKET_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "../../ccnd/ccnd_private.h"
#include <ccn/charbuf.h>

#include <conet/conet.h>


enum conet_chunk_size {
	CONET_CONTINUATION,
	CONET_FOLLOW_VAR_CHUNK_SIZE,
	CONET_2KB,
	CONET_4KB,
	CONET_8KB,
	CONET_16KB,
	CONET_32KB,
	CONET_64KB,
	CONET_128KB,
	CONET_256KB,
	CONET_512KB,
	CONET_1MB,
	CONET_2MB,
	CONET_4MB,
	CONET_8MB
};


/*Carrier packet functions*/

//build a carrier-packet from the passed parameters putting the builded message in the memory area pointed by *buffer
//Return the number of bytes written
//unsigned int build_carrier_packet(unsigned char** buffer, int ciu_type, int cache, const char * nid, unsigned long long chunk_number, unsigned int left_edge, unsigned int right_edge, unsigned short cp_flags, int chunk_size);
unsigned int build_carrier_packet(unsigned char** buffer, int buffer_size, int ciu_type, int cache, const char * nid, unsigned long long chunk_number, unsigned int left_edge, unsigned int right_edge, unsigned short cp_flags, int chunk_size);
//nid must be null-terminated


//obtain the nid from the UDP CoNet message.
//iterator must point to the beginning of NID
//Return the NID in string form.
char* obtainNID(unsigned char** buf_iterator, unsigned short ll_flag);


//obtain the CSN from the position pointed by buf_iterator in the received buffer. The value in the buffer is in network byte order.
//Return the CSN value
//unsigned long long obtainCSN(unsigned char** buf_iterator);

//write variable len field
//bufsize is used to reach the position of the variable to write inside *buf
//number is the value to write
//Return the new dimension of the packet being built
//TODO this function maybe must be private(?)
unsigned int write_variable_len_number(unsigned char** buf, size_t bufsize, unsigned long long number);

//read variable len field
//Fill pos with the position after "number" in the buf (so allocate memory for it previously )
//Return the value read
//TODO this function maybe must be private(?)
unsigned long long read_variable_len_number(unsigned char* buf, int* pos);
/**
 * Set the openflow tag of a packet. A 4 byte tag is set after the CONET IP option, (physically in place of UDP ports).
 * Tag is created via CCNx SHA-256 digest. Tag is inserted at the end of buffer.
 * @param buffer the current packet, it must contain almost IP option fields.
 * @param packet_size
 * @param ciu_type to specify what type of hash is used (hash for interest or hash for data) XXX no more used.
 * @param nid the ICN-ID of content.
 * @return the new packet size
 */
unsigned int set_openflow_tag(unsigned char** buffer, unsigned int packet_size,int ciu_type, 
	const char * nid, unsigned long long csn);

/**
 * Add the CONET IP option at the end of buffer.
 * @param buffer containing almost the IP header.
 * @param ciu_type interest or data
 * @param cache flag
 * @param nid ICN-ID of content
 * @param chunk_number
 * @return  the new packet size
 */
//unsigned int build_ip_option(unsigned char** buffer, int ciu_type, int cache, const char * nid, unsigned long long chunk_number);

unsigned int build_ip_option(unsigned char** buffer, int ciu_type, int cache, const char * nid, unsigned long long chunk_number, int family);
/**
 * Add the CONET carrier packet at the end of buffer.
 * @param buffer containing almost the IP header + CONET IP option.
 * @param ciu_type interest or data
 * @param left_edge left byte of the segment to transmit
 * @param right_edge right byte of the segment to transmit
 * @param cp_flags carrier packet flags
 * @param chunk_size if not 0 chunk size is transmitted with the segment.
 * @return  the new packet size
 */

#endif /* CONET_PACKET_H_ */
