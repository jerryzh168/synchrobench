#ifndef _HPRECTYPE_H_
#define _HPRECTYPE_H_
#include <iostream>
#include "list.h"
#include "harris.h"
#include <pthread.h>

//#define EPOCH_HP

#define TIMER_SIGNAL SIGALRM


typedef struct hp{
	node_t *ptr;
#ifdef EPOCH_HP
	unsigned long epoch;
	char padding[CACHE_LINE_SIZE - sizeof(node_t) - sizeof(unsigned long)];	
#else
	char padding[CACHE_LINE_SIZE - sizeof(node_t) ];	
#endif
}HP_t;

#define K 2

extern HP_t *HP;

class HPRecType_t{
	public:
	int Active;
	list_t rlist;
	int rcount;
	node_t **plist;
	void (*free_node)(node_t*);
	void *(*alloc_node)(unsigned );
	node_t **prev;
	node_t *cur;
	node_t *next;
	int tid;
	void init(int maxThreadcount, pthread_t *threads, void (*lamda)(node_t*), void *(*alloc)(unsigned),int idx);
	void finit();	
	
};
void hp_init_global(int thread_cnt);
extern __thread HPRecType_t thread_local_hpr;
//HPRecType_t *HeadHPRec = NULL;
extern int maxThreadCount;
#define H maxThreadCount*K 
#define R(H) ((H)*2 + 100)
//void allocate_hprec(HPRecType_t **myhprec);
//void retire_hprec(HPRecType_t *myhprec);
void retire_node(node_t *node);
void scan(HPRecType_t *myhprec);
//void help_scan(HPRecType_t *myhprec);
#endif

