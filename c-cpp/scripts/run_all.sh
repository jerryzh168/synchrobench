#/bin/bash
TEST=epoch
make clean
make lockfree
./scripts/smr_overhead.sh
./scripts/rw_dominant.sh
./scripts/op_len.sh
./scripts/contention.sh
python scripts/get_stats.py -d results/contention/
python scripts/get_stats.py -d results/smr_overhead/
python scripts/get_stats.py -d results/rw_dominant/
python scripts/get_stats.py -d results/op_len/
rm -r ../../results/$TEST
cp -r results ../../results/$TEST
