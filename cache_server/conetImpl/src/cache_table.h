/**
 * author: andrea.araldo@gmail.com
 * inspired by http://jehiah.cz/a/uthash
 */

#ifndef CACHE_TABLE_H
#define	CACHE_TABLE_H

#ifdef	__cplusplus
extern "C" {
#endif


#ifdef	__cplusplus
}
#endif



#include <stdlib.h>    /* malloc       */
#include <stddef.h>    /* offsetof     */
#include <stdio.h>     /* printf       */
#include <string.h>    /* memset       */
#include "uthash/uthash.h"
#include "uthash/utstring.h"

#define MAX_CACHE_SIZE 2000


typedef struct CacheEntry {
    UT_hash_handle hh;
    unsigned long long chunk_size;
    long value;
    size_t nidlength;
    long tag;
    unsigned long long csn;
    char nid[];
} CacheEntry_t;

typedef struct Lookup_key{ long csn; char nid[]; } Lookup_key_t;

//nid must be of type UT_string*
#define CALCULATE_ENTRY_LEN(nid)		\
	sizeof(struct CacheEntry) + utstring_len(nid)


/* calculate the key length including padding
 * Expected types:
 * UT_string* mynid
 */
#define CALCULATE_KEY_LEN(nidlength)													\
    (																			\
    offsetof(struct CacheEntry, nid)    /* offset of last key field */			\
    + nidlength			             	/* size of last key field */			\
    - offsetof(struct CacheEntry, csn)	/* offset of first key field */    		\
    )


/**
 * Expected types:
 * lookup_key must be of type Lookup_key_t*
 * nid of type UT_string* 
 * csn of type long
*/
#define ALLOCATE_LOOKUP_KEY(lookup_key,nid,csn)							\
	lookup_key = malloc(sizeof(Lookup_key_t) + utstring_len(nid) );		\
    memset(lookup_key, 0, sizeof(Lookup_key_t) + utstring_len(nid) );	\
    lookup_key->csn = csn;												\
    memcpy(lookup_key->nid, utstring_body(nid), utstring_len(nid) );
    
/**
 * Expected types:
 * CacheEntry_t* entry
 * utstring nid_
 * long csn_
 * long tag_
 * unsigned long long chunk_size	(it must be filled)
 */
#define FILL_CACHE_ENTRY(entry,tag_,nid_,csn_,chunk_size,value_)							\
    entry = (CacheEntry_t*) malloc( CALCULATE_ENTRY_LEN(nid_) );	\
    memset(entry, 0, CALCULATE_ENTRY_LEN(nid_) ); /* zero fill */		\
    entry->nidlength = utstring_len(nid_);								\
    entry->value=value_; entry->csn=csn_; entry->tag=tag_;	\
    entry->chunk_size = chunk_size;    \
/*    printf("[cache_table.h:%d]chunk size is %llu\n",__LINE__,entry->chunk_size );*/ \
    memcpy(entry->nid, utstring_body(nid_) , entry->nidlength);

/**
 * Expected types
 * CacheEntry_t* 	cache 		(it must be filled)
 * CacheEntry_t* 	entry		(it will be filled)
 * CacheEntry_t* 	tmp_entry 	(it will be filled. It has no meaning but it is needed)
 * CacheEntry_t* 	entryToDelete(it will be filled)
 * UT_string* 		nid			(it must be filled)
 * unsigned long long 	csn			(it must be filled)
 * unsigned long long 	chunk_size	(it must be filled)
 * long 			value		(it must be filled)
 */
#define ADD_TO_CACHE_TABLE(cache,entry,tmp_entry,tag_,nid_,csn_,chunk_size_,value, entryToDelete)	\
    ;FILL_CACHE_ENTRY(entry,tag_,nid_,csn_,chunk_size_,value);\
    HASH_ADD( hh, cache, csn, (CALCULATE_KEY_LEN(entry->nidlength) ), entry);    \
   /* fprintf(stderr,"[cache_table.h] nid=%s, csn=%llu tag=%ld added. Now cache has %d elements\n", utstring_body(nid_),csn_,tag_,HASH_COUNT(cache) );*/\
    /** prune the cache to MAX_CACHE_SIZE */\
    if (HASH_COUNT(cache) > MAX_CACHE_SIZE)\
    {\
        HASH_ITER(hh, cache, entry, tmp_entry)\
        {\
            /*prune the first entry (loop is based on insertion order so this deletes */\
            /*   the oldest item) */\
            HASH_DELETE(hh, cache, entry);														\
            /*fprintf(stderr,"[cache_table.h]nid=%s, csn=%llu, tag=%llu deleted\n",(entry->nid),(entry->csn), (entry->tag) );*/								\
            /*FILL_CACHE_ENTRY(entryToDelete,(entry->nid),(entry->csn),0)*/\
			/*free(entry->nid);free(entry);*/														\
			entryToDelete=entry;\
			break;																					\
        }																						\
    }


/**
 * @return the chunk size of the chunk, if it exists, -1 if the chunk does not exist
 */
unsigned long long  find_on_cache_table(CacheEntry_t* cache,UT_string* nid, unsigned long long csn);
void add_to_cache_table (CacheEntry_t* cache, CacheEntry_t* entry);



#endif	/* CACHE_TABLE_H */
