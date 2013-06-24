//&ct
/* 
 * File:   info_passing.h
 * Author: andrea.araldo@gmail.com
 *
 * Created on January 24, 2012, 9:51 PM
 */

#ifndef INFO_PASSING
#define	INFO_PASSING

#ifdef	__cplusplus
extern "C" {
#endif




#ifdef	__cplusplus
}
#endif


#define INTEREST_DESCRIPTOR 'i'
#define DATA_DESCRIPTOR 'd'
#define IP_ADDR_STRING_LEN 16


typedef struct cp_descriptor
{
	char type; //could be INTEREST_DESCRIPTOR or DATA_DESCRIPTOR
	long nid_length;
	unsigned long long csn;
	char source_ip_addr[IP_ADDR_STRING_LEN];//The ip addr of the sending node
	unsigned char tag[4];//udp ports length is 4 bytes
	unsigned long long l_edge;
	unsigned long long segment_size;
	unsigned short is_final; //1 if it is a data carrier packet containing a segment marked as final; 0 otherwise
	unsigned char flags; //Represents if the carrier packet is the final or not and other things (//TODO: ask to cancellieri)

	unsigned int chunk_size; 
		//If this carrier packet is the data packet representing the first
		//segment of a chunk, chunk_size (the size of the whole chunk) must
		//be provided

        unsigned int bytes_not_sent_on_starting;
        unsigned int m_segment_size;
                //computed at conet.c


	char nid[];
	
	//char data[]
} cp_descriptor_t;

/**
 * Expected types
 * unsigned short nid_length //must be filled
 */
#define CALCULATE_INTEREST_DESCRIPTOR_LENGTH(nid_length) \
		(sizeof(cp_descriptor_t)+ nid_length)

/**
 * Expected types
 * unsigned short nid_length //must be filled
 * unsigned short segment_size //must be filled
 */
#define CALCULATE_DATA_DESCRIPTOR_LENGTH(nid_length,segment_size) \
		(sizeof(cp_descriptor_t)+ nid_length)



#endif	/* INFO_PASSING_H */
