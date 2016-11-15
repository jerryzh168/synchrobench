/*
 * File:
 *   harris.h
 * Author(s):
 *   Vincent Gramoli <vincent.gramoli@epfl.ch>
 * Description:
 *   Lock-free linkedlist implementation of Harris' algorithm
 *   "A Pragmatic Implementation of Non-Blocking Linked Lists" 
 *   T. Harris, p. 300-314, DISC 2001.
 *
 * Copyright (c) 2009-2010.
 *
 * harris.h is part of Synchrobench
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
#ifndef _HARRIS_H_
#define _HARRIS_H_

#include "linkedlist.h"

/* ################################################################### *
 * HARRIS' LINKED LIST
 * ################################################################### */

int harris_find(intset_t *set, val_t val);
int harris_insert(intset_t *set, val_t val);
int harris_delete(intset_t *set, val_t val);
#endif
