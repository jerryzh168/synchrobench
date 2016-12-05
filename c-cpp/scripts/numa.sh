#/bin/sh
source scripts/bench.sh

# on unix
threads=(1 2 4 8 12)

# on ziqi's machine
# threads=(1 2 4 8 16 20)

rm -f results/numa.report
touch results/numa.report
for thread in "${threads[@]}"
do
    for lf in "${lfs[@]}"
    do
	for update in "${updates[@]}"
	do
	    for topo in "${topos[@]}"
	    do
		name=scale'_'$thread'_'$lf'_'$update'_'$topo
		echo "Thread: $thread LF: $lf Update: $update Topo:$topo"
		bin/lockfree-hashtable -A $alternate -d $duration -t $thread -S $seed -u $update -l $lf -p $interval -L $topo > results/"numa_$i_$lf_$update_$topo"
	    python scripts/get_stats.py -f results/$name
	    python scripts/get_stats.py -f results/$name >> results/numa.report
	    done
	done
    done
done
