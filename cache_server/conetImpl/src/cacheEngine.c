/**
 * Author: andrea.araldo@gmail.com
 */


#include <cache_table.h>	//for Cache_entry_t
#include <cacheEngine.h> 	//for socket_to_controller
#include <conet_for_cache.h>
#include <conet/conet.h>	//for CONET_DEBUG and CONET_SEVERE_DEBUG
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/dir.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <java_to_c_adapter.h>
#include <utilities.h>
#include <c_json.h>


#define SEGMENT_SUFFIX ".chu"

//Important definitions
//Leos
#define MAX_BUFFER 1024

char msg_to_controller_buffer[BUFFERLEN]; //BUFFERLEN defined in c_json.h
char my_cache_server_ip [20];
char my_cache_server_mac [20];

typedef struct chunkid{
	long nid_length;
	long csn;
	char* nid;
} chunk_id;

typedef struct segmentid{
	chunk_id chunk;
	long left_byte;
	long segment_length;
} segment_id;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void parsing(char* buffer, chunk_id* chk);
void chunk_builder(char* data, chunk_id* chunk);
void add(chunk_id chk);
int precache(segment_id seg_id, char* data);


void calculate_filename (char* nid, unsigned long long csn, UT_string* filename)
{
	utstring_printf(filename,"%s%s/%llu%s\0",CONTAINING_FOLDER,nid,csn,SEGMENT_SUFFIX);
}


long readSegmentFromFileSystem(
	const char* nid,unsigned long long csn,size_t leftByte, size_t segmentSize,void* buffer)
{
	size_t bytesRead=0; size_t fileNameSize;
	size_t bytesToRead;
	FILE *fp; //file pointer

	//variables to update cache_table
	UT_string* utnid;
	UT_string* filename; utstring_new(filename);

	calculate_filename( (char*)nid,csn,filename);
	if(CONET_DEBUG) 
		fprintf(stderr,"[cacheEngine.c:%d]Opening file \"%s\" \n",__LINE__, utstring_body(filename) );

	fp = fopen( utstring_body(filename), "rb");
	if (fp == NULL)
	{
		fprintf(stderr, "[cacheEngine.c:%d]Error: Failed to open the file %s; maybe the chunk does not exist\n\n", 
			__LINE__, filename);
		if (CONET_SEVERE_DEBUG) exit(-651);
		return ( (size_t) - 1);
	}

	//!!!Questo fa andare in buffer overflow, il primo parametro dovrebbe essere
	//buffer senza +leftByte
	//Go to the leftByte location
	int fseek_return = fseek(fp,leftByte,SEEK_SET);
	if(fseek_return!=0)
	{
		fprintf(stderr,"[cacheEngine.c:%d]Error reading file %s. fseek returned with %d (ferror() tests the error indicator for the stream pointed to by stream,  returning nonzero if it is set.)\n",
			__LINE__,utstring_body(filename),fseek_return );
		//if(CONET_SEVERE_DEBUG) exit(-652);
		
		return ( (size_t) -0);
	}
	
	if(feof(fp))
	{
		fprintf(stderr,"[cacheEngine.c:%d]Error reading file %s. feof(fp)=%d\n",
			__LINE__,utstring_body(filename), feof(fp));
		//if(CONET_SEVERE_DEBUG) exit(-644);
		
		return ( (size_t) -0);
	}

	clearerr(fp);

	bytesRead = fread(buffer, 1, segmentSize, fp);

	if (bytesRead <= 0)
	{
		fprintf(stderr,"%s\n",utstring_body(filename) );
		fprintf(
			stderr, 
			"[cacheEngine.c:%d] Error: in reading file %s. fread returned with %u. ferror=%d, feof(fp)=%d (ferror() tests the error indicator for the stream pointed to by stream,  returning nonzero if it is set.)\n\n",
			__LINE__,utstring_body(filename), bytesRead,ferror(fp), feof(fp)
		);

		if (CONET_SEVERE_DEBUG) exit(-652);
		return bytesRead;
	}

	fclose(fp);

	if(CONET_DEBUG)
		fprintf(stderr,"[cacheEngine.c:%d]Successfully read  %d bytes\n", 
			__LINE__,(int) bytesRead);

	utstring_free(filename);
	return bytesRead;
}

