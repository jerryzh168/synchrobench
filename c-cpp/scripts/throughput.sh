#/bin/sh
source scripts/bench.sh

report_name=results/throughput.report
rm -f $report_name
touch $report_name
for thread in "${threads[@]}"
do
    for lf in "${lfs[@]}"
    do
	for update in "${updates[@]}"
	do
	    for topo in "${topos[@]}"
	    do
		name=throughput'_'$thread'_'$lf'_'$update'_'$topo
		echo "Thread: $thread LF: $lf Update: $update Topo:$topo"
		bin/lockfree-hashtable -s $size -A $alternate -d $duration -t $thread -S $seed -u $update -l $lf -p $interval -L $topo > results/$name
		python scripts/get_stats.py -e -f results/$name
		python scripts/get_stats.py -f results/$name >> $report_name
	    done
	done
    done
done
