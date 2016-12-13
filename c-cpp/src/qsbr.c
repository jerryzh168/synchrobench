/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright (c) Thomas E. Hart.
 */
#include <iostream>
#include "qsbr.h"
#include "mr.h"
#include <stdio.h>
#include "atomic_ops_if.h"
#include "utils.h"
#include "ssalloc.h"
#include "lock_if.h"

#define NOT_ENTERED 0
#define ENTERED 1
#define DONE 2

struct qsbr_globals {
    ALIGNED(CACHE_LINE_SIZE) ptlock_t update_lock;
    ALIGNED(CACHE_LINE_SIZE) int global_epoch ;
    char padding[CACHE_LINE_SIZE - sizeof(ptlock_t) - sizeof(int)];
    // OANA IGOR Should we pad here? It's not clear how big the padding should be...

    ALIGNED(CACHE_LINE_SIZE) int gc_epoch;
};

ALIGNED(CACHE_LINE_SIZE) struct qsbr_globals *qg ;

struct qsbr_data {
    /* EBR per-thread data:
     *  limbo_list: three lists of nodes awaiting physical deletion, one
     *              for each epoch
     *  in_critical: flag telling us whether we're in a critical section
     *               with respect to memory reclamation
     *  entries_since_update: the number of times we have entered a critical 
     *                        section in the current epoch since trying to
     *                        update the global epoch
     *  epoch: the local epoch
     */
    mr_node_t *limbo_list [N_EPOCHS];
    int entries_since_update;
    int epoch;
    int in_critical;
    int rcount;
    char padding[CACHE_LINE_SIZE - 4 * sizeof(int) - N_EPOCHS * sizeof(mr_node_t*)];

    int local_epoch;
};

struct qsbr_aux_data {

    uint64_t thread_index;
    uint64_t nthreads;
    char padding[CACHE_LINE_SIZE - 2*sizeof(uint64_t)];
};

typedef ALIGNED(CACHE_LINE_SIZE) struct qsbr_data qsbr_data_t;
typedef ALIGNED(CACHE_LINE_SIZE) struct qsbr_aux_data qsbr_aux_data_t;

qsbr_data_t *qd;
__thread qsbr_aux_data_t qad;

/*
 * update_gc_epoch() - Updates the GC epoch counter
 *
 * This function does not require any memory barrier since by default epoch
 * prevents reclaiming, and it is only used to guarantee progress
 */
void update_gc_epoch() 
{
    qg->gc_epoch += 1;
    return;
}

/*
 * update_local_epoch() - Updates the local epoch for each thread
 */
void update_local_epoch(int thread_id) 
{
    // Copies it from the global epoch
    // This also does not need memory barrier because even if this write is
    // delayed the worst is to have delaied deallocation
    qd[thread_id]->local_epoch = qg->gc_epoch;

    return;
}

int update_epoch()
{
    int curr_epoch;
    int i;
    
    if (!TRYLOCK(&qg->update_lock)) {
        /* Someone could be preempted while holding the update lock. Give
         * them the CPU. */
        return 0;
    }
    
    /* If any CPU hasn't advanced to the current epoch, abort the attempt. */
    curr_epoch = qg->global_epoch;
    for (i = 0; i < qad.nthreads; i++) {
        if (qd[i].in_critical == 1 && 
            qd[i].epoch != curr_epoch &&
            i != qad.thread_index) {
            UNLOCK(&qg->update_lock);
            return 0;
        }
    }
    
    /* Update the global epoch. */
    qg->global_epoch = (curr_epoch + 1) % N_EPOCHS;
    
    UNLOCK(&(qg->update_lock));
    return 1;
}

void mr_init_local(uint8_t thread_index, uint8_t nthreads) {
    qad.thread_index = thread_index;
    qad.nthreads = nthreads;
}

void mr_init_global(uint8_t nthreads) {
    int i, j;
    
    qg = (struct qsbr_globals *)malloc(sizeof(struct qsbr_globals));

    qd = (qsbr_data_t *)malloc(nthreads * sizeof(qsbr_data_t));

    for (i = 0; i < nthreads; i++) {
        qd[i].epoch = 0;
        // Local epoch is initialized to 0 to be consistent 
        // with the init value of the global GC epoch
        qd[i].local_epoch = 0;
        qd[i].in_critical = 1;
        qd[i].rcount = 0;
        for (j = 0; j < N_EPOCHS; j++)
            qd[i].limbo_list[j] = NULL;
    }
    
    qg->global_epoch = 1;

    // This will be increased for certain amount of operations
    qg->gc_epoch = 0;

    INIT_LOCK(&(qg->update_lock));
}

