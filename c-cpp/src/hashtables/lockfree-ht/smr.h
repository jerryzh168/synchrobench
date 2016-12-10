#ifndef _SMR_H_

#define _SMR_H_
#include "intset.h"
#include <hwloc.h>

typedef struct barrier {
	pthread_cond_t complete;
	pthread_mutex_t mutex;
	int count;
	int crossing;
} barrier_t;

typedef  struct node node_t;
typedef struct thread_data {
  val_t first;
	void *(*malloc_node)(unsigned int);
	void (*free_node)(node_t *);

	int idx;
	int nb_threads;
	pthread_t *threads;
	hwloc_obj_t topo_root;
	int topo_pu_num;

	long range;
	int update;
	int move;
	int snapshot;
	int unit_tx;
	int alternate;
	int effective;
	unsigned long nb_add;
	unsigned long nb_added;
	unsigned long nb_remove;
	unsigned long nb_removed;
	unsigned long nb_contains;
	/* added for HashTables */
	unsigned long load_factor;
	// unsigned long nb_move;
	// unsigned long nb_moved;
	// unsigned long nb_snapshot;
	// unsigned long nb_snapshoted;
	/* end: added for HashTables */
	unsigned long nb_found;
	// unsigned long nb_aborts;
	// unsigned long nb_aborts_locked_read;
	// unsigned long nb_aborts_locked_write;
	// unsigned long nb_aborts_validate_read;
	// unsigned long nb_aborts_validate_write;
	// unsigned long nb_aborts_validate_commit;
	// unsigned long nb_aborts_invalid_memory;
	// unsigned long nb_aborts_double_write;
	// unsigned long max_retries;
	unsigned short seed[3];
	ht_intset_t *set;
	barrier_t *barrier;
	// unsigned long failures_because_contention;
} thread_data_t;


/* should get the idx of thread passed in from d->idx, not pthread_self()!!*/
int get_thread_idx();
/* you can have a chance to do thread local init */
void thread_local_init(thread_data_t *d);
/* you can have a chance to do global init before any smr threads are spawned */
void smr_global_init(int thread_cnt);

void thread_local_finit();
#endif
