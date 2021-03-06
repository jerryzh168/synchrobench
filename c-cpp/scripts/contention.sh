#/bin/sh
source scripts/bench.sh

test_name=contention
report_name=results/$test_name.report
rm -f $report_name
touch $report_name

mkdir -p results/$test_name

lf=50
bucket_num=5
for thread in "${threads[@]}"
do
    for update in "${updates[@]}"
    do
     	for i in "${itr[@]}"
	do
		size=$(($bucket_num * $lf))
		echo "Thread: $thread Bucket Num: $bucket_num LF: $lf Update: $update"
		file_name=results/$test_name/$test_name'_'$update'_'$thread'_'$i
		bin/lockfree-hashtable -i $size -d $duration -t $thread -S $seed -u $update -l $lf -p $interval -L 1 -n 0 > $file_name
	done
	#python scripts/get_stats.py -f $file_name
	#python scripts/get_stats.py -f $file_name >> $report_name
    done
done
