#/bin/sh
source scripts/bench.sh

# on unix
threads=(1 2 4 8 12)

# on ziqi's machine
# threads=(1 2 4 8 16 20)

report_name=results/numa.report
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
		name=results/numa'_'$thread'_'$lf'_'$update'_'$topo
		echo "Thread: $thread LF: $lf Update: $update Topo:$topo"
		bin/lockfree-hashtable -i $size -d $duration -t $thread -S $seed -u $update -l $lf -p $interval -L $topo > $name
		python scripts/get_stats.py -f $name
		python scripts/get_stats.py -f $name >> $report_name
	    done
	done
    done
done