/**
 * Searches the chunk.
 * If the chunk is found, puts segment bytes in buffer and returns the size of the extracted segment
 * If no dataCIU is found, returns a negative number
 * @param nid network identifier (must be NULL-TERMINATED)
 * @param csn chunk sequence number (must be NULL-TERMINATED)
 * @param leftByte, segmentSize: the segment start at leftByte and ends segmentSize bytes after.
 * @param buffer memory area where to put the segment
 * @param is_final: it will be set to 1 if the segment is the final, 0 otherwise
 */
long retrieveSegment(
		const char* nid,unsigned long long csn,long tag,size_t leftByte, size_t segmentSize,
		void* buffer,unsigned int* is_final, unsigned long long* chunk_size)
{
	if(CONET_SEVERE_DEBUG && csn>=100)
	{
		fprintf(stderr,"[cacheEngine.c:%d] csn=%llu. This is very large. It is not necessarily an error but ... are you sure?\n",__LINE__, csn);
		exit(-8776);
	}


	int bytesRead=0; char* fileName; size_t fileNameSize;
	size_t bytesToRead;
	FILE *fp; //file pointer

	//variables to update cache_table
	UT_string* utnid;
	char** conversion_pointer=NULL;

	if (CONET_DEBUG)
		fprintf(stderr,"\n[cacheEngine.c: %d] Request for nid=%s, csn=%ull, tag=%ld\n",
		__LINE__,nid,csn,tag);

	//update table

	utstring_new(utnid);
	utstring_printf(utnid, nid );
	if(CONET_DEBUG)
		fprintf(stderr,"[cacheEngine.c: %d] Updating cache table. Now the chunk nid=%s, csn=%llu is at the head\n",
		__LINE__,nid,csn);
	*chunk_size=find_on_cache_table(cache_table,utnid,csn);

	if (CONET_DEBUG)
		fprintf(stderr,"[cacheEngine.c: %d] chunk_size=%llu\n",__LINE__, *chunk_size);

	if (*chunk_size<0)
	{
		if (CONET_SEVERE_DEBUG) exit(-642);
		return (long) *chunk_size;
	}
	
	bytesRead=readSegmentFromFileSystem(nid,csn,leftByte, segmentSize,buffer);

	if (CONET_DEBUG)
		fprintf(stderr,"[cacheEngine.c: %d] chunk size is is %llu and %d(leftByte) + %d(bytesRead)=%d\n",
			__LINE__, *chunk_size,leftByte ,bytesRead,leftByte+bytesRead);

	if (bytesRead<=0) 
	{
		fprintf(stderr,"[cacheEngine.c:%d] reading file: bytesRead=%d\n",
			__LINE__,bytesRead);
		//if (CONET_SEVERE_DEBUG) exit(-989);
		return bytesRead;
	}else
	{
		if (leftByte+bytesRead== *chunk_size)
			*is_final=1;
		else if (leftByte+bytesRead > *chunk_size)
		{
			fprintf(stderr,"[cacheEngine.c: %d] Error: Error chunk_size(%d)<leftByte(%d)+bytesRead(%d). This is a nonsense \n",
				__LINE__,*chunk_size, leftByte, bytesRead);
			if (CONET_SEVERE_DEBUG) exit(-3);
			return -3;
		}
		else *is_final=0;
	}
	return bytesRead;
}

/**
 * @param nid network identifier (must be NULL-TERMINATED)
 * @param chunk_number chunk sequence number (must be NULL-TERMINATED)
 * @param leftByte, segmentSize: the segment start at leftByte and ends segmentSize bytes after.
 * @param cp_descriptor: it is filled in listener.c and defined in conet/info_passing.h
 * @return 0 chunk is saved in cache
 * @return 1 segment is saved in pre-cache
 * @return 2 segment is duplicated.
 */
