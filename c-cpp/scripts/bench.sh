#/bin/sh

<<<<<<< HEAD
threads=(1 2 4 8)

DURATION=10000
mkdir -p results
for i in "${threads[@]}"
do
    echo "threads $i"
    bin/lockfree-hashtable -L 1 -n 0 -d $DURATION -t $i > results/"numa_$i"
done

for i in "${threads[@]}"
do
    python scripts/get_stats.py -f results/"numa_$i"
done
=======
# Operations: Update -> Move or Add or Reamove, Move = Add + Remove
#   with alternate == 1, consequtive add/remove will operate on the same val
#             Reads -> Contains
# Snapshot not supported on Lockfree mode

# Variables
## number of threads

## duration

## set size
### We care more about the distribution of the hashset values rather than the size, because it's hard to exhaust 128GB memory in the test machine
### node size?

## percentage of update tx [0-100] - controls write contention
## percentage move rate tx [0-update rate]
## load factor, ratio of keys over buckets [< initial], average linkedlist length - controls overall contention
## numa

# Other Args
## Number of buckets = initial size / load_factor
## elasticity
## initial: initial size of the test set
## d->effective, a failed remove/add considered read-only tx or update

# Experiments
## Throughput
### Description: Measure the throuhgput of reclamation operations versus the throughput of data strucutres over time -profile
### Metric: #txs per second and reclamation operations per second
#### reclamation operations: specific to smr technique, need to add a function to gather this information
### data structure throughput: #txs per second as reported by the program
### Workload
#### 1. update 0, 20 or 80 (read only, normal, heavy write)
#### 2. load factor 1, 4, 8(low contention, medium contention, high contention)

## Scalability
### Description: Varying the number of threads and see the impact on different parameter settings
### Metric: #txs per second
### Workload
#### 1. thread number 1, 2, 4, 8, 16, 32, 40, 64
#### 2. update 0, 20 or 80 (read only, normal, heavy write)
#### 3. load factor 1, 4, 8(low contention, medium contention, high contention)

## Numa
### Description: Varying the distribution of the threads
### Metric: #txs per second
### Workload
#### 1. distribute threads to all sockets, or distribute to one socket
#### 2. thread number 1, 2, 4, 8...number of threads in one socket
#### 3. update 0, 20 or 80 (read only, normal, heavy write)
#### 4. load factor 1, 4, 8(low contention, medium contention, high contention)

# Updates
updates=(0 20 80)

# size
size=100000

# Load factor
lfs=(1 100 10000)

# numa
topos=(0 1)

# Threads
threads=(1 2 4 8 16 32 40 64)

# Duration
duration=10000

# Profile interval
interval=10

# Seed
seed=1

# Alternate
alternate=0

# for i in "${threads[@]}"
# do
#     echo "threads $i"
#     bin/lockfree-hashtable -L 1 -n 0 -d $DURATION -t $i > results/"numa_$i"
# done

# for i in "${threads[@]}"
# do
#     python scripts/get_stats.py -f results/"numa_$i"
# done
>>>>>>> master
