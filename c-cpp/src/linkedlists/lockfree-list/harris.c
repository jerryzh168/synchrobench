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
 * The five following functions handle the low-order mark bit that indicates
 * whether a node is logically deleted (1) or not (0).
 *  - is_marked_ref returns whether it is marked, 
 *  - (un)set_marked changes the mark,
 *  - get_(un)marked_ref sets the mark before returning the node.
 */
inline int is_marked_ref(long i) {
    return (int) (i & (LONG_MIN+1));
}

inline long unset_mark(long i) {
	i &= LONG_MAX-1;
	return i;
}

inline long set_mark(long i) {
	i = unset_mark(i);
	i += 1;
	return i;
}

inline long get_unmarked_ref(long w) {
	return unset_mark(w);
}

inline long get_marked_ref(long w) {
	return set_mark(w);
}

/*
 * harris_search looks for value val, it
 *  - returns right_node owning val (if present) or its immediately higher 
 *    value present in the list (otherwise) and 
 *  - sets the left_node to the node owning the value immediately lower than val. 
 * Encountered nodes that are marked as logically deleted are physically removed
 * from the list, yet not garbage collected.
 */
node_t *harris_search(intset_t *set, val_t val, std::atomic<node_t *>&left_node) {
  std::atomic<node_t *> left_node_next({nullptr}), right_node({nullptr});
  //left_node_next = set->head;
  LFRCCopy(left_node_next, set->head.load());
	
search_again:
	do {
	  //node_t *t = set->head;
	  std::atomic<node_t *>t({nullptr});
	  LFRCCopy(t, set->head.load());
	  //node_t *t_next = set->head->next;
	  std::atomic<node_t *>t_next({nullptr});
	  LFRCCopy(t_next, set->head.load()->next.load());
		
		/* Find left_node and right_node */
		do {
		  // if (!is_marked_ref((long) t_next)) {
		  if (!is_marked_ref((long) LFRCPass(t_next.load()))) {
			  //(*left_node) = t;
			  LFRCStore(left_node, t.load());
			  //left_node_next = t_next;
			  LFRCStore(left_node_next, t_next.load());
			}
			//t = (node_t *) get_unmarked_ref((long) t_next);
		  LFRCStore(t, (node_t *)get_unmarked_ref((long)LFRCPass(t_next.load())));
			//if (!t->next) break;
		  if (!t.load()->next.load()) break;
			//t_next = t->next;
		  LFRCCopy(t_next, t.load()->next.load());
			//} while (is_marked_ref((long) t_next) || (t->val < val));
		} while (is_marked_ref((long)LFRCPass(t_next.load()) || (t.load()->val < val)));
		//right_node = t;
		LFRCCopy(right_node, t.load());
		
		/* Check that nodes are adjacent */
		//if (left_node_next == right_node) {
		if (left_node_next.load() == right_node.load()) {
		  // if (right_node->next && is_marked_ref((long) right_node->next))
		  if (right_node.load()->next.load() && is_marked_ref((long) right_node.load()->next.load()))
		    goto search_again;
		  else return right_node.load();
		}
		
		/* Remove one or more marked nodes */
		//if (ATOMIC_CAS_MB(&(*left_node)->next, left_node_next, right_node)) {
		if (LFRCCAS(left_node.load()->next, left_node_next.load(), right_node.load())) {
		  //if (right_node->next && is_marked_ref((long) right_node->next))
		  if (right_node.load()->next.load() && is_marked_ref((long) right_node.load()->next.load()))		  
		    goto search_again;
		  else return right_node;
		} 
		
	} while (1);
}

/*
 * harris_find returns whether there is a node in the list owning value val.
 */
int harris_find(intset_t *set, val_t val) {
  //node_t *right_node, *left_node;
  #ifdef DEBUG
  printf("--------------------Entering harris_find------------------\n");
  #endif
  std::atomic<node_t *> left_node({nullptr});
  node_t *right_node;
  //left_node = set->head;
#ifdef DEBUG  
  printf("Before LFRCCopy, left_node(%d), second(%d)\n", left_node.load(), set->head.load());
  #endif
  LFRCCopy(left_node, set->head.load());
	
  //	right_node = harris_search(set, val, &left_node);
#ifdef DEBUG
  printf("Before harris_search\n");
#endif
  right_node = harris_search(set, val, left_node);
  //	if ((!right_node->next) || right_node->val != val)
#ifdef DEBUG  
  printf("------------------Exiting harris_find---------------\n");
#endif
  if ((!right_node->next) || right_node->val != val)
		return 0;
	else 
		return 1;
}

/*
 * harris_find inserts a new node with the given value val in the list
 * (if the value was absent) or does nothing (if the value is already present).
 */
int harris_insert(intset_t *set, val_t val) {
  //node_t *newnode, *right_node, *left_node;
  std::atomic<node_t *>newnode({nullptr}), left_node({nullptr});
  node_t *right_node;
  //left_node = set->head;
  LFRCCopy(left_node, set->head.load());
	
  do {
    //right_node = harris_search(set, val, &left_node);
    right_node = harris_search(set, val, left_node);
    if (right_node->val == val)
      return 0;
    LFRCStoreAlloc(newnode, new_node(val, right_node, 0));
    /* mem-bar between node creation and insertion */
    AO_nop_full(); 
    //if (ATOMIC_CAS_MB(&left_node->next, right_node, newnode))
    if (LFRCCAS(left_node.load()->next, right_node, newnode.load()))
      return 1;
  } while(1);
}

/*
 * harris_find deletes a node with the given value val (if the value is present) 
 * or does nothing (if the value is already present).
 * The deletion is logical and consists of setting the node mark bit to 1.
 */
int harris_delete(intset_t *set, val_t val) {
  //node_t *right_node, *right_node_next, *left_node;
  std::atomic<node_t *> right_node_next({nullptr}), left_node({nullptr});
  node_t *right_node;
  //left_node = set->head;
  LFRCCopy(left_node, set->head.load());
  do {
    //right_node = harris_search(set, val, &left_node);
    right_node = harris_search(set, val, left_node);
    //if (right_node->val != val)
    if (right_node->val != val)
      return 0;
    //right_node_next = right_node->next;
    LFRCCopy(right_node_next, right_node->next);
    //if (!is_marked_ref((long) right_node_next))
    if (!is_marked_ref((long) LFRCPass(right_node_next.load())))
      //if (ATOMIC_CAS_MB(&right_node->next, right_node_next, get_marked_ref((long) right_node_next)))
      if (LFRCCAS(right_node->next, right_node_next.load(), (node_t *)get_marked_ref((long) LFRCPass(right_node_next.load()))))
				break;
	} while(1);
  //if (!ATOMIC_CAS_MB(&left_node->next, right_node, right_node_next))
  if (!LFRCCAS(left_node.load()->next, right_node, right_node_next.load()))
    //right_node = harris_search(set, right_node->val, &left_node);
    right_node = harris_search(set, right_node->val, left_node);
  return 1;
}



