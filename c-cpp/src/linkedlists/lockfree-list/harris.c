#include "linkedlist.h"
#include "hprectype.h"
#include "list.h"



int harris_insert(intset_t *set, val_t val) {
	node_t *newnode = NULL;
	while(1){
		if(find(set, val)){
		  if(newnode != NULL)
			/* TODO free the node */
		  return 0;
		}
		if(newnode == NULL){
			newnode = new_node(val, thread_local.cur, 0);
			AO_nop_full();
		}
		else
			newnode->next = thread_local.cur;
		//node->next = thread_local.cur;
		//AO_nop_full(); 	
		if(ATOMIC_CAS_MB(thread_local.prev, thread_local.cur, newnode)) return 1;
	}
}

int harris_delete(intset_t *set, val_t val) {
	while(1){
		if(!find(set, val)) return 0;
		if(!ATOMIC_CAS_MB(&(thread_local.cur->next), thread_local.next, thread_local.next | 0x1))	continue;
		if(ATOMIC_CAS_MB(thread_local.prev, thread_local.cur, thread_local.next))
			retire_node(thread_local.cur);
		else
			find(set, val);
		return 1;
	}	

}



int harris_find(intset_t *set, val_t val){
	return find(set, val);
}


int find(intset_t *set, val_t val){
try_again:
	int offset = 0;
	int start = K*thread_local.tid;
	thread_local.prev = set->head;
	thread_local.cur = *(thread_local.prev);
	while(thread_local.cur != NULL){
		HP[base + offset].ptr = thread_local.cur;					
		AO_nop_full();
		if(*thread_local.prev != thread_local.cur)
			goto try_again;
		thread_local.next = (thread_local.cur)->next;
		if(thread_local.next & 1){
			if(!ATOMIC_CAS_MB(thread_local.prev, thread_local.cur, thread_local.next&(~1))) goto try_again;
			retire_node(thread_local.cur);
			thread_local.cur = thread_local.next&(~1);
		}else{
			int cval = thread_local.cur->val;
			if(*thread_local.prev != thread_local.cur) goto try_again;
			if(cval >= val) return cval == val;
			thread_local.prev = &thread_local.cur->next;
			offset = 1 - offset;
			thread_local.cur = thread_local.next;
		}
	}
	return false;
}
