/**
 * Author: andrea.araldo@gmail.com
 */

#include <stdio.h>
#include <string.h>
#include <ccnd_private.h>

//This will be filled when setup_local_addresses(...) will be called
unsigned char * local_mac; 


int main(int argc, char** argv)
{
	//At first, I must setup my net parameters
	struct ccnd_handle fasullo; //defined in ccnd_private.h
    int my_raw_sock=setup_raw_socket_listener(); //conet_net.h
       
    fasullo.conet_raw_fd=my_raw_sock;
       
    setup_local_addresses(&fasullo);//conet_net.h
    



	printf("Hello.\n");
	unsigned long long csn=0;
	char* nid="/prova/ciaoleo";
	unsigned int l_edge=1;
	unsigned int segment_size=8;
	unsigned char segment[segment_size];
	
	int i=0;
	for (;i<segment_size;i++)
		segment[i]=0x20;
	
	char* src_addr="192.168.1.23";
	int chunk_size=(int)segment_size;
	unsigned int r_edge=l_edge+segment_size-1;
	unsigned int is_final=1; //per ora e' sempre quello finaleq
	
	//flag trasportato nell'interest per richiedere che nel data spedito sia trasportata anche la chunk size.
	unsigned int add_chunk_info=0; 

	unsigned short is_raw=1;//TODO: se lo teniamo sempre a 1 va bene?

	
	printf("I want to send a data: nid=%s, csn=%llu, l_edge=%u, r_edge=%u\n",
			nid, csn, l_edge, r_edge);
			
	
	//PLEASE, PAY ATTENTION: you must have a static variable 
	//		unsigned char * local_mac; 
	//before calling conet_send_data_cp
	int bytesSent=conet_send_data_cp(src_addr, chunk_size, segment, segment_size, nid, csn, l_edge, r_edge, is_final, add_chunk_info, is_raw);
	
	printf("\n\nSent %d bytes\n",bytesSent);
}