struct content_entry_adapted_to_cache* nid_table=NULL;

size_t handleSegment(cp_descriptor_t* cp_descriptor)
{
	//TODO: forse cp_descriptor->m_segment_size e' inutile
	struct content_entry_adapted_to_cache* ce=NULL;
	struct chunk_entry_adapted_to_cache* ch=NULL;
	char nid[cp_descriptor->nid_length+1]; //+1 for the escape character
	memset(nid,   '\0',  cp_descriptor->nid_length+1);
	memcpy(nid, cp_descriptor->nid, cp_descriptor->nid_length);
//	snprintf(nid,cp_descriptor->nid_length+ 1, "%s", cp_descriptor->nid);
        escape_nid(nid,cp_descriptor->nid_length);
	unsigned long long csn= cp_descriptor->csn;
	long tag=(long)from_byte_array_to_number(cp_descriptor->tag);
	unsigned long long segment_size = cp_descriptor->segment_size;
	unsigned long long m_segment_size=0;
	m_segment_size = cp_descriptor->m_segment_size;
	unsigned long long  l_edge = cp_descriptor->l_edge;
	
	if(CONET_SEVERE_DEBUG && csn>=100)
	{
		fprintf(stderr,"[cacheEngine.c:%d] csn=%llu. This is very large. It is not necessarily an error but ... are you sure?\n",__LINE__, csn);
		exit(-8776);
	}

	
	//data buffer
	 void* buffer=(void*)(cp_descriptor->nid + cp_descriptor->nid_length );

	//TODO: eliminare questa ridondanza di informazione
	if (segment_size <= 0)
	{
		fprintf(stderr,"[cacheEngine.c:%d]: cp_descriptor->m_segment_size=%llu, segment_size=%llu\n",
			__LINE__,m_segment_size, segment_size);
		if(CONET_SEVERE_DEBUG) exit(-13);
		return -1;
	}

	unsigned long long right_edge=l_edge + segment_size-1;
	if (CONET_DEBUG)
		fprintf(stderr,"\n[cacheEngine.c: %d] handle segment: %s/%d l_edge:%d, right_edge:%d ", __LINE__,nid, csn, l_edge, right_edge);
	//first init of nid_table
	if (nid_table==NULL){
		if(CONET_DEBUG) fprintf(stderr,"\n [cacheEngine.c: %d] first creation of nid_table: nid=%s/chunk=%d ", __LINE__,nid, csn);
		ce = (struct content_entry_adapted_to_cache*)
			malloc(sizeof(struct content_entry_adapted_to_cache) );
		//In ce we will mantain all the info related to a chunk
		//SS added +1 in the malloc following valgrind 
		ce->nid=malloc(cp_descriptor->nid_length+1); //vecchia versione: ce->nid=malloc(strlen(nid));
		strcpy(ce->nid, nid);
		ce->chunk_head=NULL;
		HASH_ADD_KEYPTR( hh, nid_table, ce->nid, cp_descriptor->nid_length, ce );
		//vecchia versione: HASH_ADD_KEYPTR( hh, nid_table, ce->nid, strlen(ce->nid), ce );
		nid_table=ce;
	}else if(CONET_DEBUG) fprintf(stderr,"\n[cacheEngine.c: %d] nid_table already created",__LINE__);
	//search the nid_table for corresponding NID or create a new entry.
	HASH_FIND_STR(nid_table, nid, ce);
	if (ce==NULL){
		if(CONET_DEBUG)
			fprintf(stderr,"\n[cacheEngine.c: %d] %s entry does not exist in precache, creating it", __LINE__,nid);
		ce = (struct content_entry_adapted_to_cache*)malloc(sizeof(struct content_entry_adapted_to_cache));
		ce->nid=malloc(strlen(nid)+1);
		strcpy(ce->nid, nid);
		ce->chunk_head = NULL;
		HASH_ADD_KEYPTR( hh, nid_table, ce->nid, strlen(ce->nid), ce );
	}
	else if(CONET_DEBUG)
		fprintf(stderr,"\n[cacheEngine.c: %d] nid %s exist in precache",__LINE__, nid);

	

	//creation of chunk hash table
	if (ce->chunk_head==NULL){
		if (CONET_DEBUG)
			fprintf(stderr,"\n[cacheEngine.c: %d] %s/%d entry doesn't exist in precache, creating it",__LINE__, nid, csn);
		ch = (struct chunk_entry_adapted_to_cache * )malloc(sizeof (struct chunk_entry_adapted_to_cache));
		memset(ch,0,sizeof(struct chunk_entry_adapted_to_cache));//&ct
		ch->chunk_number=csn;
		ch->last_in_sequence=0; //&ct
		ch->last_in_sequence_right_edge=-1;//&ct
		HASH_ADD_INT(ce->chunk_head, chunk_number, ch);
	}
	else {
		//search for chunk entry, if chunk not exist it creates it.
		HASH_FIND_INT(ce->chunk_head, &csn, ch);
		if (ch==NULL){
			//			fprintf(stderr,"\n %s/%d entry does not exist in precache, creating it", nid, csn);
			ch = (struct chunk_entry_adapted_to_cache * )malloc(sizeof (struct chunk_entry_adapted_to_cache));
			ch->chunk_number=csn;
			ch->chunk=NULL;
			ch->received=NULL;
			ch->buffer_size=0;
			ch->last_in_sequence=0; //&ct
			ch->last_in_sequence_right_edge=-1;//&ct
			ch->max_segment_received=0;
			ch->max_buffer_size=0;
			ch->completed=0;
			HASH_ADD_INT(ce->chunk_head, chunk_number, ch);
		}
		else {
			if (CONET_DEBUG) fprintf(stderr,"\n[cacheEngine.c: %d] chunk nid=%s/csn=%d exists,(preaceche has already started to build it)",__LINE__, nid,csn);
			if (ch->completed==1){
				//chunk exist and is completed: segment is duplicated
				fprintf(stderr,"\n CHUNK %s/%llu IS ALREADY IN CACHE",nid,csn);
				return 2;
			}
		}
	}

	if (CONET_DEBUG)
		fprintf(stderr,"\n[cacheEngine.c: handleSegment(..): line%d] Before this segment arrived, buffer_size of the chunk was %d",__LINE__,ch->buffer_size);

	if(CONET_DEBUG)	fprintf(stderr,"\n[cacheEngine.c: %d] saving chunk %s/%d in precache\n",__LINE__, nid,csn);
	//if needed update the chunk buffer.
	if (ch->chunk==NULL || ch->max_buffer_size <= l_edge){
		ch->max_buffer_size+=PRECACHE_DEFAULT_CHUNK_SIZE_ADAPTED_TO_CACHE;
		if (CONET_DEBUG) 
			fprintf(stderr,"\n [cacheEngine.c%d: handleSegment(..)]Space allocated for the chunks is not enough: reallocating chunk for %d byte %s/%d exist",
				__LINE__,ch->max_buffer_size , nid,csn);
		
		unsigned char* new_space=malloc(ch->max_buffer_size );
		memset(new_space,0,ch->max_buffer_size);
		memcpy(new_space,ch->chunk, ch->buffer_size);
		fprintf(stderr,"prima di free\n");
		free(ch->chunk);
		fprintf(stderr,"dopo di free\n");
		ch->chunk=new_space;
		fprintf(stderr,"dopo risistemazione ch-chunk\n");


//		ch->chunk=realloc(ch->chunk,ch->max_buffer_size);
//		unsigned long long toerase = ch->max_buffer_size - ch->buffer_size;
		//reset avalaible chunk buffer to 0
//		memset(ch->chunk+ch->buffer_size, 0, toerase);
	}

	int segment_index ;
	if (l_edge==0){ //This is the first segment
		segment_index=0; 
		ch->chunk_size = cp_descriptor->chunk_size;
		ch->m_segment_size = cp_descriptor->m_segment_size;
		ch->bytes_not_sent_on_starting = cp_descriptor->bytes_not_sent_on_starting;
	}
	else
		segment_index = calculate_seg_index( //defined in conet.h
			(unsigned long long)l_edge, (unsigned long long) l_edge+segment_size-1,
			m_segment_size, ch->starting_segment_size,  ch->bytes_not_sent_on_starting
		);
		//Attenzione: prima si passava ch->m_segment_size anziche' segment_size

	if(CONET_DEBUG) 
		fprintf(stderr,"\n[cacheEngine.c %d] segment_index=%d",__LINE__,segment_index);
/*
	//&ct:inizio
//	if (flags&FINAL_SEGMENT_FLAG_ADAPTED_TO_CACHE==0) //This is not the final segment
//		segment_index=(right_edge+1)/segmentSize;
//	else //This is the final segment
//	{
		if (l_edge==ch->last_in_sequence_right_edge+1)
		//The last in sequence segment is the second last
			segment_index=ch->last_in_sequence+1;
		else{
			fprintf(stderr,"[cacheEngine.c: handleSegment(..): line%d] last_in_sequence_right_edge=%d. This segment is out of sequence. Give up",__LINE__,ch->last_in_sequence_right_edge);
			return -3;
		}
//	}
	//&ct: fine
*/
	int max_segment=ch->max_buffer_size/segment_size;
	//if needed allocate or create the received bitmap
	if (segment_index>max_segment || ch->received==NULL){
		int newmaxsegment = max_segment +  PRECACHE_DEFAULT_CHUNK_SIZE_ADAPTED_TO_CACHE/segment_size;
		//		fprintf(stderr,"\n allocating received for %d segments", newmaxsegment);
		//SS added +1 in the malloc following valgrind
		ch->received=malloc(1+newmaxsegment*sizeof(conet_bit)); //ch->received=realloc(ch->received, newmaxsegment);
		//		fprintf(stderr,"\n resetting from %d", ch->max_segment_received);
		int i=ch->max_segment_received+1;
		//reset received fields to 0.
		while(i<= newmaxsegment){
			ch->received[i].bit=0;
			i++;
		}
		ch->max_segment_received=segment_index;
	}
	if (CONET_DEBUG) 
		fprintf(stderr,"\n [cacheEngine.c: handleSegment(..)]is the segment with index %d received?= %d ",
			segment_index, ch->received[segment_index].bit);
	if (ch->received[segment_index].bit==0 ){ //SS TODO? Conditional jump or move depends on uninitialised value(s)
		if (CONET_DEBUG) 
			fprintf(stderr,"\n saving segment %d for %d byte %s/%d ",segment_index, right_edge, nid,csn);
		memcpy(ch->chunk+l_edge, buffer,segment_size);
		ch->received[segment_index].bit=1;
		if (ch->max_segment_received < segment_index){
			ch->max_segment_received=segment_index;
		}
		if (ch->buffer_size<right_edge){
			ch->buffer_size=right_edge+1;
		}

		fprintf(stderr,"\n SEGMENT #%d L_BYTE:%d R_BYTE:%llu FOR CONTENT HANDLED SUCCESSFULLY %s/%llu\n", segment_index, l_edge, right_edge, nid, csn);

	}
	else {//ch->received[segment_index].bit!=0
		fprintf(stderr,"\n SEGMENT #%d FOR %s/%llu IS DUPLICATED",segment_index, nid,csn);
		return 2;
	}
	//XXX in case of null byte problems decomment this.
	//	debug_precache(ch->chunk, ch->buffer_size);
	//	int j=0;
	//	for (j=0; j<=ch->max_segment_received; j++){
	//		fprintf(stderr,"\n received[%d]=%d", j, ch->received[j].bit);
	//	}
	//update last segment in sequence if segments arrives in sequence.
	if (ch->last_in_sequence +1 ==segment_index){
		ch->last_in_sequence=segment_index;
		ch->last_in_sequence_right_edge=right_edge;
		if (CONET_DEBUG)
			fprintf(stderr,"\n[cacheEngine.c: handleSegment: line %d] The segment is in sequence. Now last segment is sequence is %d and the right edge in sequence is ",__LINE__,ch->last_in_sequence,ch->last_in_sequence_right_edge);
	}else if(CONET_DEBUG) 
		fprintf(stderr,"\n[cacheEngine.c: handleSegment(..): line %d]: segment out of sequence: last_in_sequence=%d, segment_index=%d",__LINE__,ch->last_in_sequence, segment_index);

	if (CONET_DEBUG) 
		fprintf(stderr,"\n[cacheEngine.c: %d] The last segment arrived in sequence is %d ", __LINE__,ch->last_in_sequence);
	if ( cp_descriptor->flags&FINAL_SEGMENT_FLAG_ADAPTED_TO_CACHE != 0 ){
		ch->final_segment_received=1;

		if (ch->last_in_sequence+1 == segment_index){
			//			fprintf(stderr,"\n all in sequence save chunk ");
			handleChunk(nid, csn, tag, ch->buffer_size, (void*)ch->chunk);
			free(ch->chunk);
			ch->completed=1;
			fprintf(stderr,"\n CHUNK %s/%llu.chu SAVED IN CACHE\n",nid, csn);
			return 0;
		}else if (CONET_DEBUG) 
			fprintf(stderr,"\n[cacheEngine.c: handleSegment]: this is the final segment but it is out of sequence. Anyway, it was saved in precache (forse)");
	}else if (CONET_DEBUG)
		fprintf(stderr,"\n[cacheEngine.c: handleSegment(..)]: It is not the final segment");
	if (ch->final_segment_received){
		int i=ch->last_in_sequence;
		while (i<ch->max_segment_received){
			if (ch->received[i].bit==0){
				if(CONET_DEBUG)printf("\n[cachEngine.c: handleSegment(..)] segment %d not yet received ", ch->last_in_sequence);
				break;
			}
			i++;
		}
		if (i==ch->max_segment_received){
			if(CONET_DEBUG)
				fprintf(stderr,"\n[cacheEngine.c: handleSegment(..)] last_in_sequence==max_segment_received => chunk completed. save it  ");
			handleChunk(nid, csn, tag, ch->buffer_size, (void *)ch->chunk);
			free(ch->chunk);
			ch->completed=1;
			fprintf(stderr,"\n CHUNK %s/%llu.chu SAVED IN CACHE\n",nid, csn);
			return 0;
		}else if (CONET_DEBUG)
			fprintf(stderr,"\n[cacheEngine.c: handleSegment(..)] last_in_sequence=%d, max_segment_received=%d. The chunk is not completed. See you soon",i,ch->max_segment_received);
	}

	return 1;
}
/**
 * @param nid network identifier (must be NULL-TERMINATED)
 * @param csn chunk sequence number (must be NULL-TERMINATED)
 * @param buffer memory area where the segment is placed
 */ 
