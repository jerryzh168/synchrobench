#include <hprectype.h>
#include <algrithm>

void allocate_hprec(HPRectType **myhprec, int maxThreadcount, void (*lamda)(node_t*), int idx){
	HPRecType_t *hprec;
	for(hprec = HeadHPRec; hprec != NULL; hprec = hprec->Next){
		if(hprec->Active)
			continue;
		if(ATOMIC_TAS(&hprec->Active)
			continue;
		*myhprec = hprec;
		return;
	}
	int old;
	do{
		oldcount = H;
	}while(!ATOMIC_CAS_MB(&H, oldcount, oldcount + K));

	hprec = new HPRecType_t(maxThreadcount, lamda, idx);
	HPRecType_t *oldhead;
	do{
		oldhead = HeadHPRec;
		hprec->Next = oldhead;
	}while(!ATOMIC_CAS_MB(&HeadHPRec, oldhead, hprec));
	*myhprec = hprec;
}

void retire_hprec(HPRecType_t *myhprec){
	for(int i = 0; i < K; i++)
		myhprec->HP[i] = NULL;
	myhprec->Active = false;
}

void retire_node(node_t *node, HPRecType_t *myhprec){
	HPRecType_t *head;
	list_push(&myhprec->rlist, &node->r_entry);
	myhprec->rcount++;
	head = HeadHPRec;
	if(myhprec->rcount >= R(H)){
		scan(head, myhprec);
		help_scan(myprec);	
	}
}

void scan(HPRecType_t *head, HPRecType_t *myhprec){
	int cnt = 0;
	node_t *plist = myhprec->plist;
	HPRecType_t *hprec;
	hprec = head;
	/* stage 1 */
	while(hprec != NULL){
		for(int i = 0; i < K; i++){
			node_t *hp = hprec->HP[i];
			if(hp != NULL){
				plist[cnt++] = hp;
			}
		}
	}

	std::sort(plist, plist + cnt);

	/* stage 2 */
	list_t tmplist;
	list_init(&tmplist);
	list_append(&tmplist, &myhprec->rlist);
	myhprec->rcount = 0;
	list *entry = list_pop(&tmplist);
	while(entry != NULL){
		node_t *node = LIST_ENTRY(entry, node_t, r_entry);
		if(std::binary_search(plist, plist + cnt, node)){
			list_push(&myhprec->rlist, &node->r_entry);
			myhprec->rcount++;
		}else{
			myhprec->free_node(node);
		}
		entry = list_pop(&tmplist);
	}
	
}


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

