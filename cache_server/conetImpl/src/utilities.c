#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */


/**
 * author: andrea.araldo@gmail.com
 * inspired by:
 * http://cs.baylor.edu/~donahoo/practical/CSockets/code/TCPEchoClient.c
 */
int create_connection(char* ip_address, unsigned short port)
{
	int sock;
	struct sockaddr_in destAddr; /* destination address structure*/
    char *controllerIPaddr;                    /*  (dotted quad) */
        
    /* Create a reliable, stream socket using TCP */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        fprintf(stderr,"socket() to controller failed\n");
        return -1;
    }
        
    /* Construct the controller address structure */
    memset(&destAddr, 0, sizeof(destAddr));     /* Zero out structure */
    destAddr.sin_family      = AF_INET;             /* Internet address family */
    destAddr.sin_addr.s_addr = inet_addr(ip_address);   /* controller IP address */
    destAddr.sin_port        = htons(port);
    
    /* Establish the connection to the controller */
    if (connect(sock, (struct sockaddr *) &destAddr, sizeof(destAddr)) < 0)
    {
        error("connect() to controller failed\n");
        return -2;
    }

	return sock;
	/*
    char *msg;

	msg="Ciao sono io, ma pure no";        
    // Send the string to the controller
    if (send(sock, msg, strlen(msg), 0) != strlen(msg))
        fprintf(stderr,"send() sent a different number of bytes than expected\n");
    */
}


/**
 * Replace any '\' with '_'
 */
 int escape_nid(char* nid, long nid_length)
 {
 	int i;
 	for (i=0; i<nid_length;i++)
 		if (nid[i]=='/')
 			nid[i]='_';
 	
 }
 
 
 /**
 * Replace any '_' with '/'
 */
 int reverse_escape_nid(char* nid, long nid_length)
 {
 	int i;
 	for (i=0; i<nid_length;i++)
 		if (nid[i]=='_')
 			nid[i]='/';
 }
 
 
  /**
  * @param byte_array must be an array of four unsigned char
  */
 unsigned int from_byte_array_to_numberVECCHIO(unsigned char* byte_array)
 {
 	unsigned int number=0;
 	unsigned char* pointer=(unsigned char*)&number;
 	pointer+=sizeof(number)-4;
	memcpy(pointer,byte_array,4);
 	return number;
 }

  /**
  * @param byte_array must be an array of four unsigned char
  */
 unsigned int from_byte_array_to_number(unsigned char* byte_array)
 {
        unsigned int number=0;
        unsigned char* pointer=(unsigned char*)&number;
        pointer+=sizeof(number)-4;
	int i;
	for (i=0; i<4; i++)
		pointer[i]=byte_array[3-i];
        return number;
 }

