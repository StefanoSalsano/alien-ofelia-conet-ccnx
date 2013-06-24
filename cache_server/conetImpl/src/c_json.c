#include "c_json.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <netinet/in.h>
#include <netdb.h>

void fill_message(long tag, char *type, char* nid, unsigned long long csn, char *message)
{
	memset(message,'\0',BUFFERLEN);
	sprintf(message,"{\"CONTENT NAME\":\"%ld\",\"type\":\"%s\",\"nid\":\"%s\",\"csn\":\"%llu\"}",
			tag,type,nid,csn);
/*
	sprintf(message,"{\"CONTENT NAME\":");
	
	
	UT_string* temp;

	utstring_new(temp);
	utstring_printf(temp,"\"%ld\",", tag);
	
	strcat(message,utstring_body(temp));
	
	strcat(message,"\"type\":");
	
	utstring_clear(temp);
	utstring_printf(temp,"\"%s\"}", type);
	strcat(message,utstring_body(temp));
	
	utstring_free(temp);
*/
}

/*
void json_messageOld(long tag, char *type, char **message)
{
	char* initial_string="{\"CONTENT NAME\":";
	char* content_string=malloc(100);
	sprintf(content_string, "\"%ld\"", tag);

    	size_t message_len = strlen(initial_string) + 1; // + 1 for terminating NULL
		*message = (char*) malloc(message_len);
    	strncat(*message, initial_string, message_len);

	message_len +=strlen(content_string); // the content name
	*message = (char*) realloc(*message, message_len);
	strncat(*message, content_string, message_len);

	char* action=malloc(100*sizeof(char));
	sprintf(action, "\"type\":\"%s\"}", type);

	message_len += 1 + strlen(action); // 1 + for separator ','
	*message = (char*) realloc(*message, message_len);
    strncat(strncat(*message, ",", message_len), action, message_len);
}
*/


/*
int mainProva(int argc, char** argv){

	//this is a simple test-main
	int sockfd, newsockfd, portno=9999;
	struct hostent *hp;	
	char dir[100]; //Response, if any
	if ((hp = gethostbyname("localhost")) == 0)
		error("gethostbyname");
	
	//Preparation section
	struct sockaddr_in serv_addr, cli_addr;
	int n;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
        	error("ERROR opening socket");
	printf("client=%d\n",sockfd);

	//Data and addresses preparation
	memset(&serv_addr, 0, sizeof(serv_addr));	
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_port=htons(portno);
	memcpy(&serv_addr.sin_addr, gethostbyname("localhost")->h_addr, sizeof(serv_addr.sin_addr));

	//Connect
	if (connect(sockfd,(struct sockaddr *)  &serv_addr, sizeof(serv_addr)) == -1)
		error("errore nella connessione");

	//Example:	
	long nid=32617126;
	char *message;


	json_message(nid, "stored", &message);	//creates json message with nid and of type "stored", and store it to message;
	puts(message);				//print the result
	write(sockfd, (void*)message, strlen(message));	//write to the server
	getchar();					//pause


	json_message(321678, "deleted",&message);
	puts(message);
	write(sockfd, (void*)message, strlen(message));
	getchar();

	json_message(213, "stored", &message);
	puts(message);
	write(sockfd, (void*)message, strlen(message));
	getchar();

	//bzero(message, sizeof(message));

	json_message(61278, "deleted", &message);
	write(sockfd, (void*)message, strlen(message));
	puts(message);
	getchar();

	printf("close=%d\n",close(sockfd));
			

}
*/

//void concatena(const char *header, char **words, size_t num_words)
