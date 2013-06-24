/*
Questo file contiene una versione delle librerie di Conet in kernel space
sono impementate tutte le funzioni utili al server  
*/
#include <linux/string.h>
#include <linux/module.h>
#include <linux/kernel.h> 
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <asm/uaccess.h>
#include "kconet.h"


unsigned int inet_addr(char *str)
{
	int a,b,c,d;
	char arr[4];
	sscanf(str,"%d.%d.%d.%d",&a,&b,&c,&d);
	arr[0] = a; arr[1] = b; arr[2] = c; arr[3] = d;
	return *(unsigned int*)arr;
}

static inline int get_file_size(char *Filename)
{
	struct file *filp;
	mm_segment_t oldfs;
	int	bytesRead;

	filp = filp_open(Filename,00,O_RDONLY);
	if (IS_ERR(filp)||(filp==NULL))
		return -1;  

	if (filp->f_op->read==NULL)
		return -1;  

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	bytesRead = filp->f_op->llseek(filp,0,SEEK_END);
	set_fs(oldfs);
	fput(filp);
	return bytesRead;
}

static inline int read_cp_from_file(char *filename, char *buff, int n_bytes, int start_pos)
{
	struct file *filp;
	mm_segment_t oldfs;
	int	bytes_read;

	filp = filp_open(filename,00,O_RDONLY);
	if (IS_ERR(filp)||(filp==NULL))
		return -1;  

	if (filp->f_op->read==NULL)
		return -1;  

	filp->f_pos = start_pos;
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	bytes_read = filp->f_op->read(filp,buff,n_bytes,&filp->f_pos);
	set_fs(oldfs);
	fput(filp);
	return bytes_read;
}

static inline char* obtainNID(unsigned char** buf_iterator, unsigned short ll_flag)
{	
	unsigned short nid_length;
	char* nid=NULL;

	if(ll_flag == CONET_CIU_DEFAULT_NID_LENGTH_FLAG){
		nid_length = CONET_DEFAULT_NID_LENGTH;
		nid = kmalloc( (nid_length + 1)*sizeof(char), GFP_KERNEL);
		nid = strncpy(nid, (const char*)(*buf_iterator), nid_length);
		nid[nid_length] = '\0';
	}
	else if(ll_flag == CONET_CIU_VARIABLE_NID_LENGTH_FLAG){
		nid_length = (unsigned short) ((*buf_iterator)[0]);
		nid = kmalloc( (nid_length + 1)*sizeof(char), GFP_KERNEL);
		nid = strncpy(nid, (const char*) (*buf_iterator +1), nid_length);
		nid[nid_length] = '\0';
	}
	else{
		//TODO this case will never happen. It's controlled by the caller
		KDEBUG("[kCONET]: Ritorno null questo caso non e' ancora gestito \n");
		return NULL;
	}

	return nid;
}

