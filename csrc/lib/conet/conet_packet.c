#include <conet/conet.h>
#include <conet/conet_packet.h>
#include <ccn/digest.h>

//TODO maybe right and left edge can be unsigned short

unsigned int build_carrier_packet(unsigned char** buffer, int buffer_size, int ciu_type, int cache, const char * nid, unsigned long long chunk_number, unsigned int left_edge, unsigned int right_edge, unsigned short cp_flags, int chunk_size){ //nid must be null-terminated
	if (buffer == NULL || buffer_size == 0)
		*buffer = calloc(1, sizeof(unsigned char));
	unsigned char* packet=*buffer;
	int packet_size =buffer_size;
	if (buffer_size <= 4) { //non entro qui se uso l'ip option (20 bytes), invece entro sia se uso il tag sia se no lo uso
		unsigned char* hdrptr = packet + buffer_size;
		int nid_len = strlen(nid);

		int ll_flag = 0;
		if (nid_len != 16) {
			ll_flag = 2;
		}

		packet[packet_size] = 0;
		packet[packet_size] = (unsigned char) ciu_type << 4
				| (unsigned char) ll_flag << 2 | (unsigned char) cache << 1;//the cast to (unsigned char) is used to support both big and little endian (NB:the shift operation also reflects the endianess)
		packet_size += 1;

		if (ll_flag == 0) {
			packet = realloc(packet, packet_size + strlen(nid));
			packet_size += strlen(nid);
			hdrptr = packet + 1 + buffer_size;
		} else {
			packet = realloc(packet, packet_size + strlen(nid) + 1);
			packet_size += strlen(nid) + 1;
			hdrptr = packet + 1 + buffer_size;
			hdrptr[0] = (unsigned char) (strlen(nid));
			hdrptr++;
		}

		memcpy(hdrptr, nid, strlen(nid));
		hdrptr += strlen(nid);
		packet_size = write_variable_len_number(&packet, packet_size,
				chunk_number);

		//fprintf(stderr, "[CONET]: left_edge: %d\n", left_edge);
		//fprintf(stderr, "[CONET]: right_edge: %d\n", right_edge);

		//cp flag
	}
	if (ciu_type==CONET_NAMED_DATA_CIU_TYPE_FLAG){
		packet=realloc(packet, packet_size +1);
		packet[packet_size]=(unsigned char) cp_flags;
		packet_size++;
	}
	else {
		packet = realloc(packet, packet_size +1);

		packet[packet_size]=CONET_PREFETCH <<4;
		if (chunk_size==0){
			packet[packet_size]|=CONET_ASK_CHUNK_INFO_FLAG;
		}
		packet_size++;

	}
	packet_size = write_variable_len_number(&packet, packet_size, left_edge);
	packet_size = write_variable_len_number(&packet, packet_size, right_edge);

	if(ciu_type==CONET_NAMED_DATA_CIU_TYPE_FLAG && (cp_flags >> 4) == CONET_FOLLOW_VAR_CHUNK_SIZE){
		packet_size =
			write_variable_len_number(&packet, packet_size, (unsigned long long)chunk_size);
	}

	if (CONET_DEBUG >= 2)
		fprintf(stderr, "[conet_packet.c:%d]: cp_flags=%hu, packet[packet_size-1]=%x \n",
			__LINE__, cp_flags,packet[packet_size-1]);

	*buffer = packet;

	return packet_size;
}
unsigned int build_ip_option(unsigned char** buffer, int ciu_type, int cache, const char * nid, unsigned long long chunk_number, int family){
	//unsigned int build_ip_option(unsigned char** buffer, int ciu_type, int cache, const char * nid, unsigned long long chunk_number){

	//fprintf(stderr, "[CONET]: ...calling build_carrier_packet()...\n");
	*buffer = calloc(4, sizeof(unsigned char));
	unsigned char* packet = *buffer;
	unsigned char* hdrptr = packet;
	int nid_len = strlen(nid);
	int ll_flag = 0;
	int packet_size = 0;
	if (nid_len != 16) {
		ll_flag = 2;
	}

	if (family == AF_INET6)
		packet[0] = CONET_IPV6_OPTION_CODE; //TODO ip option type check this
	else
		packet[0] = CONET_IP_OPTION_CODE;

	packet[1] = 0; //ip option len updated later
	packet[2] = 0;
	packet[2] = (unsigned char) ciu_type << 4 | (unsigned char) ll_flag << 2
			| (unsigned char) cache << 1;//the cast to (unsigned char) is used to support both big and little endian (NB:the shift operation also reflects the endianess)
	packet[3] = 0;
	packet_size += 4;

	if (CONET_DEBUG >= 2)
		fprintf(stderr, "[conet_packet.c:%d]: ipv4 flags: %d\n", __LINE__,
				packet[2] & 0xFF);
	if (ll_flag == 0) {
		int toalloc = strlen(nid);
		packet = realloc(packet, packet_size + toalloc);
		packet_size += toalloc;
		hdrptr = packet + 4;
	} else {
		int toalloc = strlen(nid) + 1;
		packet = realloc(packet, packet_size + toalloc);
		packet_size += toalloc;
		hdrptr = packet + 4;
		hdrptr[0] = (unsigned char) (strlen(nid));
		hdrptr++;
	}
	memcpy(hdrptr, nid, strlen(nid));

	//fprintf(stderr, "[CONET]: nid: %s\n", hdrptr);

	hdrptr += strlen(nid);

	packet_size = write_variable_len_number(&packet, packet_size, chunk_number);

	//fprintf(stderr, "[CONET]: left_edge: %d\n", left_edge);
	//fprintf(stderr, "[CONET]: right_edge: %d\n", right_edge);

	//cp flag
	if (packet_size % 4 != 0) {
		packet = realloc(packet, packet_size + (4 - packet_size % 4));

		int i = 0;
		for (i = 0; i < (4 - packet_size % 4); i++) {
			packet[packet_size + i] = 0x00;
		}
		packet_size = packet_size + (4 - packet_size % 4);
	}
	packet[1] = (unsigned char) packet_size;

	*buffer = packet;
	return packet_size;
}

