#/bin/sh
source scripts/bench.sh

report_name=results/scale.report
rm -f $report_name
touch $report_name

for thread in "${threads[@]}"
do
    for lf in "${lfs[@]}"
    do
	for update in "${updates[@]}"
	do
	    echo "Thread: $thread LF: $lf Update: $update"
	    name=results/scale'_'$thread'_'$lf'_'$update
	    bin/lockfree-hashtable -s $size -A $alternate -d $duration -t $thread -S $seed -u $update -l $lf -p $interval > $name
	    python scripts/get_stats.py -f $name
	    python scripts/get_stats.py -f $name >> $report_name
	done
    done
done
