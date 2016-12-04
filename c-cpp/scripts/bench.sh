#/bin/sh

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
