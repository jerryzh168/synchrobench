#/bin/sh
source scripts/bench.sh

test_name=op_len
report_name=results/$test_name.report
rm -f $report_name
touch $report_name

bucket_num=20
thread=20
update=20
for lf in "${lfs[@]}"
do
  for i in "${itr[@]}"
  do
    size=$(($bucket_num * $lf))
    echo "Thread: $thread Bucket Num: $bucket_num LF: $lf Update: $update"
    file_name=results/$test_name'_'$lf'_'$i
    bin/lockfree-hashtable -i $size -d $duration -t $thread -S $seed -u $update -l $lf -p $interval -L 1 -n 0 > $file_name
 #   python scripts/get_stats.py -f $file_name
 #   python scripts/get_stats.py -f $file_name >> $report_name
  done
done

