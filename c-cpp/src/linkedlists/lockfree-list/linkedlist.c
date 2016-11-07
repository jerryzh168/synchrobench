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

node_t *new_node(val_t val, node_t *next, int transactional)
{
  node_t *node;

  if (transactional) {
    node = (node_t *)MALLOC(sizeof(node_t));
  } else {
    node = (node_t *)malloc(sizeof(node_t));
  }
  if (node == NULL) {
	perror("malloc");
	exit(1);
  }
  node->val = val;
  node->next = next;
  node->rc = 1;
  return node;
}

intset_t *set_new()
{
  intset_t *set;
  std::atomic<node_t *> min{nullptr}, max{nullptr};
	
  if ((set = (intset_t *)malloc(sizeof(intset_t))) == NULL) {
    perror("malloc");
    exit(1);
  }
  //  LFRCStoreAlloc(max, std::atomic<node_t *>(new_node(VAL_MAX, NULL, 0)));  
  LFRCStoreAlloc(max, new_node(VAL_MAX, NULL, 0)); // node_t *
  LFRCStoreAlloc(min, new_node(VAL_MIN, max.load(), 0));
  LFRCCopy(set->head, min.load());

  return set;
}

void set_delete(intset_t *set)
{
  std::atomic<node_t *> node, next;

  LFRCCopy(node, set->head);
  while (node.load() != NULL) {
    LFRCCopy(next, node.load()->next);
    //free(node);
    LFRCCopy(node, next.load());
  }
  free(set);
}

int set_size(intset_t *set)
{
  int size = 0;
  std::atomic<node_t *>node;

  /* We have at least 2 elements */
  //node = set->head->next;
  LFRCCopy(node, set->head.load()->next);
  while (node.load()->next != NULL) {
    size++;
    //node = node->next;
    LFRCCopy(node, node.load()->next);
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
  if(v != NULL && add_to_rc(v, -1) == 1) {
      LFRCDestroy(v->next);
      free(v);
  }
}

long add_to_rc(node_t *v, int val) {
  long oldrc;
  while (true) {
    oldrc = v->rc;
    if (v->rc.compare_exchange_strong(oldrc, oldrc+val))
      return oldrc;
  }
}

/* void LFRCStore(node_t **A, node_t *v) { */
/*   node_t *oldval; */
/*   if (v != NULL) add_to_rc(v, 1); */
/*   while (true) { */
/*     oldval = *A; */
/*     if (std::atomic_compare_exchange_strong(A, oldval, v)) { */
/*       LFRCDestroy(oldval); */
/*       return; */
/*     } */
/*   } */
/* } */

void LFRCStoreAlloc(std::atomic<node_t *> &A, node_t *v) {
  node_t *oldval;
  while (true) {
    oldval = A.load();
    if (A.compare_exchange_strong(oldval, v)) {
      LFRCDestroy(oldval);
      return;
    }
  }
}

void LFRCCopy(std::atomic<node_t *> &v, node_t *w) {
  node_t *oldv = v.load();
  if (w != NULL) add_to_rc(w,1);
  v.store(w);
  LFRCDestroy(oldv);
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