static inline unsigned long long read_variable_len_number(unsigned char* buf, int* pos)
{
	unsigned long long number = 0 ;
	unsigned char* byte_scroller = buf + *pos;

	if((*byte_scroller | 127) == 127) { //number length: 1byte (1bit pattern) |0XXXXXXX|
		number = (unsigned long long)(*byte_scroller); //the pattern was 0XXXXXXX so we don't worry about the sign
		*pos +=1;
	}
	else if((*byte_scroller & 192) == 128) { //number length: 2bytes (2bits pattern) |10XXXXXX|XXXXXXXX|
		unsigned char value[2] = {byte_scroller[0] & 127, byte_scroller[1]}; //return the value in network byte order without the pattern byte(2-bit pattern plus remaining 6-bit effective value) in first position
		number = (unsigned long long)ntohs(*((unsigned short*)value));
		*pos += 2;
	}
	else if((*byte_scroller & 224) == 192) { //number length: 3bytes (3bits pattern) |110XXXXX|XXXXXXXX|XXXXXXXX|
		unsigned int ret = 0; //clean memory
		unsigned char* ret_scroller = (unsigned char*)(&ret);
		ret_scroller[1] = byte_scroller[0] & 31;
		ret_scroller[2] = byte_scroller[1];
		ret_scroller[3] = byte_scroller[2];
		number = (unsigned long long)ntohl(ret);
		*pos += 3;
	}
	else if((*byte_scroller & 240) == 224) { //number length: 4byte (4bit pattern) |1110XXXX|XXXXXXXX|XXXXXXXX|XXXXXXXX|
		//return the value in network byte order with the pattern byte(3-bit pattern plus remaining 5-bit effective value) in first position
		unsigned int ret = *((unsigned int*)byte_scroller);
		unsigned char* ret_scroller = (unsigned char*)(&ret);
		//return the network byte order but without the 4 initial bit pattern
		ret_scroller[0] = byte_scroller[0] & 15;
		number = (unsigned long long)ntohl(ret);
		*pos += 4;
	}
	else if((*byte_scroller & 255) == 240) { //number length: 5bytes (8bits pattern) |11110000|XXXXXXXX|XXXXXXXX|XXXXXXXX|
		//TODO
		*pos += 5;
		KDEBUG("[kCONET]: Ritorno -1 size not yet supported for this len number \n");
		return -1;
	}
	else if((*byte_scroller & 255) == 241) {//number length: 6bytes (8bits pattern) |11110001|XXXXXXXX|XXXXXXXX|XXXXXXXX|
		//TODO
		*pos += 6;
		KDEBUG("[kCONET]: Ritorno -1 size not yet supported for this len number\n");
		return -1;
	}

	return number;
}

// qui devo togliere le realloc alloco il pachetto una volta per tutte

static inline unsigned int write_variable_len_number(unsigned char** buf, size_t bufsize, unsigned long long number)
{
	unsigned char *packet=*buf;

	if(number<128) {
		//take only the byte of chunk_number setted to a value different from 0: htons((unsigned short)chunk_number)= |0xxxxxxx|00000000|
		packet[bufsize] = (unsigned char)number;
		bufsize +=1;
	}
	else if(number < 16384 ) { //2^14
		//TODO taking this value back in the process_input use a short variable!
		unsigned short* num = (unsigned short*)(packet+bufsize);
		bufsize +=2;
		*num = htons((unsigned short)(number | 32768)) ; //10XXXXXX XXXXXXXX pattern
	}
	else if(number < 2097152) { // 2^21
		unsigned int num = htonl((unsigned int)(number | 12582912)); //110XXXXX XXXXXXXX XXXXXXXX pattern
		char* num_iterator = (char*)&num;
		//position 0 in csn is the most significant byte and it is 0 for the number in use here
		packet[bufsize]=num_iterator[1];
		packet[bufsize+1]=num_iterator[2];
		packet[bufsize+2]=num_iterator[3];
		bufsize +=3;
	}
	else if(number < 268435456U) {
		unsigned int* num = (unsigned int*)packet+bufsize;
		*num = htonl((unsigned int)(number | 3758096384U)); // 1110XXXX XXXXXXXX XXXXXXXX XXXXXXXX pattern
		bufsize += 4;
	}
	else if(number < 2147483648U ) { // 32 bit field   11110000 XXXXXXXX XXXXXXXX XXXXXXXX pattern
		//TODO
		KDEBUG(" Ritorno -1 size not yet supported for this len number \n");
		return -1;
	}
	else if(number < 549755813888U) { // 40 bit field   11110001 XXXXXXXX XXXXXXXX XXXXXXXX pattern
		//TODO
		KDEBUG(" Ritorno -1 size not yet supported for this len number \n");
		return -1;
	}

	*buf = packet;
	return bufsize;
}