unsigned int set_openflow_tag(unsigned char** buffer, unsigned int packet_size,int ciu_type, 
	const char * nid, unsigned long long csn)
{
	
	//forced to INTEREST as we do not differentiate anymore
	ciu_type=CONET_INTEREST_CIU_TYPE_FLAG;//&ct
	
	//TODO: more elastic size
	char* digest_base = malloc (255*sizeof(char));
	memset(digest_base,'\0',255);
	sprintf(digest_base, "%s_%llu",nid,csn);

	if (buffer == NULL || packet_size == 0)
		*buffer = calloc(1, sizeof(unsigned char));

	unsigned char* packet=*buffer;
	packet=realloc(packet, packet_size +4);
	struct ccn_digest* d;
	d = ccn_digest_create(CCN_DIGEST_SHA256);
	ccn_digest_init(d);
	int res=0;
	//d->sz=4;
	res = ccn_digest_update(d, digest_base, strlen(digest_base));
	if (res < 0) abort();
	unsigned char* hash=calloc(1, 32);
	res = ccn_digest_final(d, hash,32);
	if (res < 0) abort();

	if (ciu_type==CONET_INTEREST_CIU_TYPE_FLAG){
		if ((hash[3] & 1) ==0){
			hash[3]++;
		}

	}
	else {
		if ((hash[3] & 1) ==1){
			hash[3]--;
		}
	}

	//TODO: REMOVE this
//	if ( hash[0]=='\0' )
		hash[0]=0x33;

	ccn_digest_destroy(&d);
	packet[packet_size]=hash[0];
	packet[packet_size+1]=hash[1];
	packet[packet_size+2]=hash[2];
	packet[packet_size+3]=hash[3];
	//	*tag_ptr=htonl(nid_hash);

	//&ct: inizio
	if (CONET_DEBUG >= 2) {
		fprintf(stderr, "\n[conet_packet.c: %d] tag=",__LINE__);
        int i;
        for(i=0; i<4; i++)
        	fprintf(stderr, "%2X  ",packet[packet_size+i]);
        fprintf(stderr, "\n");
	}
	//&ct: fine

	
	*buffer=packet;
	free(digest_base);//whr
	free(hash);//whr
	return packet_size+4;
}

