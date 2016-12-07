/*
 * File:
 *   harris.c
 * Author(s):
 *   Vincent Gramoli <vincent.gramoli@epfl.ch>
 * Description:
 *   Lock-free linkedlist implementation of Harris' algorithm
 *   "A Pragmatic Implementation of Non-Blocking Linked Lists" 
 *   T. Harris, p. 300-314, DISC 2001.
 *
 * Copyright (c) 2009-2010.
 *
 * harris.c is part of Synchrobench
 * 
 * Synchrobench is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "harris.h"

/*
 * harris_search looks for value val, it
 *  - returns right_node owning val (if present) or its immediately higher 
 *    value present in the list (otherwise) and 
 *  - sets the left_node to the node owning the value immediately lower than val. 
 * Encountered nodes that are marked as logically deleted are physically removed
 * from the list, yet not garbage collected.
 */
node_t *harris_search(intset_t *set, val_t val, node_t **left_node) {
  node_t *left_node_next = NULL, *right_node = NULL;
  #ifdef DEBUG
  printf("Entering search %p %d--------------", set->head->next, set->head->next->rc);
  #endif

  LFRCCopy(&left_node_next, set->head);

  do {		
  //node_t *t = set->head;
  node_t *t = NULL;

  LFRCCopy(&t, set->head);
  //node_t *t_next = set->head->next;
  node_t *t_next = NULL;
  LFRCCopy(&t_next, set->head->next);

  /* Find left_node and right_node */
  do {
    // if (!is_marked_ref((long) t_next)) {
    //if (!is_marked_ref((long) t_next)) {
    //(*left_node) = t;
    LFRCStore(left_node, t);
    //left_node_next = t_next;

    LFRCCopy(&left_node_next, t_next);
    //	}
    //t = (node_t *) get_unmarked_ref((long) t_next);
    //LFRCCopy(&t, (node_t *)get_unmarked_ref((long)t_next));
    LFRCCopy(&t, t_next);
    //if (!t->next) break;
    if (!t->next) {
      break;
    }
    //t_next = t->next;
    LFRCCopy(&t_next, t->next);
    //} while (is_marked_ref((long) t_next) || (t->val < val));
    //} while (is_marked_ref((long)t_next) || (t->val < val));
  } while (t->val < val);
  //right_node = t;
  LFRCCopy(&right_node, t);
  
  /* Check that nodes are adjacent */
  //if (left_node_next == right_node) {
  if (left_node_next == right_node) {
    // if (right_node->next && is_marked_ref((long) right_node->next))
    //		  if (right_node->next && is_marked_ref((long) right_node->next)) {
    #ifdef DEBUG
    printf("In search, Detroying t, t_next and left_node_next\n");
    printf("rc for right_node(%p): %d\n", right_node, right_node->rc);
#endif
    LFRCDestroy(t);
    LFRCDestroy(t_next);
    LFRCDestroy(left_node_next);
    LFRCDestroy(right_node);
#ifdef DEBUG
    printf("search %p-------------", set->head->next);
    printf("Exiting search %p %d--------------", set->head->next, set->head->next->rc);
    printf("rc for right_node(%p): %d\n", right_node, right_node->rc);
#endif
    return right_node;
  }
  printf("Destroying t_next and t: %p %p", t_next, t);
  LFRCDestroy(t);
  LFRCDestroy(t_next);
} while(1);

  
  /* Remove one or more marked nodes */
  //if (ATOMIC_CAS_MB(&(*left_node)->next, left_node_next, right_node)) {
  /* if (LFRCCAS(&(*left_node)->next, left_node_next, right_node)) { */
  /*   //if (right_node->next && is_marked_ref((long) right_node->next)) */
  /*   if (right_node->next && is_marked_ref((long) right_node->next)) { */
  /*     LFRCDestroy(t); */
  /*     LFRCDestroy(t_next); */
  /*     goto search_again; */
  /*   } */
  /*   else { */
  /*     LFRCDestroy(t); */
  /*     LFRCDestroy(t_next); */
  /*     LFRCDestroy(left_node_next); */
  /*     return right_node; */
  /*   } */
  /* }  */
}