void mr_thread_exit()
{
    qd[qad.thread_index].in_critical = 0;

    int retries = 0;
    while(qd[qad.thread_index].rcount > 0 && retries < MAX_EXIT_RETRIES) {
        quiescent_state(FUZZY);
        sched_yield();
        retries++;
    }
}

void mr_reinitialize()
{
    int i;
    
    for (i = 0; i < qad.nthreads; i++) {
        qd[i].epoch = 0;
        // We also set global epoch to 0
        qd[i].local_epoch = 0;
        qd[i].in_critical = 1;
        qd[i].rcount = 0;
    }

    qg->global_epoch = 1;
    // This is set to 0 to be consistent with local epoch
    qg->gc_epoch = 0;
    INIT_LOCK(&(qg->update_lock));
}

/* Processes a list of callbacks.
 *
 * @list: Pointer to list of node_t's.
 */
void process_callbacks(mr_node_t **list)
{
    mr_node_t *next;
    uint64_t num = 0;
    
    MEM_BARRIER;

    for (; (*list) != NULL; (*list) = next) {
        next = (*list)->mr_next;

        ssfree_alloc(0, (*list)->actual_node);
        ssfree_alloc(1, *list);
        num++;
    }

    /* Update our accounting information. */
    qd[qad.thread_index].rcount -= num;
}

/*
 * free_garbage_node() - Frees garbage nodes for all threads
 *
 * This function goes through all thread's local data and then free them
 */
void free_garbage_node() 
{
    
}

/*
 * compute_smallest_epoch() - This function computes smallest epoch
 *                            and return it
 */
int compute_smallest_epoch() 
{
    // if there is no thread in the system return INT_MAX as epoch (recycle everything)
    if(nthreads == 0) {
        return 0x7FFFFFFF;
    }

    // Use it as min and keep updating
    int min_epoch = qd[0].local_epoch;

    // Go through all threads' local data
    for (i = 1; i < nthreads; i++) {
        if(qd[i].min_epoch < min_epoch) {
            min_epoch = qd[i].local_epoch;
        }
    }

    return min_epoch;
}

/* 
 * quiescent_state() - Informs other threads that this thread has passed 
 *                     through a quiescent state
 *
 * If all threads have passed through a quiescent state since the last time
 * this thread processed it's callbacks, proceed to process pending callbacks
 *
 * This function does not wait for other threads; instead it checks whether
 * other threads have passed the quiescent state, and if they are just free
 * all garbage since the last moment
 */
void quiescent_state(int blocking)
{
    // This updates local epoch
    update_local_epoch();

    qsbr_data_t *t = &(qd[qad.thread_index]);
    int epoch;
    int orig;

 retry:    
    epoch = qg->global_epoch;
	//std::cout <<"epoch:"<< epoch <<std::endl;
    if (t->epoch != epoch) { /* New epoch. */
        /* Process callbacks for old 'incarnation' of this epoch. */
        process_callbacks(&(t->limbo_list[epoch]));
        t->epoch = epoch;
    } else {
      //printf("[%d] old epoch\n", qad.thread_index);
        orig = t->in_critical;
        t->in_critical = 0;
        int res = update_epoch();
        // fprintf(stderr, "Update epoch returned %d\n", res);
        if (res) {
            t->in_critical = orig; 
            MEM_BARRIER;
            epoch = qg->global_epoch;
            if (t->epoch != epoch) {
                process_callbacks(&(t->limbo_list[epoch]));
                t->epoch = epoch;
            }
            return;
        } else if (blocking) {
            t->in_critical = orig; 
            MEM_BARRIER;
            sched_yield();
            goto retry;
        }
        t->in_critical = orig; 
        MEM_BARRIER;

    }
    
    return;
}

/* 
 * free_node_later() - Links the node into the per-thread list of pending deletions
 */
void free_node_later (void *q)
{
    qsbr_data_t *t = &(qd[qad.thread_index]);

    //Use allocator 0 for regular nodes and allocator 1 for mr_nodes
    mr_node_t* wrapper_node = (mr_node_t *)ssalloc_alloc(1, sizeof(mr_node_t));
    
    while (wrapper_node == NULL) {
        fprintf(stderr, "Could not allocate mr_node\n");
        quiescent_state(NOT_FUZZY);
        wrapper_node = (mr_node_t *)ssalloc_alloc(1, sizeof(mr_node_t));
    }

    wrapper_node->actual_node = q;

    wrapper_node->mr_next = t->limbo_list[t->epoch];
    t->limbo_list[t->epoch] = wrapper_node;
    t->rcount++;
}
