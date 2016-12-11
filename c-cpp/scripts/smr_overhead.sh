#/bin/sh
source scripts/bench.sh

test_name=smr_overhead
report_name=results/$test_name.report
rm -f $report_name
touch $report_name

lf=1
update=0
for thread in "${threads[@]}"
do
    bucket_num=$(($thread * 10))
    size=$(($bucket_num * $lf))
    echo "Thread: $thread Bucket Num: $bucket_num LF: $lf Update: $update"
    file_name=results/$test_name'_'$thread'_'$bucket_num
    bin/lockfree-hashtable -i $size -d $duration -t $thread -S $seed -u $update -l $lf -p $interval > $file_name
    python scripts/get_stats.py -f $file_name
    python scripts/get_stats.py -f $file_name >> $report_name
done
