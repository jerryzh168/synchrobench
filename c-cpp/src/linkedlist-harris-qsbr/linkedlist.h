/*
 *  linkedlist.h
 *  
 *
 *  Created by Vincent Gramoli on 1/12/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */
#ifndef _LINKEDLIST_QSBR_H
#define _LINKEDLIST_QSBR_H
#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>

#include <atomic_ops.h>
#include "atomic_ops_if.h"

#include "common.h"
#include "utils.h"
#include "measurements.h"
#include "ssalloc.h"
#include "tm.h"

#ifdef DEBUG
#define IO_FLUSH                        fflush(NULL)
/* Note: stdio is thread-safe */
#endif

#define DEFAULT_ALTERNATE		0
#define DEFAULT_EFFECTIVE		1
#define DEFAULT_SEED                    0

//static volatile int stop;
static volatile long unsigned int stop;

#define TRANSACTIONAL                   4

typedef volatile struct node {
    skey_t key;
    volatile struct node* next;
    uint8_t padding32[8];
#if defined(DO_PAD)
    uint8_t padding[CACHE_LINE_SIZE - sizeof(sval_t) - sizeof(skey_t) - sizeof(struct node*)];
#endif
} node_t;

typedef ALIGNED(CACHE_LINE_SIZE) struct intset {
    node_t *head;
    uint8_t padding[CACHE_LINE_SIZE - sizeof(node_t*)];
} intset_t;

struct thread_local_info_t {
  int thread_id;
  void *(*malloc_node)(unsigned int);
  void (*free_node)(void *);
  void *(*malloc_node_aligned)(size_t, size_t);// args: alignment, size  
};
extern __thread thread_local_info_t bench;

node_t *new_node(skey_t key, node_t *next, int initializing);
intset_t *set_new();
void set_delete(intset_t *set);
#endif