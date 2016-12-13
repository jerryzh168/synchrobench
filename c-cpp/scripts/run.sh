#/bin/bash

make clean && make lockfree
./scripts/rw_dominant.sh
