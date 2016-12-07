/*
 *  linkedlist.c
 *  
 *  Linked list data structure
 *
 *  Created by Vincent Gramoli on 1/12/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include <atomic>
#include "linkedlist.h"
#include <stdlib.h>
#include "../../hashtables/lockfree-ht/smr.h"
__thread thread_local_info_t bench;

#define CACHE_LINE_SIZE 64

node_t *new_node(val_t val, node_t *next, int transactional)
{
  node_t *node;
  if (bench.malloc_node != NULL) {
    node = (node_t *)bench.malloc_node(sizeof(node_t));
  } else {
    node = (node_t *)aligned_alloc(CACHE_LINE_SIZE, sizeof(node_t));
  }
  if (node == NULL) {
	perror("malloc");
	exit(1);
  }
  node->val = val;
  node->rc = 1;
  node->next = NULL;
  if (next != NULL) {
    LFRCCopy(&node->next, next);
  }

  return node;
}

intset_t *set_new()
{
  intset_t *set;
  node_t *min = NULL, *max = NULL;
  if ((set = (intset_t *)malloc(sizeof(intset_t))) == NULL) {
    perror("malloc");
    exit(1);
  }
  max = new_node(VAL_MAX, NULL, 0); // node_t *
  min = new_node(VAL_MIN, max, 0);
  set->head = NULL;
  LFRCStoreAlloc(&set->head, min);
  return set;
}

void set_delete(intset_t *set)
{
  node_t *node, *next;

  node = set->head;
  while (node != NULL) {
    next = node->next;
    //if(bench.free_node != NULL) {
    //  bench.free_node(node.load());
    //} else {
    free(node);
    //}
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

// LFRC

/* void LFRCLoad(node_t **dest, node_t **A) { */
/*   long r; node_t *a, *olddest = *dest; */
/*   while (true) { */
/*     a=*A; */
/*     if(a==NULL){ */
/*       *dest = NULL; break; */
/*     } */
/*     ref_t expected = a->ref; */
/*     r = a->ref.rc; */
/*     ref_t desired{a->ref.next, a->ref.rc + 1}; */
/*     if (r > 0 && a->ref.compare_exchange_strong(expected, desired) { */
/*       *dest = a; */
/*       break; */
/*     } */
/*   } */
/*   LFRCDestroy(olddest); */
/* } */

node_t* LFRCPass(node_t *v) {
  if (v!=NULL) add_to_rc(v,1);
  return v;
}

void LFRCDestroy(node_t *v) {
  #ifdef DEBUG
  if (v != NULL) {
    printf("rc of %p: %d\n", v, v->rc);
  }
  #endif
  if(v != NULL && add_to_rc(v, -1) == 1) {
    node_t *next = v->next;
    #ifdef DEBUG
    printf("From LFRCDestroy %p %d", v, v->rc);
    printf("From LFRCDestroy next %p\n", next);
    #endif
    LFRCDestroy(next);
    if (bench.free_node != NULL) {
      bench.free_node(v);
    } else {
      free(v);
    }
  }
}

long add_to_rc(node_t *v, int val) {
  long oldrc;
  while (true) {
    oldrc = v->rc;
    if(ATOMIC_CAS_MB(&v->rc, oldrc, oldrc+val))
      return oldrc;
  }
}

void LFRCStore(node_t **A, node_t *v) {
  node_t *oldval;
  if (v != NULL) add_to_rc(v, 1);
  while (true) {
    oldval = *A;
    if (ATOMIC_CAS_MB(A, oldval, v)) {
      #ifdef DEBUG
      printf("From LFRCStore %p\n", oldval);
      #endif
      LFRCDestroy(oldval);
      return;
    }
  }
}

void LFRCStoreAlloc(node_t **A, node_t *v) {
  node_t *oldval;
  while (true) {
    oldval = *A;
    if (ATOMIC_CAS_MB(A, oldval, v)) {
      #ifdef DEBUG
      if (oldval != NULL)
	printf("From LFRCStoreAlloc %p, %d\n", oldval, oldval->rc);
      #endif
      LFRCDestroy(oldval);
      return;
    }
  }
}

void LFRCCopy(node_t **v, node_t *w) {
  node_t *oldv = *v;
  if (w != NULL) add_to_rc(w,1);
  *v = w;
#ifdef DEBUG  
  printf("From LFRCCopy %p, rc: %d\n", w, w->rc);
#endif
  LFRCDestroy(oldv);
}
bool LFRCCAS(node_t **A0, node_t *old, node_t *newv) {
  if (newv != NULL) add_to_rc(newv, 1);
  if(ATOMIC_CAS_MB(A0, old, newv)) {
    LFRCDestroy(old);
    return true;
  } else {
    LFRCDestroy(newv);
    return false;
  }
}

/* bool LFRCDCAS(node_t **A0, node_t **A1, node_t *old0, node_t *old1, node_t *new0, node_t *new1) { */
/*   if (new0 != NULL) add_to_rc(new0, 1); */
/*   if (new1 != NULL) add_to_rc(new1, 1); */
/*   if (DCAS(A0, A1, old0, old1, new0, new1)) { */
/*     LFRCDestroy(old0); */
/*     LFRCDestroy(old1); */
/*     return true; */
/*   } else { */
/*     LFRCDestroy(new0); */
/*     LFRCDestroy(new1); */
/*     return false; */
/*   } */
/* } */

/* Tests */

/* should get the idx of thread passed in from d->idx, not pthread_self()!!*/
int get_thread_idx() {
  return bench.thread_id;
}
/* you can have a chance to do thread local init */
void thread_local_init(thread_data_t *d) {
  bench.thread_id = d->idx;
  bench.malloc_node = d->malloc_node;
  bench.free_node = d->free_node;
  return;
}
/* you can have a chance to do global init before any smr threads are spawned */
void smr_global_init(int thread_cnt) {
  return;
}

