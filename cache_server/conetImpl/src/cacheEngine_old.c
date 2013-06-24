/**
 * Author: andrea.araldo@gmail.com
 */


#include "cacheEngine.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <cache_table.h>
#include <sys/dir.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>


#define CONTAINING_FOLDER "files/chunks/"
#define SEGMENT_SUFFIX ".chu"



//Important definitions
//Leos
#define MAX_BUFFER 1024

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


CacheEntry_t* cache_table;

/**
 * Searches the chunk.
 * If the chunk is found, puts segment bytes in buffer and returns the size of the extracted segment
 * If no dataCIU is found, returns a negative number
 * @param nid network identifier (must be NULL-TERMINATED)
 * @param csn chunk sequence number (must be NULL-TERMINATED)
 * @param leftByte, segmentSize: the segment start at leftByte and ends segmentSize bytes after.
 * @param buffer memory area where to put the segment
 * 
 */ 
size_t retrieveSegment(
        const char* nid,char* csn,size_t leftByte, size_t segmentSize,void* buffer)
{
    size_t bytesRead=0; char* fileName; size_t fileNameSize;
    size_t bytesToRead;
    FILE *fp; //file pointer
    
    //variables to update cache_table
    long csn_num;
	CacheEntry_t* entry,tmp_entry;
	UT_string* utnid;
	char** conversion_pointer=NULL;
	
	printf("\nRequest for nid=%s, csn=%s\n",nid,csn);

	CALCULATE_FILE_NAME(nid,csn,fileName,fileNameSize);
    printf("Opening file \"%s\"\n", fileName);
    
    fp = fopen(fileName, "rb");
    if (fp == NULL) 
    {
        fprintf(stderr, "Error: Failed to open the file %s; maybe the chunk does not exist\n\n", fileName);
        return ( (size_t) - 1);
    }
    
	//!!!Questo fa andare in buffer overflow, il primo parametro dovrebbe essere
	//buffer senza +leftByte
    //Go to the leftByte location
    fseek(fp,leftByte,0);
    
    bytesRead = fread(buffer, 1, segmentSize, fp);

    if (bytesRead <= 0) 
    {
        fprintf(stderr, "Error: in reading file %s. fread returned with %u\n\n", 
                fileName,bytesRead);
        return ( (size_t) - 2);
    } 
    
	fclose(fp);
    

    printf("Successfully read  %d bytes\n", (int) bytesRead);

	
	//update table
	//if long long is needed, use strtoll(...)
	//base 16 is used because the csn string is an hexadecimal representation of chunk sequence number
	csn_num=strtol((const char*) csn,NULL,16);
	
	utstring_new(utnid);
    utstring_printf(utnid, nid );
    printf("Updating cache table. Now the chunk nid=%s, csn=%s is at the head\n",nid,csn);
    find_on_cache_table(cache_table,utnid,csn_num,entry);    
    
    return bytesRead;
}


/**
 * @param nid network identifier (must be NULL-TERMINATED)
 * @param csn chunk sequence number (must be NULL-TERMINATED)
 * @param leftByte, segmentSize: the segment start at leftByte and ends segmentSize bytes after.
 * @param buffer memory area where the segment is placed
 * 
 */ 
