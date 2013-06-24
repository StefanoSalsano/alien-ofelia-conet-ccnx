/*
 * conet_timer.c
 *
 *  Created on: 27/nov/2011
 *      Author: cancel
 */


#include <conet/conet_timer.h>

struct timeout_entry* TIMEOUT_LIST_START=NULL;
struct timeout_entry* TIMEOUT_LIST_END =NULL;
unsigned int list_size=0;

void timeout_list_append(struct conet_entry* ce, struct chunk_entry* ch){
	double now = get_time();
	struct timeout_entry* new = malloc(sizeof(struct timeout_entry));
	new->ce=ce;
	new->ch=ch;
	new->time=now+ce->rto;
//	if (CONET_DEBUG >= 1) fprintf(stderr, "ce->rto : %f \n", ce->rto/1e6);
//	if (CONET_DEBUG >= 1) fprintf(stderr, "now : %f \n", now/1e6);


	new->next=NULL;
	new->prev=NULL;
	if (CONET_DEBUG >= 1) fprintf(stderr, "Append a timeout at time: %f for %s/%llu tries: %u \t list_size: %d \n", new->time/1e6, new->ce->nid, new->ch->chunk_number, new->ch->tries, list_size);

	if (TIMEOUT_LIST_START==NULL){
		TIMEOUT_LIST_START = new;
		TIMEOUT_LIST_END = new;
	}
	else {
		TIMEOUT_LIST_END->next=new;
		new->prev=TIMEOUT_LIST_END;
		TIMEOUT_LIST_END=new;
	}
	list_size++;
	ch->to=new;
}

void check_for_timeouts(struct ccnd_handle* h) {
	struct timeout_entry* t = timeout_list_pop();
	if (t==NULL){
		return;
	}
	do {
		if (t->time==0 ){
			t->ce=NULL;
			t->ch=NULL;
			free(t);
			t=timeout_list_pop();
			break;
		}
		//if (CONET_DEBUG >= 1) fprintf(stderr, " %fl Timeout for %s/%d tries: %u \t list_size: %d \n", t->time, t->ce->nid, t->ch->chunk_number, t->ch->tries, list_size);

		t->ch->tries++;
		if (list_size<0){


			fprintf(stderr, "impossibru");

		}


		if (CONET_DEBUG >= 1) fprintf(stderr, " Timeout for %s/%llu at time %f tries: %u \t list_size: %d \n", t->ce->nid, t->ch->chunk_number, t->time/1e6, t->ch->tries, list_size);
		if ( t->ch->tries > MAX_TRIES ){
			if (1) fprintf(stderr, " Timeout for %s/%llu at time %f tries: %u \t list_size: %d \n", t->ce->nid, t->ch->chunk_number, t->time/1e6, t->ch->tries, list_size);

			if (1) fprintf(stderr, " Give up\n");
			remove_entry(t);
			t->ch->never_arrived=1;
			t->ch=NULL;
			t->ce=NULL;
			free(t);
			t=timeout_list_pop();
			break;
		}
		if (t->ch->arrived==1 || t->ch->chunk_number>t->ce->final_chunk_number){
			if (CONET_DEBUG >= 1) fprintf(stderr, " Chunk removed\n");
			//possibili memory leak usare la remove chunk
			if (t->ce->chunk_expected!=NULL && t->ce->chunk_expected->chunk_number == t->ch->chunk_number){
				t->ce->chunk_expected=NULL;
			}
			t->ch->to=NULL;
			struct hashtb_enumerator ee;
			struct hashtb_enumerator *e1 = &ee;

			hashtb_start(t->ce->chunks, e1);
			int r=hashtb_seek(e1, &(t->ch->chunk_number), sizeof(int), 0);
			if (r==HT_OLD_ENTRY){
				hashtb_delete(e1);
			}

			hashtb_end(e1);
			remove_entry(t);
			free(t);
			t=timeout_list_pop();
			break;
		}
		if (CONET_DEBUG >= 1){
			int i=0;
			for (i=0; i< t->ch->last_interest; i++){
				fprintf(stderr, "\n last_in_seq: %d \n ", t->ch->last_in_sequence);
				if (t->ch->expected[i].bit==1){

					fprintf(stderr, "\n %llu to send segment: %d \n ", t->ch->chunk_number, i);
				}
			}
		}
#ifdef TCP_BUG_FIX
		if (t->ce->out_of_sequence_counter>=3){
			t->ce->out_of_sequence_counter=0;
		}
		t->ce->threshold = t->ce->in_flight /2;
//		t->ce->in_flight = 0 ;
		t->ce->cwnd = 1;
#endif
		t->ch->is_oos=1;
		conet_send_interest_cp(h, t->ce, t->ch, 1);
		reschedule(t);

		t=timeout_list_pop();

	}while (t!=NULL);

}

