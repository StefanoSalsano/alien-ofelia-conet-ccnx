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

/**
 * Expected types
 * char* nid
 * char* csn
 * char* filename
 * size_t fileNameSize
 */
/*
#define CALCULATE_FILE_NAME(nid,csn,filename,fileNameSize)									\
	fileNameSize=(size_t) 											\
    	(																	\
    		strlen(CONTAINING_FOLDER)+strlen(nid)+1+strlen(csn)+			\
            strlen(SEGMENT_SUFFIX)+1										\
        );																	\
    fileName=(char*) malloc(fileNameSize);									\
    snprintf(fileName,fileNameSize,"%s%s/%s%s\0",CONTAINING_FOLDER,nid,csn,SEGMENT_SUFFIX);
*/
CacheEntry_t* cache_table;
int socket_to_controller;


void calculate_filename(char* nid, unsigned long long csn, UT_string* filename);

/**
 * Searches the chunk.
 * If the chunk is found, puts segment bytes in buffer and returns the size of the extracted segment
 * If no dataCIU is found, returns a negative number
 * @param nid network identifier (must be NULL-TERMINATED)
 * @param csn chunk sequence number (must be NULL-TERMINATED)
 * @param leftByte, segmentSize: the segment start at leftByte and ends segmentSize bytes after.
 * @param buffer memory area where to put the segment
 * @param is_final: it will be set to 1 if the segment is the final, 0 otherwise
 * 
 */ 
long retrieveSegment(
        const char* nid,char* csn,long tag,size_t leftByte, size_t segmentSize,
	void* buffer,unsigned int* is_final, unsigned long long* chunk_size
	);
        
long readSegmentFromFileSystem(
		const char* nid,char *csn,size_t leftByte, size_t segmentSize,void* buffer);
        
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
	/*Go again at the beginning*/\
	rewind(file);		\
	fread(buffer, 1, fileSize, file);\
\
	fclose(file);\
	printf("Bytes read from %s: %ld\n",utstring_body(utfilename), fileSize );
	
	
/**
 * Retrieves the whole segment and puts it in the buffer
 * Expected types
 * const char*	nid (it must be alreay filled)
 * char*		csn (it must be alreay filled)
 * long 		tag (it must be alreay filled)
 * long			chunkSize (it will be filled)
 * UT_string*	utfilename (it will be filled)
 * FILE*		file (it will be filled)
 * void*		buffer (it will be filled)
 */
#define RETRIEVE_THE_WHOLE_SEGMENT(nid,csn,tag,chunkSize,utfilename,file,buffer)					\
\
	CALCULATE_UTSTRING_FILE_NAME(nid,csn,utfilename) \
	file = fopen(utstring_body(utfilename), "rb");			\
	if (file == NULL) \
	{\
		fprintf(stderr, "Error: Failed to write the file %s;\n\n", \
		utstring_body(utfilename) );\
		return  - 1;\
	}\
	if (fseek(file,0L,SEEK_END) != 0)\
	{\
		fprintf(stderr,"Error calculating the size of file\n");\
		return -8761;\
	}\
	\
	chunkSize=ftell(file);\
	fclose(file);\
	buffer=malloc(chunkSize);\
	unsigned int* is_final_fictitious=malloc(sizeof(unsigned int) );\
	unsigned long long* chunk_size=malloc(sizeof(unsigned long long));\
	printf("[cacheEngine.h:%d] Please, pay attention: the chunk_size may be wrong\n",__LINE__);\
	retrieveSegment(nid,csn,tag,0,(size_t)chunkSize,buffer,is_final_fictitious,chunk_size);\
	free(is_final_fictitious);  \
	free(chunk_size);


	
void provaController();

void sendWelcomeMsgToController(char* my_mac_addr, char* my_ip_addr);
