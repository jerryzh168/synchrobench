/*
 * File:
 *   test.c
 * Author(s):
 *   Vincent Gramoli <vincent.gramoli@epfl.ch>
 * Description:
 *   Concurrent accesses of a hashtable
 *
 * Copyright (c) 2009-2010.
 *
 * test.c is part of Synchrobench
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

#include "intset.h"
#include "smr.h"
#include <unistd.h>
#include <hwloc.h>
#include <iostream>
#include <stdexcept>      // std::invalid_argument
#include <stdlib.h>

#include "qsbr.h"
/* Hashtable length (# of buckets) */
unsigned int maxhtlength;

/* TOOD .. here? */
#define CACHE_LINE_SIZE 64

/* Hashtable seed */
#ifdef TLS
__thread unsigned int *rng_seed;
#else /* ! TLS */
pthread_key_t rng_seed_key;
#endif /* ! TLS */


#define DEFAULT_PROFILE_RATE 10    //in ms
int nb_threads = 0;
__thread unsigned long * seeds;

/* 
 * Returns a pseudo-random value in [1;range).
 * Depending on the symbolic constant RAND_MAX>=32767 defined in stdlib.h,
 * the granularity of rand() could be lower-bounded by the 32767^th which might 
 * be too high for given values of range and initial.
 */
/* inline long rand_range(long r) { */
/* 	int m = RAND_MAX; */
/* 	long d, v = 0; */
	
/* 	do { */
/* 		d = (m > r ? r : m); */
/* 		v += 1 + (long)(d * ((double)rand()/((double)(m)+1.0))); */
/* 		r -= m; */
/* 	} while (r > 0); */
/* 	return v; */
/* } */

/* /\* Re-entrant version of rand_range(r) *\/ */
/* inline long rand_range_re(unsigned int *seed, long r) { */
/* 	int m = RAND_MAX; */
/* 	long d, v = 0; */
	
/* 	do { */
/* 		d = (m > r ? r : m);		 */
/* 		v += 1 + (long)(d * ((double)rand_r(seed)/((double)(m)+1.0))); */
/* 		r -= m; */
/* 	} while (r > 0); */
/* 	return v; */
/* } */


static int locate_pu_affinity_helper(hwloc_obj_t root, int *child_found, int idx){
	if(root->type == HWLOC_OBJ_PU){
		int found = (*child_found)++;
		if(found == idx){
			return root->os_index;
		}else{
			return -1;
		}

	}else{
		int ret = -1;
		for(auto i = 0; i < root->arity; i++){
			ret = locate_pu_affinity_helper(root->children[i], child_found, idx);
			if(ret != -1)
				return ret;
		}

		return -1;
	}
}



static int locate_pu_affinity(hwloc_obj_t root, int num_pu, int idx){
	// idx = idx % num_pu;
	// int child_found = 0;
	// return locate_pu_affinity_helper(root, &child_found, idx);	
	while(1){
		if(root->type == HWLOC_OBJ_PU)
			return root->os_index;
		int i = idx % root->arity;
		idx = idx / root->arity;
		root = root->children[i];
	}
}

struct malloc_list{
  long nb_malloc;
  long nb_free;
  char padding[CACHE_LINE_SIZE - sizeof(long) - sizeof(long)];
} *malloc_list;

void free_node(void *n){
	free(n);
	//std::cout << "free"<<std::endl;
	malloc_list[get_thread_idx()].nb_free++;
}

void *malloc_node(unsigned int size){
	void *ret = malloc(size);
	malloc_list[get_thread_idx()].nb_malloc++;
	return ret;
}

void *malloc_node_aligned( size_t alignment, size_t size) {
	void *ret= memalign(alignment, size);
	malloc_list[get_thread_idx()].nb_malloc++;
	return ret;
}