//size_t handleChunk(char* nid,char* csn,long tag,size_t left_edge, size_t segment_size,void* buffer)
size_t handleChunk(char* nid,unsigned long long csn,long tag,unsigned long long chunk_size,void* buffer)
{
	size_t writtenBytes,fileNameSize;
	char* fileName;
	char csnToDelete[64];
	FILE *fp;
	UT_string* folderName;
	utstring_new(folderName);
	UT_string* tempHexCsn;
	utstring_new(tempHexCsn);
	UT_string* filename; utstring_new(filename);


	//variables for cache table
	CacheEntry_t 	*entry, *tmp_entry; 
	CacheEntry_t	*entryToDelete=NULL;
	UT_string 		*utnid, *filenameToDelete;
	//	long 			csnlong;

	//folderName=NULL;
	if(CONET_DEBUG) fprintf(stderr,"\n[cacheEngine.c:%d]New segment nid=%s, csn=%llu received.\n",
		__LINE__,nid,csn);

	calculate_filename(nid,csn,filename);

	utstring_printf(folderName,CONTAINING_FOLDER);
	utstring_printf(folderName,nid);
	mkdir(utstring_body(folderName));
	fp = fopen(utstring_body(filename) , "w+b");

	if (fp == NULL) {
		//SSfprintf(stderr, "Error: Failed to open the file %s for writing;\n\n", fileName);
		fprintf(stderr, "Error: Failed to open the file %s for writing;\n\n", utstring_body(filename));
		if (CONET_SEVERE_DEBUG) exit(-863);
		return ( (size_t) - 1);
	}

	writtenBytes = fwrite(buffer, 1,chunk_size, fp);
	fclose(fp);

	if (writtenBytes!=chunk_size) {
		fprintf(stderr, "Error in writing %s: writtenBytes=%d, while chunk_size=%d\n",
				utstring_body(filename),writtenBytes,chunk_size);
		if (CONET_SEVERE_DEBUG) exit(-8763);
		return ( (size_t) - 1);
	}

	utstring_new(utnid); utstring_printf(utnid,nid);
	//utstring_printf(tempHexCsn,"0x");utstring_printf(tempHexCsn,csn);
	ADD_TO_CACHE_TABLE(cache_table,entry,tmp_entry,tag,utnid,csn,chunk_size,0,entryToDelete);
	
	//creates json message with nid and of type "stored", and store it to msg_to_controller_buffer;
	fill_message(tag, "stored", nid, csn, my_cache_server_ip, my_cache_server_mac, msg_to_controller_buffer);
	//BUFFERLEN is defined in c_json.h
	int bytesSentToController=send(socket_to_controller, msg_to_controller_buffer, BUFFERLEN, 0);
	if(CONET_DEBUG )
		fprintf(stderr,"[cacheEngine.c:%d]Message sent to controller = %s\n",__LINE__,msg_to_controller_buffer);
	if 	(bytesSentToController <=strlen(msg_to_controller_buffer) )
	{
        fprintf(stderr,"[cacheEngine.c: %d] bytes sent to controller=%d\n",__LINE__, bytesSentToController);
        if (CONET_SEVERE_DEBUG) exit(-982);
    }



	if (entryToDelete!=NULL)
	{
		sprintf(csnToDelete,"%x",(unsigned int) entryToDelete->csn);
		CALCULATE_UTSTRING_FILE_NAME(entryToDelete->nid,csnToDelete,filenameToDelete);

		if (remove(utstring_body(filenameToDelete) )==0)
			fprintf(stderr,"File %s deleted.\n",utstring_body(filenameToDelete));
		else
			fprintf(stderr,"Error deleting file %s,\n",utstring_body(filenameToDelete));

		//creates json message with nid and of type "stored", and store it to msg_to_controller_buffer;
		fill_message(entryToDelete->tag, "deleted", entryToDelete->nid, entryToDelete->csn, my_cache_server_ip, my_cache_server_mac, msg_to_controller_buffer);
		//BUFFERLEN is defined in c_json.h
		if 	(send(socket_to_controller, msg_to_controller_buffer, BUFFERLEN, 0) != 
				strlen(msg_to_controller_buffer)
			)
		    fprintf(stderr,"send() sent a different number of bytes than expected\n");
		
		
	}

	fprintf(stderr,"[cacheEngine.c:%d]New chunk added: nid=%s, csn=%llu, tag=%ld\n",__LINE__,nid,csn,tag);

	//    utstring_free(utnid); utstring_free(filenameToDelete);
	utstring_free(filename);

	return writtenBytes;
}