static inline unsigned int build_carrier_packet(unsigned char** buffer, int ciu_type, int cache, const char * nid, unsigned long long chunk_number, unsigned int left_edge, unsigned int right_edge, unsigned short cp_flags, int chunk_size)
{ 
	unsigned char* packet=*buffer;
	unsigned char* hdrptr = packet;
	int nid_len = strlen(nid);
 	int ll_flag=0;
	int packet_size =0;

	if (nid_len!=16) {
		ll_flag=2;
	}

	packet[0] = 0;
	packet[0] = (unsigned char)ciu_type<<4 | (unsigned char)ll_flag<<2 | (unsigned char)cache<<1;//the cast to (unsigned char) is used to support both big and little endian (NB:the shift operation also reflects the endianess)
	packet_size +=1;

	if (ll_flag==0) {
		packet_size += strlen(nid);
		hdrptr = packet+1;
	}
	else {
		packet_size += strlen(nid) +1 ;
		hdrptr=packet+1;
		hdrptr[0]=(unsigned char)(strlen(nid));
		hdrptr++;
	}

	memcpy(hdrptr, nid, strlen(nid));
	hdrptr+=strlen(nid);
	packet_size = write_variable_len_number(&packet, packet_size, chunk_number);

	if (ciu_type==CONET_NAMED_DATA_CIU_TYPE_FLAG) {
		packet[packet_size]=(unsigned char) cp_flags;
		packet_size++;
	}
	else {
		packet[packet_size]=CONET_PREFETCH <<4;
		if (chunk_size==0) {
			packet[packet_size]|=CONET_ASK_CHUNK_INFO_FLAG;
		}
		packet_size++;
	}

	packet_size = write_variable_len_number(&packet, packet_size, left_edge);
	packet_size = write_variable_len_number(&packet, packet_size, right_edge);

	if(ciu_type==CONET_NAMED_DATA_CIU_TYPE_FLAG && (cp_flags >> 4) == CONET_FOLLOW_VAR_CHUNK_SIZE) {
		packet_size = 
			write_variable_len_number(&packet, packet_size, (unsigned long long)chunk_size);
	}

	*buffer = packet;
	return packet_size;
}


