/* 
 * File:   cacheEngine.h
 * Author: andrea.araldo@gmail.com
 *
 * Created on January 24, 2012, 9:41 PM
 */

#ifndef CACHEENGINE_H
#define	CACHEENGINE_H

#ifdef	__cplusplus
extern "C" {
#endif


#ifdef	__cplusplus
}
#endif

#endif	/* CACHEENGINE_H */

#include <cache_table.h> 	//for CacheEntry_t
#include <stdlib.h>
#include <conet/conet.h>
#include <conet_for_cache.h>
#include "conet_types.h"
#include <conet/info_passing.h>
#include <uthash/utstring.h>

#define CONTAINING_FOLDER "files/chunks/"


#define LISTENER_TO_CACHE_ENGINE_PORT 9876

int i=CONET_DEBUG;

CacheEntry_t* cache_table;
int socket_to_controller;


/**
 * @param filename: this will be filled. It must be instantiated with utstring_new(filename) before invoking this method
 */
void calculate_filename(char* nid, unsigned long long csn, UT_string* filename);


#define CALCULATE_FILE_NAME(nid,csn,filename,fileNameSize)	\
	fprintf(stderr, "[cacheEngine.h:%d] CALCULATE_FILE_NAME is forbidden",__LINE__  );	\
	exit(-1769);

/**
 * Searches the chunk.
 * If the chunk is found, puts segment bytes in buffer and returns the size of the extracted segment
 * If no dataCIU is found, returns a negative number
 * @param nid network identifier (must be NULL-TERMINATED)
 * @param csn chunk sequence number
 * @param leftByte, segmentSize: the segment start at leftByte and ends segmentSize bytes after.
 * @param buffer memory area where to put the segment
 * @param is_final: it will be set to 1 if the segment is the final, 0 otherwise
 * 
 */ 
long retrieveSegment(
        const char* nid, unsigned long long csn,long tag,size_t leftByte, size_t segmentSize,
	void* buffer,unsigned int* is_final, unsigned long long* chunk_size
	);
        
long readSegmentFromFileSystem(
		const char* nid, unsigned long long csn,size_t leftByte, size_t segmentSize,void* buffer);
        
/**
 * @param nid network identifier (must be NULL-TERMINATED)
 * @param csn chunk sequence number (must be NULL-TERMINATED)
 * @param buffer memory area where the segment is placed
 * 
 */
size_t handleChunk(char* nid,unsigned long long csn,long tag,unsigned long long chunk_size,void* buffer);


/**
 * @param cp_descriptor: it is filled in listener.c and defined in conet/info_passing.h
 * @return 0 chunk is saved in cache
 * @return 1 segment is saved in pre-cache
 * @return 2 segment is duplicated.
 */
size_t handleSegment(cp_descriptor_t *cp_descr);


void debug_precache(unsigned char* buf, unsigned long long size);

/**
 * Initializes the cache_table with the chunks already existing in filesystem
 */
int cache_table_init();
        
/**
 * To be used for testing purpose
 */
void debug_cache_engine_run();


/**
 * Expected types
 * char* nid (it must be alreay filled)
 * char* csn (it must be alreay filled)
 * UT_string* filename (it will be filled)
 */
#define CALCULATE_UTSTRING_FILE_NAME(nid,csn,filename)						\
	utstring_new(filename);													\
	utstring_printf(filename,CONTAINING_FOLDER);							\
	utstring_printf(filename,nid);											\
	utstring_printf(filename,"/");											\
	utstring_printf(filename,csn);											\
	utstring_printf(filename,SEGMENT_SUFFIX);
	

/**
 * Expected types
 * UT_string*		utfilename (it must be alreay filled)
 * FILE*			file (it will be filled)
 * long				fileSize (it will be filled)
 * void*			buffer (it will be filled)
 */
/*
#define READ_THE_WHOLE_FILE(utfilename, file, fileSize, buffer)					\
	file = fopen(utstring_body(utfilename), "rb");			\
	if (file == NULL) \
	{\
		fprintf(stderr, "Error: Failed to write the file %s;\n\n", \
		utstring_body(utfilename) );\
		return ( (size_t) - 1);\
	}\
\
	if (fseek(file,0L,SEEK_END) != 0)\
	{\
		fprintf(stderr,"Error calculating the size of file\n");\
		return -8761;\
	}\
	\
	fileSize=ftell(file);\
	buffer=malloc(fileSize);	\
	//Go again at the beginning
	rewind(file);		\
	fread(buffer, 1, fileSize, file);\
\
	fclose(file);\
	printf("Bytes read from %s: %ld\n",utstring_body(utfilename), fileSize );
*/

void provaController();

void sendWelcomeMsgToController(char* my_mac_addr, char* my_ip_addr);
