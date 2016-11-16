#ifndef _HPRECTYPE_H_
#define _HPRECTYPE_H_
#include <iostream>
#include "list.h"
#include "harris.h"

typedef struct hp{
	node_t *ptr;
	char padding[CACHE_LINE_SIZE - sizeof(node_t)];	
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
	node_t **prev;
	node_t *cur;
	node_t *next;
	int tid;
	void init(int maxThreadcount, void (*lamda)(node_t*), int idx){
		Active = true;
		rcount = 0;
		list_init(&rlist);
		plist = new node_t*[maxThreadcount*K];
		free_node = lamda;
		prev = NULL;
		cur = next = NULL;
		tid = idx;
	}
		
	
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

