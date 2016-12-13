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
#include "../../hashtables/lockfree-ht/smr.h"
__thread int thread_id;

void *(*__malloc)(unsigned int) = NULL;
void (*__free)(void *) = NULL;

node_t *new_node(val_t val, node_t *next, int transactional)
{
  node_t *node;
  if(!__malloc)
    node = (node_t *)malloc(sizeof(node_t));
  else
  	node = (node_t *)__malloc(sizeof(node_t));
  if (node == NULL) {
	perror("malloc");
	exit(1);
  }

  node->val = val;
  node->next = next;

  return node;
}

intset_t *set_new()
{
  intset_t *set;
  node_t *min, *max;
	
  if ((set = (intset_t *)malloc(sizeof(intset_t))) == NULL) {
    perror("malloc");
    exit(1);
  }
  max = new_node(VAL_MAX, NULL, 0);
  min = new_node(VAL_MIN, max, 0);
  set->head = min;

  return set;
}

void set_delete(intset_t *set)
{
  node_t *node, *next;

  node = set->head;
  while (node != NULL) {
    next = node->next;
    free(node);
    node = next;
  }
  free(set);
}

int set_size(intset_t *set)
{
  int size = 0;
  node_t *node;

  /* We have at least 2 elements */
  node = set->head->next;
  while (node->next != NULL) {
    size++;
    node = node->next;
  }

  return size;
}

/* Tests */

/* should get the idx of thread passed in from d->idx, not pthread_self()!!*/
int get_thread_idx() {
  return thread_id;
}
/* you can have a chance to do thread local init */
void thread_local_init(thread_data_t *d) {
  thread_id = d->idx;
  __malloc = d->malloc_node;
  __free = d->free_node;
  return;
}
/* you can have a chance to do global init before any smr threads are spawned */
void smr_global_init(int thread_cnt) {
  return;
}
