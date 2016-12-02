#/bin/sh

threads=(1 2 4 8 16 20 32)

for i in "${threads[@]}"
do
    ../bin/lockfree-hashtable -L 1 -n 0 -d 10000 -t $i
done
