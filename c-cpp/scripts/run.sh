#/bin/bash

make clean && make lockfree
./scripts/rw_dominant.sh
python scripts/get_status.py -d results/rw_dominant/
