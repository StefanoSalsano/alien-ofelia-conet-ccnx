#ifndef CONET_FOR_CACHE_H_
#define CONET_FOR_CACHE_H_

#include <conet/conet.h> 	// for conet_bit
#include "uthash/uthash.h"

#define FINAL_SEGMENT_FLAG_ADAPTED_TO_CACHE 0x01
#define PRECACHE_DEFAULT_CHUNK_SIZE_ADAPTED_TO_CACHE 256*1024 //default size of chunks in conet is 256KB

/*
typedef struct {
	unsigned short bit:1;
} conet_bit;
*/

//&ct: Does it override content_entry defined in ccnd_private.h?
struct content_entry_adapted_to_cache {
	//TODO here comes all content-related info
	//the nid is the key.
	char* nid;
	//only stats
	unsigned int chunk_saved;
	unsigned int chunk_size; //not needed, used only for stats
	struct chunk_entry_adapted_to_cache* chunk_head; //&ct: Does it point to the chunk???
	UT_hash_handle hh;
};

struct chunk_entry_adapted_to_cache {
	//the key of the hash table
	unsigned int chunk_number;
	unsigned int segment_size;
	unsigned int starting_segment_size;
                //starting_segment_size is the size of the first segment of this chunk
                //This size could be different from other segment size

	unsigned int bytes_not_sent_on_starting;
	unsigned int m_segment_size;
		//computed at conet.c

	unsigned int chunk_size;

	//if is set an entry in cache already exists
	unsigned int completed;
	// current buffer size (it is the last byte of the greatest segment received
	unsigned long long buffer_size;
	// buffer size really pre-allocated (it is multiple of PRECACHE_DEFAULT_CHUNK_SIZE)
	unsigned long long max_buffer_size;
	// index of greatest segment received
	unsigned int max_segment_received;
	//bitmap of received segments
	conet_bit* received;
	//it indicates the last segment in sequence received.
	unsigned int last_in_sequence;

	//&ct: indicates the right_edge of the last segment in sequence received
	unsigned int last_in_sequence_right_edge;

	//flags to indicates if the final segment is already received. (activates chunk completion control)
	unsigned short final_segment_received;
	unsigned char* chunk;
	UT_hash_handle hh;
};


#endif /* CONET_FOR_CACHE_H_ */
