#/bin/sh
source scripts/bench.sh

rm -f results/scale.report
touch results/scale.report

# TODO delete
threads=(1 2 4 8)

for thread in "${threads[@]}"
do
    for lf in "${lfs[@]}"
    do
	for update in "${updates[@]}"
	do
	    echo "Thread: $thread LF: $lf Update: $update"
	    name=scale'_'$thread'_'$lf'_'$update
	    bin/lockfree-hashtable -A $alternate -d $duration -t $thread -S $seed -u $update -l $lf -p $interval > results/$name
	    python scripts/get_stats.py -f results/$name
	    python scripts/get_stats.py -f results/$name >> results/scale.report
	done
    done
done