/*
int cache_table_init()
{
	return -1;




	//TODO: don't initialize: you don't know the tag
	long tagFinto=-1;
	DIR *parent_dir, *child_dir;
	struct dirent *ent,*child_ent;
	const char* parent_dir_name="files/chunks/";
	UT_string* child_dir_name; 	utstring_new(child_dir_name);
	UT_string* nid; 			utstring_new(nid);
	UT_string* utfilename;		utstring_new(utfilename);
	char* csn; int csn_length;
	void* chunkBuffer; long chunkSize;
	FILE* chunkFile;

	fprintf(stderr,"Itializing cache table with preexisting chunks\n");	

	parent_dir = opendir (parent_dir_name);
	if (parent_dir == NULL) 
	{
		//could not open directory
		fprintf(stderr,"Error opening directory %s\n",ent->d_name);
		perror ("Could not open directory");
		return -6187;

	}

	// print all the files and directories within directory
	while ( (ent = readdir (parent_dir)) != NULL )
	{//parent_while
		if (   strcmp(ent->d_name,".")!=0 && strcmp(ent->d_name,"..")!=0 )
		{
			utstring_clear(nid); utstring_clear(child_dir_name);
			utstring_printf(nid, ent->d_name);
			utstring_printf(child_dir_name,parent_dir_name);
			utstring_printf(child_dir_name,ent->d_name);

			child_dir=opendir( utstring_body(child_dir_name) );
			if (child_dir==NULL)
			{
				fprintf(stderr,"Error opening directory %s\n",utstring_body(child_dir_name));
				perror ("Could not open directory");return -6187;
			}


			while ( (child_ent = readdir (child_dir)) != NULL )
			{//child_while
				if (   strcmp(child_ent->d_name,".")!=0 && strcmp(child_ent->d_name,"..")!=0 )
				{
					csn_length=strlen(child_ent->d_name) -4;
					if (csn_length<1)
					{
						fprintf(stderr,"Error opening file %s/%s\n",
								utstring_body(child_dir_name),child_ent->d_name );
						return -6253;
					}

					// "+1" is needed to add null terminating character
					csn=(char*) malloc(csn_length+1);
					snprintf(csn,csn_length+1,"%s",child_ent->d_name);

					fprintf(stderr,"\n\nFound the preexisting chunk: nid=%s, csn=%s\n",utstring_body(nid),csn);

					RETRIEVE_THE_WHOLE_SEGMENT(
							utstring_body(nid),csn,tagFinto,chunkSize,utfilename,chunkFile,chunkBuffer);

					handleChunk( utstring_body(nid), atoll(csn), tagFinto, chunkSize,chunkBuffer);
				} //else do nothing because you found "." or ".." in child dir

			}//end of child_while
			closedir(child_dir);

		}//else do nothing because you found "." or ".." in parent dir

	}//end of parent_while

	closedir (parent_dir);

	utstring_free(child_dir_name); utstring_free(nid); utstring_free(utfilename);

	fprintf(stderr,"Initialization finished\n");
}
*/


void cacheEngine_init(char* cache_server_ip, char* cache_server_mac,char* controller_ip_address, unsigned short controller_port)
{
	fprintf(stderr,"Opening socket to talk with controller\n");
	socket_to_controller=create_connection(controller_ip_address, controller_port);
	if (socket_to_controller<0)
		error("ERROR opening socket to the controller\n");
	strcpy	(my_cache_server_ip, cache_server_ip);
	strcpy	(my_cache_server_mac, cache_server_mac);
	sendWelcomeMsgToController(cache_server_mac, cache_server_ip);

}

void sendWelcomeMsgToController(char* my_mac_addr, char* my_ip_addr)
{
	char msg[BUFFERLEN]={'\0'};
	strcat(msg,"{\"type\":\"Connection setup\",\"MAC\":\"");
	strcat(msg,my_mac_addr);
	strcat(msg,"\",\"IP\":\"");
	strcat(msg,my_ip_addr);
	strcat(msg,"\"}");
	
	if 	(send(socket_to_controller, msg, BUFFERLEN, 0) != 
			BUFFERLEN
		)
	{
        fprintf(stderr,"send() sent a different number of bytes than expected\n");
        exit(-65);
    }
}