void *test(void *data) {
	int val2, numtx, r;
	long int last = -10;
	val_t val = 0;
	int unext, mnext, cnext;
	
	thread_data_t *d = (thread_data_t *)data;
	thread_local_init(d);
	// thread_local_hpr.init(d->nb_threads, d->threads, free_node, malloc_node, d->idx);
	int physical_idx = locate_pu_affinity(d->topo_root, d->topo_pu_num, d->idx);
	mr_init_local(d->idx, nb_threads);
	/* set affinity according to topology */
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(physical_idx, &cpuset);
	int ret = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
	printf("In test %d\n", (int)d->idx);
	if(ret != 0) {
	  printf("set affinity fail\n");
		throw "set affinity fail\n";
	}
	/* Create transaction */

	TM_THREAD_ENTER();

	/* Wait on barrier */
	r = rand_range_re(d->seed, (long)100) - 1;

	barrier_cross(d->barrier);
	
	/* Is the first op an update, a move? */
	unext = (r < d->update);
	mnext = (r < d->move);
	cnext = (r >= d->update + d->snapshot);
// #ifdef ICC
	while (stop == 0) {
// #else
	// while (AO_load_full(&stop) == 0) {
// #endif /* ICC */
		
	  if (unext) { // update
	    
	    // if (mnext) { // move
	      
	    //   if (last == -1) val = rand_range_re(&d->seed, d->range);
	    //   else val = last;
	    //   val2 = rand_range_re(&d->seed, d->range);
	    //   if (ht_move(d->set, val, val2, TRANSACTIONAL)) {
					// d->nb_moved++;
					// last = -1;
	    //   }
	    //   d->nb_move++;
	      
	    // } else 
	    if (last < 0) { // add
	      // std::cout<<val<<std::endl;	      
	      val = rand_range_re(d->seed, d->range);
	      if (ht_add(d->set, val, TRANSACTIONAL)) {
					d->nb_added++;
					last = val;
	      }		
	      d->nb_add++;
	      
	    } else { // remove
	      

			/* Random computation only in non-alternated cases */
			val = rand_range_re(d->seed, d->range);


			/* Remove one random value */
	      // std::cout<<val << ":"<<last<<std::endl;
			if (ht_remove(d->set, val, TRANSACTIONAL)) {
				d->nb_removed++;
				/* Repeat until successful, to avoid size variations */
				last = -1;
			}
	      
			d->nb_remove++;
	    }
	    
	  } else { // reads
	    
	    // if (cnext) { // contains (no snapshot)
				
		  val = rand_range_re(d->seed, d->range);
			
	      if (ht_contains(d->set, val, TRANSACTIONAL)) 
					d->nb_found++;
	      d->nb_contains++;
	      
	    // } 
	 //    else { // snapshot
	      
	 //      if (ht_snapshot(d->set, TRANSACTIONAL))
		// d->nb_snapshoted++;
	 //      d->nb_snapshot++;
	      
	 //    }
	  }
	  
	  /* Is the next op an update, a move, a contains? */
	  // if (d->effective) { // a failed remove/add is a read-only tx
	    numtx = d->nb_contains + d->nb_add + d->nb_remove ;
	    unext = ((100 * (d->nb_added + d->nb_removed )) < (d->update * numtx));
	    // std::cout <<   (d->nb_added + d->nb_removed + d->nb_moved)<<":"<<d->update*numtx<<std::endl;
	    // mnext = ((100 * d->nb_moved) < (d->move * numtx));
	    // cnext = !((100 * d->nb_snapshoted) < (d->snapshot * numtx)); 
	  // } else { // remove/add (even failed) is considered as an update
	  //   r = rand_range_re(d->seed, 100) - 1;
	  //   unext = (r < d->update);
	  //   mnext = (r < d->move);
	  //   cnext = (r >= d->update + d->snapshot);
	  // }
	  
	}

	
	/* Free transaction */
	TM_THREAD_EXIT();
	// std::cout<<"exit"<<std::endl;
	return NULL;
}

