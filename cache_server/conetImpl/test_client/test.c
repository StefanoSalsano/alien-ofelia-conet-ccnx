#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <netinet/in.h>
#include <netdb.h>

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

int main(int argc, char** argv){
	int sockfd, newsockfd, portno=9876;
	struct hostent *hp;	
	char dir[100]; //Response
	//if (argc<1)
	//	error("ERROR: insert name and csn");	
	if ((hp = gethostbyname("localhost")) == 0)
		error("gethostbyname");
	long tlength;
	long tcsn;
	char* tnid;
	//tlength=44;
	tcsn=123;
	char* prova="Adesso invece provo a mandare una stringa diversa! his is the representation of the data bytes of a nid";
	tlength=(long)strlen(prova);

	tnid=malloc(tlength*sizeof(char));
	strcpy(tnid, "poi");

	printf("Sending: \n");
	printf("length: %ld\n", tlength);
	printf("csn: %ld\n", tcsn);
	printf("nid: %s\n", tnid);


	chunk_id* msg = malloc(sizeof(chunk_id));
	//printf("Il fault è qui!\n", tnid);
	msg->nid_length=tlength;
	//printf("E non qui\n", tnid);
	msg->csn=tcsn;
	//sprintf(msg->nid);
	//printf("Prima di string copy\n", tnid);

	//strcpy(msg->nid, "buonasera");	
	msg->nid=tnid;	



	struct sockaddr_in serv_addr, cli_addr;


	int n;
	printf("Ciao Leo, ma sono io?\n");
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
        	error("ERROR opening socket");
	printf("client=%d\n",sockfd);

	memset(&serv_addr, 0, sizeof(serv_addr));	
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_port=htons(portno);
	memcpy(&serv_addr.sin_addr, gethostbyname("localhost")->h_addr, sizeof(serv_addr.sin_addr));

	if (connect(sockfd,(struct sockaddr *)  &serv_addr, sizeof(serv_addr)) == -1)
		error("errore nella connessione");

	//printf("Qua non lo so");
	//strcpy(msg, "ciao mbare");


	char *sending=malloc(sizeof(long));
	sprintf(sending, "%ld", tlength);
	//write(sockfd, (void *), sizeof(tlength));
	write(sockfd, (void*)sending, sizeof(long));
	sleep(1);

	bzero(sending,sizeof(long));
	sprintf(sending, "%ld", tcsn);
	write(sockfd, (void *)sending, sizeof(long));
	sleep(1);
	write(sockfd, (void *)prova, tlength*sizeof(char));


	//printf("QUA NO SICURO", tnid);
       //if (recv(sockfd, dir, 100, 0) == -1)
       //         error("recv");


	printf("close=%d\n",close(sockfd));
}