size_t handleSegment(char* nid,char* csn,size_t leftByte, size_t segmentSize,void* buffer)
{
    size_t writtenBytes,fileNameSize;
    char* fileName;
    char csnToDelete[64];
    FILE *fp;
    UT_string* folderName; utstring_new(folderName);
    UT_string* tempHexCsn; utstring_new(tempHexCsn);
    
    //variables for cache table
	CacheEntry_t 	*entry, *tmp_entry; 
	CacheEntry_t	*entryToDelete=NULL;
	UT_string 		*utnid, *filenameToDelete;
	long 			csnlong;
    
    
    printf("\nNew segment nid=%s, csn=%s received.\n",nid,csn);
	CALCULATE_FILE_NAME(nid,csn,fileName,fileNameSize);
    printf("Creating file \"%s\"\n", fileName);
	utstring_printf(folderName,CONTAINING_FOLDER);
	utstring_printf(folderName,nid);
	mkdir(utstring_body(folderName));
    fp = fopen(fileName, "w+b");

    if (fp == NULL) {
        fprintf(stderr, "Error: Failed to open the file %s for writing;\n\n", fileName);
        return ( (size_t) - 1);
    }

    writtenBytes = fwrite(buffer, 1, segmentSize, fp);
    fclose(fp);
    
    if (writtenBytes!=segmentSize) {
        fprintf(stderr, "Error in writing %s: writtenBytes=%d, while segmentSize=%d\n",
        		fileName,writtenBytes,segmentSize);
        return ( (size_t) - 1);
    }
    
    
    utstring_new(utnid); utstring_printf(utnid,nid);
    //utstring_printf(tempHexCsn,"0x");utstring_printf(tempHexCsn,csn);
    csnlong=strtol((const char*) csn,NULL,16);
    ADD_TO_CACHE_TABLE(cache_table,entry,tmp_entry,utnid,csnlong,0,entryToDelete);
    
    if (entryToDelete!=NULL)
    {
    	sprintf(csnToDelete,"%x",(unsigned int) entryToDelete->csn);
		CALCULATE_UTSTRING_FILE_NAME(entryToDelete->nid,csnToDelete,filenameToDelete);
		
		if (remove(utstring_body(filenameToDelete) )==0)
			printf("File %s deleted.\n",utstring_body(filenameToDelete));
		else{
			fprintf(stderr,"Error deleting file %s,\n",utstring_body(filenameToDelete));
			return -712;
		}
    }
    
    printf("New chunk added: nid=%s, csn=%ld\n",nid,csnlong);
    
//    utstring_free(utnid); utstring_free(filenameToDelete);
    
    return writtenBytes;

}


int cache_table_init()
{
	DIR *parent_dir, *child_dir;
	struct dirent *ent,*child_ent;
	const char* parent_dir_name="files/chunks/";
	UT_string* child_dir_name; 	utstring_new(child_dir_name);
	UT_string* nid; 			utstring_new(nid);
	UT_string* utfilename;		utstring_new(utfilename);
	char* csn; int csn_length;
	void* chunkBuffer; long chunkSize;
	FILE* chunkFile;
    
	printf("Itializing cache table with preexisting chunks\n");	
		
	parent_dir = opendir (parent_dir_name);
	if (parent_dir == NULL) 
	{
		/* could not open directory */
		fprintf(stderr,"Error opening directory %s\n",ent->d_name);
		perror ("Could not open directory");
		return -6187;

	}
	
		/* print all the files and directories within directory */
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
							
							printf("\n\nFound the preexisting chunk: nid=%s, csn=%s\n",utstring_body(nid),csn);
							
							RETRIEVE_THE_WHOLE_SEGMENT(
									utstring_body(nid),csn,chunkSize,utfilename,chunkFile,chunkBuffer);
							
							handleSegment( utstring_body(nid), csn, 0, (size_t)chunkSize,chunkBuffer);
					} //else do nothing because you found "." or ".." in child dir
					
				}//end of child_while
				closedir(child_dir);
				
			}//else do nothing because you found "." or ".." in parent dir
			
		}//end of parent_while
	
		closedir (parent_dir);
	
	utstring_free(child_dir_name); utstring_free(nid); utstring_free(utfilename);
	
	printf("Initialization finished\n");
}


void debug_cache_engine_run()
{
	char* nid;
	char* csn;
	size_t leftByte;
	size_t segmentSize;
	char buffer[1000];
	UT_string* utbuffer; utstring_new(utbuffer);
	void* data_buffer;

	printf("*******************************************************\nThis is a debug demo:\n************************************\n");	
	cache_table_init();
	

	//What happens if a dataCIU arrives (example1)
	nid="rametta";csn="a2";
	sprintf(buffer,"%s","asdhjgasdgajhsg sdagdga asd iasud hghdg ");
	handleSegment(nid,csn,0, strlen(buffer),buffer);
	
	//What happens if a dataCIU arrives (example2)
	nid="rametta";csn="a1";
	sprintf(buffer,"%s","uatsdhgdvausfvf sdagdga asd iasud hghdg ");
	handleSegment(nid,csn,0, strlen(buffer),buffer);

	//What happens if an interestCIU arrives (example1)
	nid="buonasera";
	csn="aa";leftByte=0;segmentSize=2;
	data_buffer=malloc(segmentSize);
	retrieveSegment(nid,csn,leftByte, segmentSize,data_buffer);
	free(data_buffer);


	//What happens if an interestCIU arrives (example2)
	nid="rametta";
	csn="a1";leftByte=0;segmentSize=18276;
	data_buffer=malloc(segmentSize);
	retrieveSegment(nid,csn,leftByte, segmentSize,data_buffer);
	free(data_buffer);


	//What happens if a dataCIU arrives (example3)
	nid="lugubre";csn="331a";
	sprintf(buffer,"%s","uatsdhgdvausfvf sdagdga asd iasud hghdg ");
	handleSegment(nid,csn,0, strlen(buffer),buffer);


}