char* obtainNID(unsigned char** buf_iterator, unsigned short ll_flag){
	//TODO check the process below comparing the cp building

	unsigned short nid_length;
	char* nid=NULL;

	if(ll_flag == CONET_CIU_DEFAULT_NID_LENGTH_FLAG){
		nid_length = CONET_DEFAULT_NID_LENGTH;
		nid = calloc( nid_length + 1, sizeof(char));
		nid = strncpy(nid, (const char*)(*buf_iterator), nid_length);
		nid[nid_length] = '\0';
	}
	else if(ll_flag == CONET_CIU_VARIABLE_NID_LENGTH_FLAG){
		nid_length = (unsigned short) ((*buf_iterator)[0]);
		nid = calloc( nid_length+ 1, sizeof(char));
		nid = strncpy(nid, (const char*) (*buf_iterator +1), nid_length);
		nid[nid_length] = '\0';
	}
	else{
		//TODO this case will never happen. It's controlled by the caller
		fprintf(stderr,"[CONET]: reserved coding for ll-flag in obtainNID(). Aborting...\n");
		abort();
	}

	return nid;
}

/*
unsigned long long obtainCSN(unsigned char** buf_iterator){

	unsigned long long csn;
	unsigned char* byte_scroller = (unsigned char*)(*buf_iterator);

	if((*byte_scroller | 127) == 127){ //1byte CSN (1bit pattern)
		const char value[2] = {*byte_scroller, '\0'};
		csn = (unsigned long long)atoi(value); //the pattern was 0xxxxxxx so we don't worry about the sign
	}
	else if((*byte_scroller & 192) == 128){ //2bytes CSN (2bits pattern)
		unsigned short ret = *((unsigned short*)byte_scroller); //return the value in network byte order with the pattern byte(2-bit pattern plus remaining 6-bit effective value) in first position
		ret = ret << 2;
		ret = ret >> 2;//csn is in network byte order but without the 2 initial bit pattern
		csn = (unsigned long long)ntohs(ret);
	}
	else if((*byte_scroller & 224) == 192){ //3bytes CSN (3bits pattern)
		unsigned int ret = 0; //clean memory
		unsigned char* ret_scroller = (unsigned char*)(&ret);
		ret_scroller[0] = byte_scroller[0] & 31;
		ret_scroller[1] = byte_scroller[1];
		ret_scroller[2] = byte_scroller[2];
		csn = (unsigned long long)ntohl(ret);
	}
	else if((*byte_scroller & 224) == 224){ //4byte CSN (3bit pattern)
		unsigned int ret = *((unsigned int*)byte_scroller); //return the value in network byte order with the pattern byte(3-bit pattern plus remaining 5-bit effective value) in first position
		ret = ret << 3;
		ret = ret >> 3; //csn is in network byte order but without the 3 initial bit pattern
		csn = (unsigned long long)ntohl(ret);
	}
	else if(*((unsigned short*)byte_scroller) == 240){ //5bytes CSN (8bits pattern)

	}
	else{ //  *((unsigned short*)byte_scroller) == 241 //6bytes CSN (8bits pattern)

	}

	return csn;
}
 */

unsigned int write_variable_len_number(unsigned char** buf, size_t bufsize, unsigned long long number){

	unsigned char *packet=*buf;

	if(number<128)
	{
		packet = realloc(packet, bufsize + 1);
		//take only the byte of chunk_number setted to a value different from 0: htons((unsigned short)chunk_number)= |0xxxxxxx|00000000|
		packet[bufsize] = (unsigned char)number;
		bufsize +=1;
	}
	else if(number < 16384 ) //2^14
	{
		packet = realloc(packet, bufsize + 2);
		//TODO taking this value back in the process_input use a short variable!
		unsigned short* num = (unsigned short*)(packet+bufsize);
		bufsize +=2;
		*num = htons((unsigned short)(number | 32768)) ; //10XXXXXX XXXXXXXX pattern
	}
	else if(number < 2097152) // 2^21
	{
		packet = realloc(packet, bufsize + 3);

		unsigned int num = htonl((unsigned int)(number | 12582912)); //110XXXXX XXXXXXXX XXXXXXXX pattern
		char* num_iterator = (char*)&num;
		//position 0 in csn is the most significant byte and it is 0 for the number in use here
		packet[bufsize]=num_iterator[1];
		packet[bufsize+1]=num_iterator[2];
		packet[bufsize+2]=num_iterator[3];
		bufsize +=3;
	}
	else if(number < 268435456U) //
	{
		packet = realloc(packet, bufsize + 4);

		unsigned int* num = (unsigned int*)packet+bufsize;
		*num = htonl((unsigned int)(number | 3758096384U)); // 1110XXXX XXXXXXXX XXXXXXXX XXXXXXXX pattern
		bufsize += 4;
	}
	else if(number < 2147483648U ) // 32 bit field   11110000 XXXXXXXX XXXXXXXX XXXXXXXX pattern
	{
		//TODO
		fprintf(stderr, "[CONET]:: size not yet supported for this len number\n");
		abort();
	}
	else if(number < 549755813888U) // 40 bit field   11110001 XXXXXXXX XXXXXXXX XXXXXXXX pattern
	{
		//TODO
		fprintf(stderr, "[CONET]: size not yet supported for this len number\n");
		abort();
	}

	*buf = packet;
	return bufsize;
}


