#include "hprectype.h"
#include <algorithm>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include "../../hashtables/lockfree-ht/smr.h"
/* epoch is 30 ms */
#define timer_nsec 30000

static unsigned long epoch = 1;

int maxThreadCount;
HP_t *HP;
pthread_t *thread_array;

__thread HPRecType_t thread_local_hpr;


void hp_init_global(int thread_cnt){
	maxThreadCount = thread_cnt;
	HP = (HP_t *)aligned_alloc(CACHE_LINE_SIZE, sizeof(HP_t) * thread_cnt * K); 
	memset(HP, 0, sizeof(HP_t) * thread_cnt * K);
}

void smr_global_init(int thread_cnt){
	hp_init_global(thread_cnt);
}

unsigned int inc_epoch(){
	return ++epoch;
}
unsigned int get_epoch(){
	return epoch;
}

void broadcast(int signum){	
//	inc_epoch();
//	AO_nop_full();
}
void *timer_handler(void *arg){
	pthread_detach(pthread_self());
	struct sigaction s_action;
    s_action.sa_handler = broadcast;
    sigaction(TIMER_SIGNAL, &s_action, NULL);
    timer_t timerid;
    struct sigevent sev;
    struct itimerspec its;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = TIMER_SIGNAL;
    sev.sigev_value.sival_ptr = &timerid;
    timer_create(CLOCK_REALTIME, &sev, &timerid);
    its.it_value.tv_sec = timer_nsec / 1000000000;
    its.it_value.tv_nsec = timer_nsec %1000000000;
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;
    timer_settime(timerid, 0, &its, NULL);
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, TIMER_SIGNAL);
	//std::cout<<"SSSS"<<std::endl;
	while(1){
		//printf("signal\n");
		int signum;
		sigwait(&set, &signum); 
		//printf("signal get\n");
    	//printf("epoch is %lx\n", get_epoch());
    	inc_epoch();
		AO_nop_full();
    	for(int i = 0; i < maxThreadCount; i++){
        	pthread_t  trd = thread_array[i];
            if(pthread_kill(trd, SIGUSR1) != 0){
               	return NULL;
            }
    	}
	}
}

void flush_handler(int signum){	
	AO_nop_full();
	HP[K*thread_local_hpr.tid].epoch = get_epoch();
}

void HPRecType_t::init(int maxThreadcount, pthread_t *peers, void (*lamda)(node_t*), void *(*alloc)(unsigned),int idx){
       this->Active = true;
       this->rcount = 0;
       list_init(&this->rlist);
	   HP[K*idx].epoch = get_epoch();
       this->plist = (node_t**)malloc((maxThreadcount*K)*sizeof(node_t*));
       this->free_node = lamda;
       this->alloc_node = alloc;
       this->prev = NULL;
       this->cur = next = NULL;
       this->tid = idx;
	   /* register usr1 signal handling */
	   struct sigaction s_action;
	   s_action.sa_handler = flush_handler;
	   s_action.sa_flags = 0 | SA_RESTART;
	   sigaction(SIGUSR1, &s_action, NULL);
	   sigset_t set;
	   sigemptyset(&set);
	   sigaddset(&set, TIMER_SIGNAL);
       pthread_sigmask(SIG_BLOCK, &set, NULL);
	   
      /* if I am in charge, register broadcast thread*/
       if(idx == 0){
            thread_array = peers;
            pthread_t sleeper;
            pthread_create(&sleeper, NULL, timer_handler, NULL);
       }
}

void thread_local_init(thread_data_t *d){
	thread_local_hpr.init(d->nb_threads, d->threads, d->free_node, d->malloc_node, d->idx);
}

int get_thread_idx(){
	return thread_local_hpr.tid;
}

void retire_node(node_t *node){
	node->remove_epoch = get_epoch();
	list_add_tail(&thread_local_hpr.rlist, &node->r_entry);
	thread_local_hpr.rcount++;
	//std::cout<<"rcount is "<<thread_local_hpr.rcount<<std::endl;
	if(thread_local_hpr.rcount >= R(H)){
		scan(&thread_local_hpr);
		//help_scan(myprec);	
	}
}

void scan(HPRecType_t *myhprec){
	int cnt = 0;
	node_t **plist = myhprec->plist;
	//HPRecType_t *hprec;
	//hprec = head;
	AO_nop_full();
	/* stage 1 */
	unsigned int remove_epoch;
    list_t *entry = NEXT(&thread_local_hpr.rlist);
#define DELETABLE 		0
#define EPOCH_ISSUE 	1
#define HAZARD			2
    while(entry != &thread_local_hpr.rlist){
		node_t *node = LIST_ENTRY(entry, node_t, r_entry);
		remove_epoch = node->remove_epoch;
		int status = DELETABLE;
		int i;
		for(i = 0; i < K * maxThreadCount; i++){
			if(HP[(i)/K*K].epoch <= remove_epoch){
				status = EPOCH_ISSUE;
				break;
			}else{
				if(HP[i].ptr == node){
					status = HAZARD;
					break;
				}
			}
		}
		if(status == DELETABLE){
			if(entry->next == entry)
				exit(1);
			entry = NEXT(entry);
			list_remv(PREV(entry));
			myhprec->free_node(node);
		}else if(status == HAZARD){
			entry = NEXT(entry);
		}else{
			//std::cout<< "epoch "<<i<<":"<<HP[i].epoch<< "remove epoch"<<get_epoch()<< std::endl;
			break;
		}
		
	}
#if 0
	for(int i = 0; i < K * maxThreadCount; i++){
		node_t *hp = HP[i].ptr;
		if(hp != NULL){
			plist[cnt++] = hp;
		}
	}
	
	std::sort(plist, plist + cnt);

	/* stage 2 */
	list_t tmplist;
	list_init(&tmplist);
	list_append(&tmplist, &myhprec->rlist);
	myhprec->rcount = 0;
	list_t *entry = list_remv_head(&tmplist);
	while(entry != NULL){
		node_t *node = LIST_ENTRY(entry, node_t, r_entry);
		if(std::binary_search(plist, plist + cnt, node)){
			list_add_head(&myhprec->rlist, &node->r_entry);
			myhprec->rcount++;
		}else{
				
			myhprec->free_node(node);
		}
		entry = list_remv_head(&tmplist);
	}
#endif
	
}

/*
void help_scan(HPRecType_t *myhprec){
	HPRecType_t *hprec;
	list_t *entry;
	for(hprec = HeadHPRec; hprec != NULL; hprec = hprec->Next){
		if(hprec->Active) continue;
		if(ATOMIC_TAS(&hprec->Active)) continue;
		while(hprec->rcount > 0){
			entry = list_pop(&hprec->rlist);
			hprec->rcount--;
			list_push(&hprec->rlist, entry);
			myhprec->rcount++;
			if(myhprec->rcount >= R(H))
				scan(HeadHPRec, myhprec);
		}
		hprec->Active = false;
	}
}
*/