#if 0
void *test2(void *data)
{
	int val, newval, last, flag = 1;
	thread_data_t *d = (thread_data_t *)data;
	
	/* Create transaction */
	TM_THREAD_ENTER();
	/* Wait on barrier */
	barrier_cross(d->barrier);
	
	last = 0; // to avoid warning
	while (stop == 0) {
		
	  val = rand_range_re(&d->seed, 100) - 1;
	  /* added for HashTables */
	  if (val < d->update) {
	    if (val >= d->move) { /* update without move */
	      if (flag) {
					/* Add random value */
					val = (rand_r(&d->seed) % d->range) + 1;
					if (ht_add(d->set, val, TRANSACTIONAL)) {
						d->nb_added++;
						last = val;
						flag = 0;
					}
					d->nb_add++;
	      } else {
					if (d->alternate) {
						/* Remove last value */
						if (ht_remove(d->set, last, TRANSACTIONAL))  
							d->nb_removed++;
						d->nb_remove++;
						flag = 1;
					} else {
						/* Random computation only in non-alternated cases */
						newval = rand_range_re(&d->seed, d->range);
						if (ht_remove(d->set, newval, TRANSACTIONAL)) {  
							d->nb_removed++;
							/* Repeat until successful, to avoid size variations */
							flag = 1;
						}
						d->nb_remove++;
					}
	      } 
	    } else { /* move */
	      val = rand_range_re(&d->seed, d->range);
	      if (ht_move(d->set, last, val, TRANSACTIONAL)) {
					d->nb_moved++;
					last = val;
	      }
	      d->nb_move++;
	    }
	  } else {
	    if (val >= d->update + d->snapshot) { /* read-only without snapshot */
	      /* Look for random value */
	      val = rand_range_re(&d->seed, d->range);
	      if (ht_contains(d->set, val, TRANSACTIONAL))
					d->nb_found++;
				d->nb_contains++;
	    } else { /* snapshot */
	      if (ht_snapshot(d->set, TRANSACTIONAL))
					d->nb_snapshoted++;
	      d->nb_snapshot++;
	    }
	  }
	}
	
	/* Free transaction */
	TM_THREAD_EXIT();
	return NULL;
}
#endif
/* void print_set(intset_t *set) { */
/* 	node_t *curr, *tmp; */
	
/* 	curr = set->head; */
/* 	tmp = curr; */
/* 	do { */
/* 		printf(" - v%d", (int) curr->key); */
/* 		tmp = curr; */
/* 		curr = tmp->next; */
/* 	} while (curr->key != VAL_MAX); */
/* 	printf(" - v%d", (int) curr->key); */
/* 	printf("\n"); */
/* } */

/* void print_ht(ht_intset_t *set) { */
/* 	int i; */
/* 	for (i=0; i < maxhtlength; i++) { */
/* 		print_set(set->buckets[i]); */
/* 	} */
/* } */

int greater_and_sub(int *sum_time, int delta){
	if(*sum_time >= delta){	
		*sum_time -= delta;
		return 1;
	}else{
		return -1;
	}
}

static int sum_pu_num_under(hwloc_obj_t root){
	if(root->type == HWLOC_OBJ_PU)
		return 1;
	else{
		int sum = 0;
		for(int i = 0; i < root->arity; i++){
			sum += sum_pu_num_under(root->children[i]);
		}
		return sum;
	}

}