unsigned long long read_variable_len_number(unsigned char* buf, int* pos){

	unsigned long long number;
	unsigned char* byte_scroller = buf + *pos;
	if (CONET_DEBUG >= 2) fprintf(stderr,"[conet_packet.c:%d]pos=%d, next_bytes= %.2X %.2X %.2X %.2X\n",
		__LINE__, *pos, buf[*pos], buf[*pos+1], buf[*pos+2], buf[*pos+3]);

	if((*byte_scroller | 127) == 127){ //number length: 1byte (1bit pattern) |0XXXXXXX|
		//const char value[2] = {*byte_scroller, '\0'};
		number = (unsigned long long)(*byte_scroller); //the pattern was 0XXXXXXX so we don't worry about the sign
		if (CONET_DEBUG >= 2)
			fprintf(stderr,"[conet_packet.c:%d]Pattern 0XXXXXXX. number=%llu, byte=%2X\n",
				__LINE__,number, buf[*pos]);
		*pos +=1;
	}
	else if((*byte_scroller & 192) == 128){ //number length: 2bytes (2bits pattern) |10XXXXXX|XXXXXXXX|
		unsigned char value[2] = {byte_scroller[0] & 127, byte_scroller[1]}; //return the value in network byte order without the pattern byte(2-bit pattern plus remaining 6-bit effective value) in first position
		number = (unsigned long long)ntohs(*((unsigned short*)value));
		if (CONET_DEBUG >= 2)
                        fprintf(stderr,"[conet_packet.c:%d]Pattern |10XXXXXX|XXXXXXXX. number=%llu, bytes=%2X %2X\n",
                                __LINE__,number, buf[*pos], buf[*pos+1]);
		*pos += 2;
	}
	else if((*byte_scroller & 224) == 192){ //number length: 3bytes (3bits pattern) |110XXXXX|XXXXXXXX|XXXXXXXX|
		unsigned int ret = 0; //clean memory
		unsigned char* ret_scroller = (unsigned char*)(&ret);
		ret_scroller[1] = byte_scroller[0] & 31;
		ret_scroller[2] = byte_scroller[1];
		ret_scroller[3] = byte_scroller[2];
		number = (unsigned long long)ntohl(ret);
		*pos += 3;
	}
	else if((*byte_scroller & 240) == 224){ //number length: 4byte (4bit pattern) |1110XXXX|XXXXXXXX|XXXXXXXX|XXXXXXXX|
		//return the value in network byte order with the pattern byte(3-bit pattern plus remaining 5-bit effective value) in first position
		unsigned int ret = *((unsigned int*)byte_scroller);
		unsigned char* ret_scroller = (unsigned char*)(&ret);
		//return the network byte order but without the 4 initial bit pattern
		ret_scroller[0] = byte_scroller[0] & 15;
		number = (unsigned long long)ntohl(ret);
		*pos += 4;
	}
	else if((*byte_scroller & 255) == 240){ //number length: 5bytes (8bits pattern) |11110000|XXXXXXXX|XXXXXXXX|XXXXXXXX|
		//TODO
		*pos += 5;
		fprintf(stderr, "[CONET]: size not yet supported for this len number\n");
		abort();
	}
	else if((*byte_scroller & 255) == 241){//number length: 6bytes (8bits pattern) |11110001|XXXXXXXX|XXXXXXXX|XXXXXXXX|
		//TODO
		*pos += 6;
		fprintf(stderr, "[CONET]: size not yet supported for this len number\n");
		abort();
	}

	return number;
}
