#/bin/bash

make clean && make lockfree
./scripts/rw_dominant.sh
python scripts/get_stats.py -d results/rw_dominant/
