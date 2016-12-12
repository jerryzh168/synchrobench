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
/* epoch is 60 ms */

#define timer_nsec 600000

static volatile unsigned long epoch = 1;

int maxThreadCount;
HP_t *HP;
pthread_t *thread_array;

__thread HPRecType_t thread_local_hpr;


void hp_init_global(int thread_cnt){
	maxThreadCount = thread_cnt;
	HP = (HP_t *)aligned_alloc(CACHE_LINE_SIZE, sizeof(HP_t) * thread_cnt * K); 
	//for(int i = 0; i < thread_cnt * K; i++)
	//	std::cout << &HP[i] <<std::endl;
	memset(HP, 0, sizeof(HP_t) * thread_cnt * K);
}

void smr_global_init(int thread_cnt){
	hp_init_global(thread_cnt);
}

#ifdef EPOCH_HP

unsigned long inc_epoch(){
	return ++epoch;
}
unsigned long int get_epoch(){
	return epoch;
}

void broadcast(int signum){	
	inc_epoch();
	AO_nop_full();
/*
    for(int i = 0; i < maxThreadCount; i++){
        pthread_t  trd = thread_array[i];
        if(pthread_kill(trd, SIGUSR1) != 0){
                return ;
        }
    }
*/

}

static volatile int timer_done = 0;
static volatile int timer_exit = 0;

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
		//printf("signal get %d\n", signum);
    	//printf("epoch is %lx\n", get_epoch());
    	if(timer_done == 1){
			timer_exit = 1;
			return NULL;
		}
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

void assign_epoch(int tid, unsigned long epoch) __attribute__((noinline));

void assign_epoch(int tid, unsigned long epoch){
	HP[K*tid].epoch = epoch;
}

void flush_handler(int signum){	
	AO_nop_full();
	//get_epoch();
	// register unsigned long  epoch_tmp = get_epoch();
	//std::cout<<&HP[K*thread_local_hpr.tid].epoch<<std::endl;
	// assign_epoch(thread_local_hpr.tid, epoch_tmp);
	//thread_local_hpr.tid = thread_local_hpr.tid;
	HP[K*thread_local_hpr.tid].epoch = get_epoch();
	//std::cout<<epoch_tmp<<std::endl;
	//std::cout<<thread_local_hpr.tid << ":"<<&HP[K*thread_local_hpr.tid].epoch<<std::endl;
}
#endif

void HPRecType_t::init(int maxThreadcount, pthread_t *peers, void (*lamda)(node_t*), void *(*alloc)(unsigned),int idx){
       this->Active = true;
       this->rcount = 0;
       list_init(&this->rlist);
       this->plist = (node_t**)malloc((maxThreadcount*K)*sizeof(node_t*));
       this->free_node = lamda;
       this->alloc_node = alloc;
       this->prev = NULL;
       this->cur = next = NULL;
       this->tid = idx;
	   /* register usr1 signal handling */
#ifdef EPOCH_HP
	   HP[K*idx].epoch = get_epoch();

	   struct sigaction s_action;
	   s_action.sa_handler = flush_handler;
	   s_action.sa_flags = 0 | SA_RESTART;
	   sigaction(SIGUSR1, &s_action, NULL);
	   sigset_t set;
	   sigemptyset(&set);
	   sigaddset(&set, TIMER_SIGNAL);

       pthread_sigmask(SIG_BLOCK, &set, NULL);
	   
      /* if I am in charge, register broadcast thread*/
       if(idx == maxThreadcount - 1){
            thread_array = peers;
            pthread_t sleeper;
            pthread_create(&sleeper, NULL, timer_handler, NULL);
       }
#endif
}

void HPRecType_t::finit(){
#ifdef EPOCH_HP
	if(thread_local_hpr.tid == 0){
		timer_done = 1;
		while(timer_exit == 0);
	}
#endif
	free(thread_local_hpr.plist);
}

void thread_local_init(thread_data_t *d){
	thread_local_hpr.init(d->nb_threads, d->threads, d->free_node, d->malloc_node, d->idx);
}

void thread_local_finit(){
	thread_local_hpr.finit();
}

int get_thread_idx(){
	return thread_local_hpr.tid;
}

void retire_node(node_t *node){
#ifdef EPOCH_HP
	node->remove_epoch = get_epoch();
#endif
	list_add_tail(&thread_local_hpr.rlist, &node->r_entry);
	thread_local_hpr.rcount++;
	// std::cout<<"rcount is "<<thread_local_hpr.rcount<<std::endl;
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
#ifdef EPOCH_HP
	unsigned int remove_epoch;
    list_t *entry = NEXT(&thread_local_hpr.rlist);
    if(entry == &thread_local_hpr.rlist)
    	return;

	node_t *node = LIST_ENTRY(entry, node_t, r_entry);

    unsigned long const remove_min = node->remove_epoch;
    unsigned long minimum = ((long)0 - 1);

   	for(int i = 0; i < K * maxThreadCount; i++){
		node_t *hp = HP[i].ptr;
		if(i % K == 0){
			unsigned long stmp = HP[i].epoch;
			if(stmp <= remove_min)
				return;
			else if(stmp < minimum){
				minimum = stmp;
			}
		}

		if(hp != NULL){
			plist[cnt++] = hp;
		}
	}

	std::sort(plist, plist + cnt);
	int deleted = 0;
    while(entry != &thread_local_hpr.rlist){
		node_t *node = LIST_ENTRY(entry, node_t, r_entry);
		if(node->remove_epoch >= minimum)
			break;
		entry = NEXT(entry);
		if(!std::binary_search(plist, plist + cnt, node)){
			list_remv(PREV(entry));
			myhprec->free_node(node);
			deleted++;
		}
    }

    if(deleted)
	    myhprec->rcount -= deleted;
	
 //    while(entry != &thread_local_hpr.rlist){
	// 	node_t *node = LIST_ENTRY(entry, node_t, r_entry);
	// 	remove_epoch = node->remove_epoch;
	// 	int status = DELETABLE;
	// 	int i;
	// 	for(i = 0; i < K * maxThreadCount; i++){
	// 		if(HP[(i)/K*K].epoch <= remove_epoch){
	// 			status = EPOCH_ISSUE;
	// 			break;
	// 		}else{
	// 			if(HP[i].ptr == node){
	// 				status = HAZARD;
	// 				break;
	// 			}
	// 		}
	// 	}
	// 	if(status == DELETABLE){
	// 		if(entry->next == entry)
	// 			exit(1);
	// 		entry = NEXT(entry);
	// 		list_remv(PREV(entry));
	// 		myhprec->rcount--;
	// 		myhprec->free_node(node);
	// 	}else if(status == HAZARD){
	// 		entry = NEXT(entry);
	// 	}else{
	// 		//std::cout<< "epoch "<<i<<":"<<HP[i].epoch<< "remove epoch"<<get_epoch()<< std::endl;
	// 		break;
	// 	}
		
	// }

#else

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
