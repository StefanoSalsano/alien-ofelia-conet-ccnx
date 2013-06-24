#include "../include/conet/conet.h"
#include "../include/ccn/hashtb.h"






int main(int argc, char ** argv){
	char* nid="123";
	int chunk=1;
	struct hashtb_enumerator ee;
	struct hashtb_enumerator *e = &ee;
	struct hashtb* conetht = hashtb_create(sizeof(struct conet_entry), NULL);
	hashtb_start(conetht, e);
	struct  conet_entry* ce;
	int res=hashtb_seek(e, nid, strlen(nid), 0);
	if (res < 0)
		perror("errore nel seek dell'ht");
	ce = e->data;
	if (res == HT_NEW_ENTRY) {
		  ce->prev=NULL;
		  ce->next=NULL;
		  ce->chunk_number=chunk;
		  ce->m_req_wnd=1;
		  ce->m_segment_size=1046;
		  ce->nid=nid;
	}





	hashtb_end(e);
}