static inline char* conet_create_data_cp(/*char* src_addr, */int chunk_size, char * segment,
		unsigned int segment_size, const char* nid, unsigned long long csn,
		unsigned int l_edge, unsigned int r_edge, unsigned int is_final,
		unsigned int add_chunk_info, int *cp_size/*, unsigned short use_raw*/) 
{
	unsigned char* packet = NULL;
	unsigned char cp_flag = CONET_CONTINUATION << 4;
	unsigned int pckt_size;
	// unsigned int ipoption_size;
	// size_t packet_size;

	/*setting-up flags*/
	if (add_chunk_info == 1) {
		//first data-CP response
		//TODO vedere se potenza di 2->codificare con enum altrimenti segue...
		cp_flag = CONET_FOLLOW_VAR_CHUNK_SIZE << 4;
	}
	if (is_final == 1) {
		cp_flag = cp_flag | CONET_FINAL_SEGMENT_FLAG;
	}

	packet = kmalloc( CONET_DEFAULT_MTU*sizeof(char), GFP_KERNEL); //alloco diretto tutto lo spazio per il pacchetto
	if (packet==NULL){
		KDEBUG("[kCONET]: kmalloc fallita");
		return NULL;
	} 

/*
	if (use_raw) {
		pckt_size = build_ip_option(&packet, CONET_NAMED_DATA_CIU_TYPE_FLAG,
				CONET_CIU_CACHE_FLAG, nid, csn, AF_INET);
		ipoption_size = pckt_size;
		// if (CONET_DEBUG >= 2)
		// 	fprintf(stderr,
		// 			"!!![conet.c: %d]: sto chiamando set_open_flow_tag",
		// 			__LINE__);//&CT
#ifdef IPOPT
		pckt_size = set_openflow_tag(&packet, pckt_size,
				CONET_NAMED_DATA_CIU_TYPE_FLAG, nid, csn);
#endif
		pckt_size = build_cp(&packet, pckt_size,
				CONET_NAMED_DATA_CIU_TYPE_FLAG, l_edge, r_edge, cp_flag,
				chunk_size);
		if (pckt_size + segment_size > CONET_DEFAULT_MTU) {
			int diff = (pckt_size + segment_size) - CONET_DEFAULT_MTU;
			r_edge = r_edge - diff;
			segment_size = r_edge - l_edge + 1;
			if (packet != NULL) {
				free(packet);
			}
			packet = NULL;
			//define che si puo togliere nho
			//inserire parametro per distinguere famiglia
			pckt_size = build_ip_option(&packet,
					CONET_NAMED_DATA_CIU_TYPE_FLAG, CONET_CIU_CACHE_FLAG, nid,
					csn, AF_INET);
#ifdef IPOPT
			pckt_size = set_openflow_tag(&packet, pckt_size,
					CONET_NAMED_DATA_CIU_TYPE_FLAG, nid, csn);
#endif
			pckt_size = build_cp(&packet, pckt_size,
					CONET_NAMED_DATA_CIU_TYPE_FLAG, l_edge, r_edge, cp_flag,
					chunk_size);
		}
		packet = realloc(packet, pckt_size + segment_size);
		memcpy(packet + pckt_size, segment, segment_size);
	} else { 
*/	
	pckt_size = build_carrier_packet(&packet,
				CONET_NAMED_DATA_CIU_TYPE_FLAG, CONET_CIU_CACHE_FLAG, nid, csn,
				l_edge, r_edge, cp_flag, chunk_size); //TODO choose yes/no-CACHE
		//if packt_size is over the MTU refresh the packet with a reduced segment

	if (pckt_size + segment_size > CONET_DEFAULT_MTU) {
		int diff = (pckt_size + segment_size) - CONET_DEFAULT_MTU;
		r_edge = r_edge - diff;
		segment_size = r_edge - l_edge + 1;
		// if (packet != NULL) {
		// 	kfree(packet);
		// }
		// packet = NULL;
		pckt_size = build_carrier_packet(&packet,
				CONET_NAMED_DATA_CIU_TYPE_FLAG, CONET_CIU_CACHE_FLAG, nid,
				csn, l_edge, r_edge, cp_flag, chunk_size); 
	}
	memcpy(packet + pckt_size, segment, segment_size);
	// fine raw}
	// packet_size = (size_t) (pckt_size + segment_size);
	*cp_size = (pckt_size + segment_size);

	KDEBUG("[kCONET]: Sending an interest for %s/%llu left edge:%u, right edge:%u \n", nid, csn, l_edge, r_edge);
	kfree(segment);
	return packet; //ritorno il pacchetto creato
}


