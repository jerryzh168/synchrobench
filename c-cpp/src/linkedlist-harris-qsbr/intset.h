/*
 *  linkedlist.h
 *  
 *
 *  Created by Vincent Gramoli on 1/12/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "harris.h"

int set_contains(intset_t *set, skey_t key, int transactional);
int set_add(intset_t *set, skey_t key, int transactional);
int set_remove(intset_t *set, skey_t key, int transactional);

