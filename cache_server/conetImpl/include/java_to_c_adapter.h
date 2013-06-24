//FORSE NON PIU' USATO
/* 
 * File:   cacheEngine.h
 * Author: andrea.araldo@gmail.com
 *
 * Created on January 24, 2012, 9:41 PM
 */

#ifndef JAVA_TO_C_ADAPTER_H
#define	JAVA_TO_C_ADAPTER_H

#ifdef	__cplusplus
extern "C" {
#endif


#ifdef	__cplusplus
}
#endif

#endif	/* JAVA_TO_C_ADAPTER_H */



#include <stddef.h>    /* offsetof     */
#include <string.h>    /* memcpy     */
#include <stdlib.h>    /* malloc     */
#include <stdio.h>    /* printf     */


#define INTEREST_MSG 'i'
#define DATA_MSG 'd'

typedef struct msg_type
{
	char type; //could be INTEREST_MSG or DATA_MSG
	long nid_length;
	long csn;
	long tag;
	long left_byte;
	long segment_size;
	unsigned char flags;
	char nid[];
	
	//char data[]
}msg_t;

/**
 * Expected types
 * long my_nid_length
 * long my_csn
 * long tag
 * long my_left_byte
 * long segment_size
 * char[] nid //must be allocated
 * void* data //must be allocated
 */
#define FILL_MSG_T(nid,csn,tag,filename,fileNameSize)		

/**
 * Expected types
 * msg_t msg //must be filled
 */
#define CALCULATE_MSG_LENGTH(msg) \
		sizeof(msg_t)+ msg->nid_length + msg->segment_size		


void doveIniziaIlDato(msg_t* msg);