void mainLeo()
{
	int sockfd, newsockfd, portno=9876;
	socklen_t clilen;
	char *buffer;
	chunk_id* chk;	//One chunk per time
	long tlength;
	long tcsn;
	char* tnid;

	buffer=malloc(MAX_BUFFER*sizeof(char));
	struct sockaddr_in serv_addr, cli_addr;
	int n;
	printf("Ciao Leo\n");
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
        	error("ERROR opening socket");
	bzero((char *) &serv_addr, sizeof(serv_addr));
	//portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
		error("ERROR on binding");
	listen(sockfd,10);
	clilen = sizeof(cli_addr);
	printf("Ora entro nel while\n");
	while(1){
		printf("\nSono nel while e aspetto connessioni\n");
		//printf("non accetto connessioni!\n");
		newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0) 
			error("ERROR on accept");
		//printf("Dopo l'accept e prima del bzero");
		bzero(buffer,MAX_BUFFER);
		//printf("Dopo il bzero ma prima di read");

		//printf("Per l'esattezza: %s", buffer);

//ATTENTION! look also at test.c

		//1 step
		memset(buffer, 0x00, MAX_BUFFER);
		n = read(newsockfd,buffer,MAX_BUFFER);
		if (n < 0) error("ERROR reading from socket");
		printf("\n\nHo letto qualcosa... %s", buffer);
		tlength=atol(buffer);
		printf("\nletti %d byte, valore pari a %ld", n, atol(buffer));
/*
		//2 step
		//bzero(buffer,MAX_BUFFER);
		n = read(newsockfd,buffer,MAX_BUFFER);
		if (n < 0) error("ERROR reading from socket");
		//printf("Ho letto qualcosa... ");
		tcsn=atol(buffer);
		printf("\ncsn= %ld", tcsn);

		//3 step
		//bzero(buffer,MAX_BUFFER);
		n = read(newsockfd,buffer,MAX_BUFFER);
		if (n < 0) error("ERROR reading from socket");
		//printf("Ho letto qualcosa...%d \n",n);
		tnid=malloc(tlength*sizeof(char));
		strcpy(tnid, buffer);
		printf("\nnid= %s", tnid);
		//tlength=atol(n);

		//printf("Here is the message: %ld\n",tlength);	
		//printf("Here is the message: %s\n",n);	


		//parsing(buffer, chk); impossibru
*/
		//printf("I read: \n");
		//printf("length: %ld\n", chk->nid_length);
		//printf("csn: %ld\n", chk->csn);
		//printf("nid: %s\n", chk->nid);

		//n = write(newsockfd,"I got your message",18);
		//if (n < 0) error("ERROR writing to socket");
		
		
		close(newsockfd);
	}
	//close(sockfd);
     //return 0; 
}

void parsing(char* buffer, chunk_id* chk){
	long nid_length;
	long csn;
	char* nid;
/*
	printf("Il primo byte è, %c", buffer[0]);
	char* tlen=malloc(sizeof(long));
	snprintf(tlen, sizeof(long), buffer);
	nid_length=atol(tlen);
	
	char* tcsn=malloc(sizeof(long));
	strncpy(tcsn, buffer+sizeof(long), sizeof(long));
	csn=atol(tcsn);

	strncpy(nid, buffer+2*sizeof(long), nid_length*sizeof(char));

	//nid_length=temp->nid_length;
	//csn=temp->csn;
	//char* nid=malloc(nid_length*sizeof(char));
	//nid=temp->nid;

*/	

	//strncpy(temp,buffer, sizeof(long));
	//length=strtol(temp,NULL,0);
	printf("Your length is %ld\n", nid_length);
}