void reschedule (struct timeout_entry* t){


	remove_entry(t);

	//unsigned long now = get_time();
	double now = get_time();

	t->time=now+t->ce->rto;
	t->prev=NULL;
	t->next=NULL;
	if (CONET_DEBUG >= 1) fprintf(stderr, "Reschedule a timeout for %s/%llu at time %f (s) tries: %u \t list_size: %d \n", t->ce->nid, t->ch->chunk_number, t->time/1e6, t->ch->tries, list_size);

	if (TIMEOUT_LIST_START==NULL){
		TIMEOUT_LIST_START=t;
	}
	if (TIMEOUT_LIST_END!=NULL){
		TIMEOUT_LIST_END->next=t;
		t->prev=TIMEOUT_LIST_END;
	}
	TIMEOUT_LIST_END=t;


	list_size++;

}
struct timeout_entry* timeout_list_pop(){
	if (TIMEOUT_LIST_START==NULL){
		return NULL;
	}
	double now = get_time();
	double last = TIMEOUT_LIST_START->time;
	//if (CONET_DEBUG >= 1) fprintf(stderr, " now : %f last : %f \n",now/1e6, last/1e6 );

	double diff=now-last;

	struct timeout_entry* ret = NULL;
	if (CONET_DEBUG >= 1) fprintf(stderr, " Next timeout for %s/%llu at diff: %f\n",TIMEOUT_LIST_START->ce->nid, TIMEOUT_LIST_START->ch->chunk_number, -diff/1e6 );
	if (diff>0){
		ret=TIMEOUT_LIST_START;
		if (CONET_DEBUG >= 1) fprintf(stderr, " Timeout expired for %s/%llu",ret->ce->nid, ret->ch->chunk_number );
		remove_entry(TIMEOUT_LIST_START);
	}

	return ret;
}

double get_time(){
	struct timeval time = {0};
	gettimeofday(&time, NULL);
	//return (double) ((time.tv_sec* (double) CLOCK_MICROS + time.tv_usec);
	return (double) ((double) time.tv_sec* (double) CLOCK_MICROS + (double) time.tv_usec);
}

FILE* rtt_file = NULL;
void update_rto(struct chunk_entry* ch, struct conet_entry* ce){
	if (ch->is_oos==1){
		//avoid that great rtt for oos interfere with retransmit timeout
		return;
	}

	double r= get_time() - ch->send_started;
	if (CONET_DEBUG >= 1) fprintf(stderr, "RTT %f\n", r);
	double alpha=0.125;
	double beta = 0.25;
	if (ce->srtt==0){
		ce->srtt = r;
		ce->rttvar=r/2;
		ce->rto = ce->srtt + ((G>4*ce->rttvar)?G:4*ce->rttvar);
	}
	else {
		ce->rttvar = (1-beta)*ce->rttvar + beta*fabs(ce->srtt - r);
		ce->srtt = (1-alpha)*ce->srtt + alpha * r;
		ce->rto = ce->srtt + ((G>4*ce->rttvar)?G:4*ce->rttvar);
	}
	if (CONET_LOG_TO_FILE){
		if (rtt_file==NULL){
			rtt_file=fopen("rtt_log.csv", "wt");
		}
		log_to_file(rtt_file, (int)r, 0,  ch->chunk_number );
	}
	// dynamic threshold change (avoid too big threshold)
//	if (ce->cwnd < ce->threshold && ch->chunk_number>10){
//		double new_th_b = (double)ce->srtt *100; //(100 b/us = 100Mb/s)
//		int new_th = (int)(new_th_b /CONET_DEFAULT_MTU);
//		if (ce->threshold>new_th){
//			ce->threshold=new_th;
//			if (CONET_DEBUG >= 1) fprintf(stderr, "NEW threshold set to %d segments \n", new_th);
//		}
//
//	}




	if (CONET_DEBUG >= 1) fprintf(stderr, " RTO: %f \n", ce->rto);
	if (CONET_DEBUG >= 1) fprintf(stderr, "%s: rtt:%f \t rttvar:%f \t srtt:%f \t rto: %f\n",ce->nid,r, ce->rttvar, ce->srtt, ce->rto );
}

void remove_entry(struct timeout_entry* t){
	struct timeout_entry* n=t->next;
	struct timeout_entry* p=t->prev;
	if (t==TIMEOUT_LIST_START || t==TIMEOUT_LIST_END){
		if (t==TIMEOUT_LIST_START){
			TIMEOUT_LIST_START=n;
			if (n!=NULL){
				n->prev=NULL;
			}
		}
		if(t==TIMEOUT_LIST_END) {
			TIMEOUT_LIST_END=p;
			if (p!=NULL){
				p->next=NULL;
			}
		}
	}
	else if (n!=NULL && p!=NULL){
		p->next=n;
		n->prev=p;
	}
	t->next=NULL;
	t->prev=NULL;
	list_size--;
}