inline char*  conet_process_interest_cp(/*char *src_addr,*/
		unsigned char* readbuf,int *conet_payload_size, unsigned short ll, unsigned short c,
		unsigned short ask_info  /* ,unsigned short is_raw*/) 
{ 
	char* nid;
	unsigned short nid_length;
	unsigned char* iterator;
	unsigned int is_final = 0;
	int pos;
	unsigned long long csn;
	unsigned char cp_flag;
	unsigned int l_edge;
	unsigned int r_edge;
	char *cp = NULL;
	char *uri;
	char *filename;
	char *conet_payload = NULL;
	int segment_size;
	int cp_size = 0 ;
	int chunk_size;

	if (c == CONET_CIU_NOCACHE_FLAG) {
		//simply forward the interest without further analyze
		return NULL;
	}
	//iterator on the first byte of NID field
	/* //isolo raw
	if (is_raw)
		iterator = readbuf + 1 + 2; //ip_opt len +flag + DS&T field
	else 
	*/
	iterator = readbuf + 1;//caso udp da rivedere
	nid = obtainNID(&iterator, ll);
	KDEBUG("[kCONET]: NID: %s \n",nid);
	nid_length = strlen(nid);

	// }

	if (ll == 0) {
		//iterator on the first byte of CSN field
		iterator = iterator + nid_length;
	} else if (ll == 2) {
		//iterator on the first byte of CSN field
		iterator = iterator + 1 + nid_length;
	} else {
		KDEBUG("[kCONET]: Using reserved ll-flag in input message. Not yet supported\n");
		kfree(nid);
		return NULL;	
	}

	pos = iterator - readbuf;
	csn = read_variable_len_number(readbuf, &pos);

	// KDEBUG("[kCONET]:After reading csn=%llu pos=%d and next bytes are %2X %2X %2X %2X\n", csn, pos, readbuf[pos + 0], readbuf[pos + 1], readbuf[pos+ 2], readbuf[pos + 3]);

	//Skip the "End of ip option list" character
/*	if (is_raw) //da controllare isolo raw
		pos = (int) readbuf[0] - 1; // - ip_opt len

#ifdef IPOPT
	if (is_raw) {
		pos += 4; //jump 4 byte openflow tag
	}
#endif
*/
	cp_flag = (unsigned char) *(readbuf + pos); 
	pos++;
	
	if ((cp_flag & CONET_ASK_CHUNK_INFO_FLAG) != 0) {
		ask_info = 1;
	}

	l_edge = read_variable_len_number(readbuf, &pos);
	r_edge = read_variable_len_number(readbuf, &pos);

	KDEBUG("[kCONET]: Received an interest for %s/%llu left edge:%u, right edge:%u \n", nid, csn, l_edge, r_edge);
	
	uri = kmalloc( URI_LEN*sizeof(char), GFP_KERNEL);
	if (uri==NULL) {
		KDEBUG("[kCONET]: kmalloc fallita");
		goto end;
	} 
	sprintf(uri, "%s/%llu", nid, csn);

	filename = kmalloc( URI_LEN*sizeof(char), GFP_KERNEL);
	if (filename==NULL) {
		KDEBUG("[kCONET]: kmalloc fallita");
		goto end;
	} 
	//convertire la nid in un nomefile per adesso si risponde sempre con lo stesso file 
	sprintf(filename, "%s_%llu", FILE_NAME, csn);

	// da qui in poi becco il contenutto 
	chunk_size = get_file_size(filename);
	if (chunk_size == -1 || chunk_size == 0) {
		KDEBUG("[kCONET]:file not found. Drop\n");
		goto end;
	}

	//leggo il payload del cp dal file 
	segment_size = r_edge - l_edge + 1;
	conet_payload = kmalloc( (segment_size+1)*sizeof(char), GFP_KERNEL); 
	if (conet_payload==NULL){
		KDEBUG("[kCONET]: kmalloc fallita");
		goto end;
	} 
	
	*conet_payload_size = read_cp_from_file(filename, conet_payload, segment_size, l_edge);
	
	if (*conet_payload_size == -1 || *conet_payload_size == 0) {
		KDEBUG("[kCONET]:file not found. Drop\n");
		goto end;
	}

	KDEBUG("[kCONET]: bytes letti %d dal file %s \n",*conet_payload_size, filename);
	KDEBUG("[kCONET]: l_edge %u segment_size %d\n", l_edge, segment_size);
	
	if (*conet_payload_size < segment_size) {
		//end of chunk with a dimension different from a multiple of segment-dimension reached
		if (*conet_payload_size == 0) {
			r_edge = 0;
		} else {
			KDEBUG("[kCONET]: The segment is final\n");
			r_edge = *conet_payload_size + l_edge - 1;  
			is_final = 1;
		}
		segment_size = r_edge - l_edge + 1;
	}

	// if (*conet_payload_size - (int) l_edge < 0) {
	// 	KDEBUG("[kCONET]: The segment is shorter than this request. Drop\n");
	// 	return NULL;
	// }	

	cp=conet_create_data_cp(/*src_addr,*/ chunk_size, conet_payload, *conet_payload_size,nid,csn,l_edge,r_edge,is_final,ask_info,&cp_size);
    *conet_payload_size = cp_size;
end:	
	kfree(uri);
	kfree(nid);
	return cp;
}