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

skey_t set_contains(intset_t *set, skey_t key, int transactional) {
    skey_t result;
	
    result = harris_find(set, key);
    return result;
}

int set_add(intset_t *set, skey_t key, int transactional) {
    int result;

    result = harris_insert(set, key, 0);// hashset
    return result;
}

skey_t set_remove(intset_t *set, skey_t key, int transactional) {
    skey_t result = 0;

    result = harris_delete(set, key);
    return result;
}

