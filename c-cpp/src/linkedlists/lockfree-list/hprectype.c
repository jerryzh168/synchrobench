#include "hprectype.h"
#include <algorithm>

int maxThreadCount;
HP_t *HP;

__thread HPRecType_t thread_local_hpr;


void hp_init_global(int thread_cnt){
	maxThreadCount = thread_cnt;
	HP = (HP_t *)new HP_t[thread_cnt*K];
	for(int i = 0; i < thread_cnt*K; i++){
		HP[i].ptr = NULL;
	}
}

void retire_node(node_t *node){
	list_add_head(&thread_local_hpr.rlist, &node->r_entry);
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
	/* stage 1 */
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
		//	std::cout<<myhprec->tid<<":free node "<<node<<std::endl;
			myhprec->free_node(node);
		}
		entry = list_remv_head(&tmplist);
	}
	
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

