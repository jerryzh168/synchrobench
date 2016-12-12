#/bin/sh
source scripts/bench.sh

test_name=smr_overhead
report_name=results/$test_name.report
rm -f $report_name
touch $report_name

mkdir -p results/$test_name

lf=50
update=0
for thread in "${threads[@]}"
do
  for i in "${itr[@]}"
  do
    bucket_num=$(($thread * 10))
    size=$(($bucket_num * $lf))
    echo "Thread: $thread Bucket Num: $bucket_num LF: $lf Update: $update"
    file_name=results/$test_name/$test_name'_'$thread'_'$bucket_num'_'$i
    bin/lockfree-hashtable -i $size -d $duration -t $thread -S $seed -u $update -l $lf -p $interval -L 1 -n 0 > $file_name
    #python scripts/get_stats.py -f $file_name
    #python scripts/get_stats.py -f $file_name >> $report_name
  done
done
