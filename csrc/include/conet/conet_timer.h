#ifndef CONET_TIMER_H_
#define CONET_TIMER_H_


#include <sys/time.h>
#include <math.h>

#include <conet/conet.h>

#include <ccn/hashtb.h>


#include "../../ccnd/ccnd_private.h"

#define CLOCK_MICROS  1000000
#define MAX_TRIES 	10
#define TIMEOUT		500000

#define G	750000//500000



struct timeout_entry {
	struct conet_entry* ce;
	struct chunk_entry* ch;
	double time;
	struct timeout_entry* prev;
	struct timeout_entry* next;
};

void check_for_timeouts(struct ccnd_handle* h);
void timeout_list_append(struct conet_entry* ce, struct chunk_entry* ch);
double get_time(void );
struct timeout_entry* timeout_list_pop(void);
void reschedule (struct timeout_entry* t);
void update_rto(struct chunk_entry* ch, struct conet_entry* ce);
void remove_entry(struct timeout_entry* t);


#endif /* CONET_TIMER_H_ */












