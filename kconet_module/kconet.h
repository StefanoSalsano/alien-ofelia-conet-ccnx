
#define CONET_CIU_FLAGS_POS						0U
#define CONET_CIU_DEFAULT_NID_POS				1U
#define CONET_CIU_SHIFTED_NID_POS				2U
#define CONET_CIU_SHIFTED_NID_LENGTH_POS		1U

#define CONET_INTEREST_CIU_TYPE_FLAG			1U
#define CONET_NAMED_DATA_CIU_TYPE_FLAG			2U
#define CONET_CIU_NOCACHE_FLAG					0U
#define CONET_CIU_CACHE_FLAG					1U
#define CONET_CIU_DEFAULT_NID_LENGTH_FLAG		0U
#define CONET_CIU_VARIABLE_NID_LENGTH_FLAG		2U

#define CONET_FINAL_SEGMENT_FLAG                1U

#define CONET_ASK_CHUNK_INFO_FLAG				1U


#define CONET_DEFAULT_NID_LENGTH				16U
#define CONET_MAX_CHUNK_SIZE  262144 //256*1024 //TODO

#define CONET_DEFAULT_MTU						1400 //1468
#define CONET_PREFETCH 0

enum conet_chunk_size {
	CONET_CONTINUATION,
	CONET_FOLLOW_VAR_CHUNK_SIZE,
	CONET_2KB,
	CONET_4KB,
	CONET_8KB,
	CONET_16KB,
	CONET_32KB,
	CONET_64KB,
	CONET_128KB,
	CONET_256KB,
	CONET_512KB,
	CONET_1MB,
	CONET_2MB,
	CONET_4MB,
	CONET_8MB
};

// #define DEBUG
#ifdef DEBUG
	#define KDEBUG(fmt, args...) printk( KERN_INFO "a0: " fmt, ## args)
#else
	#define KDEBUG(fmt, args...) /* not debugging: nothing */
#endif


//queste cose andrebbero tolte ma per adesso fanno comodo

#define FILE_NAME "/home/stefano/ccnrep/mod/examplemb1"
#define URI_LEN 255 //lunghezza statica di una uri ccnx 
#define INTEREST_CP_MAX_SIZE 150 //lunghezza statica di un interest cp
#define CHUNK_SIZE 4096

char*  conet_process_interest_cp(/*char *src_addr, */unsigned char* readbuf,int *conet_payload_size, unsigned short ll, unsigned short c, unsigned short ask_info  /* ,unsigned short is_raw*/);
unsigned int inet_addr(char *str);
