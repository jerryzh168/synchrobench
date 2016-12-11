/*
 *  intset.c
 *  
 *  Integer set operations (contain, insert, delete) 
 *  that call stm-based / lock-free counterparts.
 *
 *  Created by Vincent Gramoli on 1/12/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "intset.h"

int set_contains(intset_t *set, skey_t key, int transactional) {
    sval_t result;
		
    result = harris_find(set, key);

    return result;
}

inline int set_seq_add(intset_t* set, skey_t key) {
    int result;
    node_t *prev, *next;

    prev = set->head;
    next = prev->next;
    while (next->key < key) {
        prev = next;
        next = prev->next;
    }
    result = (next->key != key);
    if (result) {
        prev->next = new_node(key, next, 0);
    }
    return result;
}

int set_add(intset_t *set, skey_t key, int transactional) {
    int result;
    result = harris_insert(set, key);
    return result;
}

int set_remove(intset_t *set, skey_t key, int transactional) {
    sval_t result = 0;
    result = harris_delete(set, key);

    return result;
}