/*
 * harris_find returns whether there is a node in the list owning value val.
 */
int harris_find(intset_t *set, val_t val) {
  //node_t *right_node, *left_node;
  #ifdef DEBUG
  printf("find %p %d %d--------------\n", set->head->next, set->head->next->rc, set->head->next->val);
  printf("--------------------Entering harris_find------------------\n");
  #endif
  node_t *left_node = NULL, *right_node = NULL;
  //left_node = set->head;
#ifdef DEBUG  
  printf("Before LFRCCopy, left_node(%d), second(%d)\n", left_node, set->head);
  #endif
  LFRCCopy(&left_node, set->head);
	
  //	right_node = harris_search(set, val, &left_node);
#ifdef DEBUG
  printf("Before harris_search\n");
#endif
  LFRCCopy(&right_node, harris_search(set, val, &left_node));
  //	if ((!right_node->next) || right_node->val != val)
#ifdef DEBUG  
  printf("------------------Exiting harris_find---------------\n");
  printf("find %p--------------", set->head->next);
#endif
  if ((!right_node->next) || right_node->val != val) {
    LFRCDestroy(left_node);
    LFRCDestroy(right_node);
    return 0;
  } else {
    LFRCDestroy(left_node);
    LFRCDestroy(right_node);
    return 1;
  }
}

/*
 * harris_find inserts a new node with the given value val in the list
 * (if the value was absent) or does nothing (if the value is already present).
 */
int harris_insert(intset_t *set, val_t val) {
  //node_t *newnode, *right_node, *left_node;
  node_t *newnode = NULL, *right_node = NULL, *left_node = NULL;
  //left_node = set->head;
#ifdef DEBUG
  printf("insert %p--------------", set->head->next);
  printf("harris_insert %d\n", val);
#endif
  LFRCCopy(&left_node, set->head);
  do {
    LFRCCopy(&right_node, harris_search(set, val, &left_node));
    if (right_node->val == val) {
      LFRCDestroy(newnode);
      LFRCDestroy(right_node);
      LFRCDestroy(left_node);
      #ifdef DEBUG
      printf("insert 1 %p--------------\n", set->head->next);
      #endif
      return 0;
    }
    newnode = new_node(val, right_node, 0);
    /* mem-bar between node creation and insertion */
    AO_nop_full(); 
    //if (ATOMIC_CAS_MB(&left_node->next, right_node, newnode))
    if (LFRCCAS(&left_node->next, right_node, newnode)) {
      LFRCDestroy(newnode);
      LFRCDestroy(right_node);
      LFRCDestroy(left_node);
      #ifdef DEBUG
      printf("insert 2 %p--------------\n", set->head->next);
      #endif
      return 1;
    }
  } while(1);
}

/*
 * harris_find deletes a node with the given value val (if the value is present) 
 * or does nothing (if the value is already present).
 * The deletion is logical and consists of setting the node mark bit to 1.
 */
int harris_delete(intset_t *set, val_t val) {
  //node_t *right_node, *right_node_next, *left_node;
  node_t *right_node = NULL, *right_node_next = NULL, *left_node = NULL;
  //left_node = set->head;
  LFRCCopy(&left_node, set->head);

  LFRCCopy(&right_node, harris_search(set, val, &left_node));
  //printf("In harris delete, harris search returned %p %p\n", right_node, left_node);
  if (right_node->val != val) {
      LFRCDestroy(right_node);
      LFRCDestroy(right_node_next);
      LFRCDestroy(left_node);
      return 0;
    }
  do {
    LFRCCopy(&right_node_next, right_node->next);
    if (LFRCCAS(&left_node->next, right_node, right_node_next)) {
  //printf("In delete right_node %p, %d\n", right_node, right_node->rc);
      break;
    }
  } while(1);

  LFRCDestroy(right_node);
  LFRCDestroy(right_node_next);
  LFRCDestroy(left_node);
  return 1;
}
