/*
 *  linkedlist.c
 *  
 *  Linked list data structure
 *
 *  Created by Vincent Gramoli on 1/12/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "linkedlist.h"
#include "../hashtables/lockfree-ht/smr.h"
#include "qsbr.h"
__thread thread_local_info_t bench;

node_t*
new_node(skey_t key, node_t *next, int initializing) {
    node_t *node;

    node = (node_t *) ssalloc_alloc(0, sizeof(node_t));
    
    if (node == NULL) {
        return NULL;
    }

    node->key = key;
    //node->val = val;
    node->next = next;
    return (node_t*) node;
}

intset_t*
set_new() {
    intset_t *set;
    node_t *min, *max;

    if ((set = (intset_t*) memalign(CACHE_LINE_SIZE, sizeof(intset_t))) == NULL) {
        perror("malloc");
        exit(1);
    }


    max = new_node(KEY_MAX, NULL, 1);
    min = new_node(KEY_MIN, max, 1);
    set->head = min;

    return set;
}

void set_delete(intset_t *set) {
    node_t *node, *next;

    node = set->head;
    while (node != NULL) {
        next = node->next;
        free((void*) node);
        node = next;
    }
    free(set);
}

/* should get the idx of thread passed in from d->idx, not pthread_self()!!*/
int get_thread_idx() {
  return bench.thread_id;
}
/* you can have a chance to do thread local init */
void thread_local_init(thread_data_t *d) {
  bench.thread_id = d->idx;
  bench.malloc_node = d->malloc_node;
  bench.free_node = d->free_node;
  bench.malloc_node_aligned = d->malloc_node_aligned;
  ssalloc_init();
  return;
}
/* you can have a chance to do global init before any smr threads are spawned */
void smr_global_init(int thread_cnt) {
  mr_init_global(thread_cnt);
  return;
}