int main(int argc, char **argv)
{
  set_cpu(the_cores[0]);
  ssalloc_init();
  seeds = seed_rand();
  

	struct option long_options[] = {
		// These options don't set a flag
		{"help",                      no_argument,       NULL, 'h'},
		{"duration",                  required_argument, NULL, 'd'},
		{"size",      		      required_argument, NULL, 'i'},
		{"thread-num",                required_argument, NULL, 't'},
//		{"range",                     required_argument, NULL, 'r'},
		{"seed",                      required_argument, NULL, 'S'},
		{"update-rate",               required_argument, NULL, 'u'},
		{"move-rate",                 required_argument, NULL, 'a'},
		{"snapshot-rate",             required_argument, NULL, 's'},
		{"elasticity",                required_argument, NULL, 'x'},
		{"profile", 		      required_argument, NULL, 'p'},
		{"topology-level",	      required_argument, NULL, 'L'},
		{"topology-number", 	      required_argument, NULL, 'n'},
		{NULL, 0, NULL, 0}
	};
	
	ht_intset_t *set;
	int i, c, size;
	val_t last = 0; 
	val_t val = 0;
	unsigned long reads, effreads, updates, effupds, moves, moved, snapshots, 
	snapshoted, aborts, aborts_locked_read, aborts_locked_write, 
	aborts_validate_read, aborts_validate_write, aborts_validate_commit, 
	aborts_invalid_memory, aborts_double_write,
	max_retries, failures_because_contention;
	thread_data_t *data;
	pthread_t *threads;
	pthread_attr_t attr;
	barrier_t barrier;
	struct timeval start, end;
	struct timespec timeout;
	struct timespec accounting_timeout;
	int duration = DEFAULT_DURATION;
	int initial = DEFAULT_INITIAL;
	nb_threads = DEFAULT_NB_THREADS;
//	long range = DEFAULT_RANGE;

	int topo_level = 0;
	int topo_num = 0;

	int seed = DEFAULT_SEED;
	int update = DEFAULT_UPDATE;
	long range = initial*2;

	int load_factor = DEFAULT_LOAD;
	int move = DEFAULT_MOVE;
	int snapshot = DEFAULT_SNAPSHOT;
	int unit_tx = DEFAULT_ELASTICITY;
	int alternate = DEFAULT_ALTERNATE;
	int effective = DEFAULT_EFFECTIVE;
	unsigned int profile_rate = DEFAULT_PROFILE_RATE;
	sigset_t block_set;
	
	while(1) {
		i = 0;
		c = getopt_long(argc, argv, "hAf:d:i:t:r:S:u:a:s:l:x:p:L:n:", long_options, &i);
		
		if(c == -1)
			break;
		
		if(c == 0 && long_options[i].flag == 0)
			c = long_options[i].val;
		
		switch(c) {
				case 0:
					// Flag is automatically set 
					break;
				case 'h':
					printf("intset -- STM stress test "
								 "(hash table)\n"
								 "\n"
								 "Usage:\n"
								 "  intset [options...]\n"
								 "\n"
								 "Options:\n"
								 "  -h, --help\n"
								 "        Print this message\n"
								 "  -A, --Alternate\n"
								 "        Consecutive insert/remove target the same value\n"
								 "  -f, --effective <int>\n"
								 "        update txs must effectively write (0=trial, 1=effective, default=" XSTR(DEFAULT_EFFECTIVE) ")\n"
								 "  -d, --duration <int>\n"
								 "        Test duration in milliseconds (0=infinite, default=" XSTR(DEFAULT_DURATION) ")\n"
								 "  -i, --size <int>\n"
								 "        Number of elements to insert before test (default=" XSTR(DEFAULT_INITIAL) ")\n"
								 "  -t, --thread-num <int>\n"
								 "        Number of threads (default=" XSTR(DEFAULT_NB_THREADS) ")\n"
//								 "  -r, --range <int>\n"
//								 "        Range of integer values inserted in set (default=" XSTR(DEFAULT_RANGE) ")\n"
								 "  -S, --seed <int>\n"
								 "        RNG seed (0=time-based, default=" XSTR(DEFAULT_SEED) ")\n"
								 "  -u, --update-rate <int>\n"
								 "        Percentage of update transactions (default=" XSTR(DEFAULT_UPDATE) ")\n"
								 "  -a , --move-rate <int>\n"
								 "        Percentage of move transactions (default=" XSTR(DEFAULT_MOVE) ")\n"
								 "  -s , --snapshot-rate <int>\n"
								 "        Percentage of snapshot transactions (default=" XSTR(DEFAULT_SNAPSHOT) ")\n"
								 "  -l , --load-factor <int>\n"
								 "        Ratio of keys over buckets (default=" XSTR(DEFAULT_LOAD) ")\n"
								 "  -x, --elasticity (default=4)\n"
								 "        Use elastic transactions\n"
								 "        0 = non-protected,\n"
								 "        1 = normal transaction,\n"
								 "        2 = read elastic-tx,\n"
								 "        3 = read/add elastic-tx,\n"
								 "        4 = read/add/rem elastic-tx,\n"
								 "        5 = elastic-tx w/ optimized move.\n"
								 "  -p, --profile rate(default=10ms)\n"
								 "        Profiling rate in milliseconds\n"
								 "  -L, --topology\n"									
								 "        Topology level to start placing threads for locality test\n"
								 "  -n, --object number on the topology level\n"
								 "        Object number on the topology level to place threads, default = 0\n"
								 );
					exit(0);
				case 'A':
					alternate = 1;
					break;
				case 'f':
					effective = atoi(optarg);
					break;
				case 'd':
					duration = atoi(optarg);
					break;
				case 'i':
					initial = atoi(optarg);
					range = initial*2;
					break;
				case 't':
					nb_threads = atoi(optarg);
					break;
//				case 'r':
//					range = atol(optarg);
//					break;
				case 'S':
					seed = atoi(optarg);
					break;
				case 'u':
					update = atoi(optarg);
					break;
				case 'a':
					move = atoi(optarg);
					break;
				case 's':
					snapshot = atoi(optarg);
					break;
				case 'l':
					load_factor = atoi(optarg);
					break;
				case 'x':
					unit_tx = atoi(optarg);
					break;
				case 'p':
					profile_rate = atoi(optarg);
					break;
				case 'L':
					topo_level = atoi(optarg);
					break;
				case 'n':
					topo_num = atoi(optarg);
					break;
				case '?':
					printf("Use -h or --help for help\n");
					exit(0);
				default:
					exit(1);
		}
	}

	if (!is_power_of_two(initial)) {
	  size_t initial_pow2 = pow2roundup(initial);
	  printf(
		 "** rounding up initial (to make it power of 2): old: %zu / new: %zu\n",
		 initial, initial_pow2);
	  initial = initial_pow2;
	}
	if (range < initial) {
	  range = 2 * initial;
	}
	
	hwloc_topology_t topology;
	hwloc_topology_init(&topology);
	hwloc_topology_load(topology);	
	if(topo_num >= hwloc_get_nbobjs_by_depth(topology, topo_level)){
		//throw std::invalid_argument("invalid topology num relative to level");
		printf("topo object index wrong\n");
		exit(1);
	}
	hwloc_obj_t topo_start_parent = hwloc_get_obj_by_depth(topology, topo_level, topo_num);
	assert(topo_start_parent != NULL);
	int topo_pu_num = sum_pu_num_under(topo_start_parent);
	assert(topo_pu_num >= 1);
	assert(duration >= 0);
	assert(initial >= 0);
	assert(nb_threads > 0);
	assert(range > 0 && range >= initial);
	assert(update >= 0 && update <= 100);
	assert(move >= 0 && move <= update);
	assert(snapshot >= 0 && snapshot <= (100-update));
	assert(initial < MAXHTLENGTH);
	assert(initial >= load_factor);
	
	printf("Set type     : lock-free hash table\n");
	printf("Duration     : %d\n", duration);
	printf("Initial size : %d\n", initial);
	printf("Nb threads   : %d\n", nb_threads);
	printf("Value range  : %ld\n", range);
	printf("Seed         : %d\n", seed);
	printf("Update rate  : %d\n", update);
	printf("Load factor  : %d\n", load_factor);
	printf("Move rate    : %d\n", move);
	printf("Snapshot rate: %d\n", snapshot);
	printf("Elasticity   : %d\n", unit_tx);
	printf("Alternate    : %d\n", alternate);	
	printf("Effective    : %d\n", effective);
	printf("Profile	     : %d ms\n", profile_rate);
	printf("Type sizes   : int=%d/long=%d/ptr=%d/word=%d\n",
				 (int)sizeof(int),
				 (int)sizeof(long),
				 (int)sizeof(void *),
				 (int)sizeof(uintptr_t));
	smr_global_init(nb_threads);
	timeout.tv_sec = duration / 1000;
	timeout.tv_nsec = (duration % 1000) * 1000000;
	accounting_timeout.tv_sec = profile_rate/1000;
	accounting_timeout.tv_nsec = (profile_rate % 1000) *1000000;
	
	if ((data = (thread_data_t *)malloc(nb_threads * sizeof(thread_data_t))) == NULL) {
		perror("malloc");
		exit(1);
	}
	if ((threads = (pthread_t *)malloc(nb_threads * sizeof(pthread_t))) == NULL) {
		perror("malloc");
		exit(1);
	}
	if ((malloc_list = (struct malloc_list *)aligned_alloc(CACHE_LINE_SIZE, nb_threads*sizeof(struct malloc_list))) == NULL)
	{	
		perror("aligned alloc");
		exit(1);
	}
	memset(malloc_list, 0, nb_threads*sizeof(struct malloc_list));

	// if (seed == 0)
	// 	srand((int)time(0));
	// else
	// 	srand(seed);

	if (seed == 0)
		srand48((long)time(0));
	else
		srand48(seed);

	
	maxhtlength = (unsigned int) initial / load_factor;
	printf("Before ht_new\n");
	set = ht_new();
	printf("After ht_new\n");
	
	stop = 0;
	
	// Init STM 
	printf("Initializing STM\n");
	
	TM_STARTUP();
	
	// Populate set 
	printf("Adding %d entries to set\n", initial);
	i = 0;
	maxhtlength = (int) (initial / load_factor);
	printf("maxhtlength: %d\n", maxhtlength);
	while (i < initial) {
		val = rand_range(range);
		if (ht_add(set, val, 0)) {
		  // std::cout <<"init:"<<val<<std::endl;
		  last = val;
		  i++;			
		}
	}
	size = ht_size(set);
	printf("Set size     : %d\n", size);
	printf("Bucket amount: %d\n", maxhtlength);
	printf("Load         : %d\n", load_factor);
	
	// Access set from all threads 
	barrier_init(&barrier, nb_threads + 1);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	for (i = 0; i < nb_threads; i++) {
		printf("Creating thread %d\n", i);
		data[i].threads = threads;
		data[i].nb_threads = nb_threads;
		data[i].malloc_node = malloc_node;
		data[i].free_node = free_node;
		data[i].idx = i;
		data[i].topo_root = topo_start_parent;
		data[i].topo_pu_num = topo_pu_num;
		data[i].first = last;
		data[i].range = range;
		data[i].update = update;
		data[i].load_factor = load_factor;
		data[i].move = move;
		data[i].snapshot = snapshot;
		data[i].unit_tx = unit_tx;
		data[i].alternate = alternate;
		data[i].effective = effective;
		data[i].nb_add = 0;
		data[i].nb_added = 0;
		data[i].nb_remove = 0;
		data[i].nb_removed = 0;

		data[i].nb_contains = 0;
		data[i].nb_found = 0;

		data[i].seed[0] = rand_range(range);
		data[i].seed[1] = rand_range(range);
		data[i].seed[2] = rand_range(range);
		data[i].set = set;
		data[i].barrier = &barrier;
		// data[i].failures_because_contention = 0;
		if (pthread_create(&threads[i], &attr, test, (void *)(&data[i])) != 0) {
			fprintf(stderr, "Error creating thread\n");
			exit(1);
		}
	}
	pthread_attr_destroy(&attr);

	
	//sigset_t sig_set;
	//sigemptyset(&sig_set);
	//sigaddset(&sig_set, TIMER_SIGNAL);
	//pthread_sigmask(SIG_BLOCK, &sig_set, NULL);
	//Start threads 
	barrier_cross(&barrier);
	/* install alarm signal */
	printf("STARTING...\n");
	gettimeofday(&start, NULL);
	if (duration > 0) {
		int last_sum = 0;
		while(greater_and_sub(&duration, profile_rate) > 0){	
			nanosleep(&accounting_timeout, NULL);
			int reads = 0;
			int updates = 0;
			int snapshots = 0;
			int mallocs = 0;
			int frees = 0;
			for(int i = 0 ; i < nb_threads;i++){
				reads += data[i].nb_contains;
				updates += (data[i].nb_add + data[i].nb_remove);
				mallocs += malloc_list[i].nb_malloc;
				frees += malloc_list[i].nb_free;
			}
			std::cout << "#profile: (txs, "<<reads+updates+snapshots<< ") (mallocs, " << mallocs << ") (frees, " << frees << ")" << std::endl;
			// Add custome stats here, following the same format.
			//std::cout << duration<<":"<<profile_rate<<std::endl;
			last_sum = reads + updates + snapshots;
		}
		timeout.tv_sec = duration/1000;
		timeout.tv_nsec = (duration % 1000) * 1000000;
		nanosleep(&timeout, NULL);
		//usleep(duration);
	} else {
		sigemptyset(&block_set);
		sigsuspend(&block_set);
	}
	AO_store_full(&stop, 1);
	gettimeofday(&end, NULL);
	printf("STOPPING...\n");

	//print_ht(set);
	
	// Wait for thread completion 
	for (i = 0; i < nb_threads; i++) {
		if (pthread_join(threads[i], NULL) != 0) {
			fprintf(stderr, "Error waiting for thread completion\n");
			exit(1);
		}
	}
	duration = (end.tv_sec * 1000 + end.tv_usec / 1000) - (start.tv_sec * 1000 + start.tv_usec / 1000);
	reads = 0;
	effreads = 0;
	updates = 0;
	effupds = 0;
	int memory_pressure = 0;
	for (i = 0; i < nb_threads; i++) {
		printf("Thread %d\n", i);
		printf("  #add        : %lu\n", data[i].nb_add);
		printf("    #added    : %lu\n", data[i].nb_added);
		printf("  #remove     : %lu\n", data[i].nb_remove);
		printf("    #removed  : %lu\n", data[i].nb_removed);
		printf("  #contains   : %lu\n", data[i].nb_contains);
		printf("    #found    : %lu\n", data[i].nb_found);
		printf("Memory Pressure: %ld\n", malloc_list[i].nb_malloc - malloc_list[i].nb_free);
		// printf("  #move       : %lu\n", data[i].nb_move);
		// printf("  #moved      : %lu\n", data[i].nb_moved);
		// printf("  #snapshot   : %lu\n", data[i].nb_snapshot);
		// printf("  #snapshoted : %lu\n", data[i].nb_snapshoted);
		// printf("  #aborts     : %lu\n", data[i].nb_aborts);
		// printf("    #lock-r   : %lu\n", data[i].nb_aborts_locked_read);
		// printf("    #lock-w   : %lu\n", data[i].nb_aborts_locked_write);
		// printf("    #val-r    : %lu\n", data[i].nb_aborts_validate_read);
		// printf("    #val-w    : %lu\n", data[i].nb_aborts_validate_write);
		// printf("    #val-c    : %lu\n", data[i].nb_aborts_validate_commit);
		// printf("    #inv-mem  : %lu\n", data[i].nb_aborts_invalid_memory);
		// printf("    #dup-w  : %lu\n", data[i].nb_aborts_double_write);
		// printf("    #failures : %lu\n", data[i].failures_because_contention);
		// printf("  Max retries : %lu\n", data[i].max_retries);
		// aborts += data[i].nb_aborts;
		// aborts_locked_read += data[i].nb_aborts_locked_read;
		// aborts_locked_write += data[i].nb_aborts_locked_write;
		// aborts_validate_read += data[i].nb_aborts_validate_read;
		// aborts_validate_write += data[i].nb_aborts_validate_write;
		// aborts_validate_commit += data[i].nb_aborts_validate_commit;
		// aborts_invalid_memory += data[i].nb_aborts_invalid_memory;
		// aborts_double_write += data[i].nb_aborts_double_write;
		// failures_because_contention += data[i].failures_because_contention;
		reads += data[i].nb_contains;
		effreads += data[i].nb_contains + 
		(data[i].nb_add - data[i].nb_added) + 
		(data[i].nb_remove - data[i].nb_removed);
		// (data[i].nb_move - data[i].nb_moved) +
		// data[i].nb_snapshoted;
		updates += (data[i].nb_add + data[i].nb_remove );
		effupds += data[i].nb_removed + data[i].nb_added; 
		// moves += data[i].nb_move;
		// moved += data[i].nb_moved;
		// snapshots += data[i].nb_snapshot;
		// snapshoted += data[i].nb_snapshoted;
		size += data[i].nb_added - data[i].nb_removed;
		memory_pressure += malloc_list[i].nb_malloc;
		// if (max_retries < data[i].max_retries)
		// 	max_retries = data[i].max_retries;
		printf("\n");
	}
	printf("Set size      : %d (expected: %d)\n", ht_size(set), size);
	printf("Duration      : %d (ms)\n", duration);
	printf("Memory pressure: %d\n",memory_pressure );
	printf("#txs          : %lu (%f / s)\n", reads + updates + snapshots, (reads + updates + snapshots) * 1000.0 / duration);
	
	printf("#read txs     : ");
	if (effective) {
		printf("%lu (%f / s)\n", effreads, effreads * 1000.0 / duration);
		printf("  #cont/snpsht: %lu (%f / s)\n", reads, reads * 1000.0 / duration);
	} else printf("%lu (%f / s)\n", reads, reads * 1000.0 / duration);
	
	printf("#eff. upd rate: %f \n", 100.0 * effupds / (effupds + effreads));
	
	printf("#update txs   : ");
	if (effective) {
		printf("%lu (%f / s)\n", effupds, effupds * 1000.0 / duration);
		printf("  #upd trials : %lu (%f / s)\n", updates, updates * 1000.0 / 
					 duration);
	} else printf("%lu (%f / s)\n", updates, updates * 1000.0 / duration);
	
	// printf("#move txs     : %lu (%f / s)\n", moves, moves * 1000.0 / duration);
	// printf("  #moved      : %lu (%f / s)\n", moved, moved * 1000.0 / duration);
	// printf("#snapshot txs : %lu (%f / s)\n", snapshots, snapshots * 1000.0 / duration);
	// printf("  #snapshoted : %lu (%f / s)\n", snapshoted, snapshoted * 1000.0 / duration);
	// printf("#aborts       : %lu (%f / s)\n", aborts, aborts * 1000.0 / duration);
	// printf("  #lock-r     : %lu (%f / s)\n", aborts_locked_read, aborts_locked_read * 1000.0 / duration);
	// printf("  #lock-w     : %lu (%f / s)\n", aborts_locked_write, aborts_locked_write * 1000.0 / duration);
	// printf("  #val-r      : %lu (%f / s)\n", aborts_validate_read, aborts_validate_read * 1000.0 / duration);
	// printf("  #val-w      : %lu (%f / s)\n", aborts_validate_write, aborts_validate_write * 1000.0 / duration);
	// printf("  #val-c      : %lu (%f / s)\n", aborts_validate_commit, aborts_validate_commit * 1000.0 / duration);
	// printf("  #inv-mem    : %lu (%f / s)\n", aborts_invalid_memory, aborts_invalid_memory * 1000.0 / duration);
	// printf("  #dup-w      : %lu (%f / s)\n", aborts_double_write, aborts_double_write * 1000.0 / duration);
	// printf("  #failures   : %lu\n",  failures_because_contention);
	// printf("Max retries   : %lu\n", max_retries);
	
	// Delete set 
	ht_delete(set);
	
	// Cleanup STM 
	TM_SHUTDOWN();
	
	free(threads);
	free(data);
	
	return 0;
}
