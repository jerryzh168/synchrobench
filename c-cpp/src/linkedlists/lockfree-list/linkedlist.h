/*
 *  linkedlist.h
 *  
 *
 *  Created by Vincent Gramoli on 1/12/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */
#ifndef _LINKEDLIST_H
#define _LINKEDLIST_H

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
#include <atomic>

#include "tm.h"

//#define DEBUG

#ifdef DEBUG
#define IO_FLUSH                        fflush(NULL)
/* Note: stdio is thread-safe */
#endif

#define DEFAULT_DURATION                10000
#define DEFAULT_INITIAL                 256
#define DEFAULT_NB_THREADS              1
#define DEFAULT_RANGE                   0x7FFFFFFF
#define DEFAULT_SEED                    0
#define DEFAULT_UPDATE                  20
#define DEFAULT_ELASTICITY							4
#define DEFAULT_ALTERNATE								0
#define DEFAULT_EFFECTIVE								1

#define XSTR(s)                         STR(s)
#define STR(s)                          #s

#define ATOMIC_CAS_MB(a, e, v)          (AO_compare_and_swap_full((volatile AO_t *)(a), (AO_t)(e), (AO_t)(v)))

static volatile AO_t stop;

#define TRANSACTIONAL                   d->unit_tx

typedef intptr_t val_t;
#define VAL_MIN                         INT_MIN
#define VAL_MAX                         INT_MAX

typedef struct node {
  val_t val;
  std::atomic<struct node *>next{nullptr};
  std::atomic<long> rc{1};
} node_t;


struct intset_t {
  std::atomic<node_t *> head{nullptr};
};

node_t *new_node(val_t val, node_t *next, int transactional);
intset_t *set_new();
void set_delete(intset_t *set);
int set_size(intset_t *set);

// LFRC
//void LFRCLoad(node_t **dest, node_t **A);
node_t* LFRCPass(node_t *v);
void LFRCDestroy(node_t *v);
long add_to_rc(node_t *v, int val);
void LFRCStore(std::atomic<node_t *> &A, node_t *v);
void LFRCStoreAlloc(std::atomic<node_t *> &A, node_t *v);
void LFRCCopy(std::atomic<node_t *> &v, node_t *w);
bool LFRCCAS(std::atomic<node_t *> &A0, node_t *old, node_t *newv);

//bool LFRCDCAS(node_t **A0, node_t **A1, node_t *old0, node_t *old1, node_t *new0, node_t *new1);

#endif
