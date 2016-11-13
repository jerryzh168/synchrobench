#ifndef _HPRECTYPE_H_
#define _HPRECTYPE_H_

#include "list.h"
#include "harris.h"

typedef struct hp{
	node_t *ptr;
	char padding[CACHE_LINE_SIZE - sizeof(NodeType_t)];	
}HP_t;


HP_t *HP;

class HPRecType_t{
	HPRecType_t *Next;
	int Active;
	list_t rlist;
	int rcount;
	node_t **plist;
	void (*free_node)(node_t*);
	node_t **prev;
	node_t *cur;
	node_t *next;
	int tid;
	HPRecType_t(int maxThreadcount, void (*lamda)(node_t*), int idx){
		Active = true;
		rcount = 0;
		for( int i = 0; i < K ; i++)
			HP[i] = NULL;
		Next = NULL;
		list_init(&rlist);
		plist = new node_t*[maxThreadcount*K];
		free_node = lamda;
		prev = cur = next = NULL;
		tid = idx;
	}

};

HPRecType_t *HeadHPRec = NULL;
int H = 0;
int maxThreadCount;
void allocate_hprec(HPRecType_t **myhprec);
void retire_hprec(HPRecType_t *myhprec);
void retire_node(node_t *node, HPRecType_t *myhprec);
void scan(HPRecType_t *head, HPRecType_t *myhprec);
void help_scan(HPRectType_t *myhprec);
#endif

