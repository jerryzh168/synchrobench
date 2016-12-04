#include "linkedlist.h"
#include "hprectype.h"
#include "list.h"
#include "harris.h"

static int find(intset_t *set, val_t val);

static void reset_hp(){
	 int base =  K*thread_local_hpr.tid;
	 HP[base].ptr = NULL;
	 HP[base + 1].ptr = NULL;
}

int harris_insert(intset_t *set, val_t val) {
	node_t *newnode = NULL;
	while(1){
		if(find(set, val)){
		  if(newnode != NULL)
		  	thread_local_hpr.free_node(newnode);
		  reset_hp();
		  return 0;
		}
		if(newnode == NULL){
			newnode = new_node(val, thread_local_hpr.cur, 0);
		}
		else
			newnode->next = thread_local_hpr.cur;
		//node->next = thread_local.cur;
		//AO_nop_full(); 	
		if(ATOMIC_CAS_MB(thread_local_hpr.prev, thread_local_hpr.cur, newnode)) {
		reset_hp();
		return 1;
	}
	}
}

int harris_delete(intset_t *set, val_t val) {
	while(1){
		if(!find(set, val)){
			reset_hp();
			return 0;
		}

		if(!ATOMIC_CAS_MB(&(thread_local_hpr.cur->next), thread_local_hpr.next, (uint64_t)thread_local_hpr.next | 0x1L))	continue;
		if(ATOMIC_CAS_MB(thread_local_hpr.prev, thread_local_hpr.cur, thread_local_hpr.next))
			retire_node(thread_local_hpr.cur);
		else
			find(set, val);
		reset_hp();
		return 1;
	}	

}



int harris_find(intset_t *set, val_t val){
	int ret = find(set, val);
	reset_hp();
	return ret;
}


static int find(intset_t *set, val_t val){
try_again:
	int offset = 0;
	int base = K*thread_local_hpr.tid;
	thread_local_hpr.prev = &set->head->next->next;
	thread_local_hpr.cur = *(thread_local_hpr.prev);
	while(thread_local_hpr.cur != NULL){
		HP[base + offset].ptr = thread_local_hpr.cur;					
		//AO_nop_full();
		if(*thread_local_hpr.prev != thread_local_hpr.cur)
			goto try_again;
		thread_local_hpr.next = (thread_local_hpr.cur)->next;
		if((uint64_t)thread_local_hpr.next & 0x1L){
			if(!ATOMIC_CAS_MB(thread_local_hpr.prev, thread_local_hpr.cur, (uint64_t)thread_local_hpr.next&(~0x1L))) goto try_again;
			retire_node(thread_local_hpr.cur);
			thread_local_hpr.cur = (node_t *)((uint64_t)thread_local_hpr.next&(~0x1L));
		}else{
			int cval = thread_local_hpr.cur->val;
			if(*thread_local_hpr.prev != thread_local_hpr.cur) goto try_again;
			if(cval >= val) return cval == val;
			thread_local_hpr.prev = &thread_local_hpr.cur->next;
			offset = 1 - offset;
			thread_local_hpr.cur = thread_local_hpr.next;
		}
	}
	return false;
}
