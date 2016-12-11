#/bin/sh
source scripts/bench.sh

test_name=contention
report_name=results/$test_name.report
rm -f $report_name
touch $report_name

lf=100
bucket_num=5
for thread in "${threads[@]}"
do
    for update in "${updates[@]}"
    do
	size=$(($bucket_num * $lf))
	echo "Thread: $thread Bucket Num: $bucket_num LF: $lf Update: $update"
	file_name=results/$test_name'_'$thread'_'$update
	bin/lockfree-hashtable -i $size -d $duration -t $thread -S $seed -u $update -l $lf -p $interval > $file_name
	python scripts/get_stats.py -f $file_name
	python scripts/get_stats.py -f $file_name >> $report_name
    done
done
