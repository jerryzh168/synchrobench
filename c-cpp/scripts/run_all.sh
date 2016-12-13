#/bin/bash
make clean
make lockfree
./scripts/smr_overhead.sh
./scripts/rw_dominant.sh
./scripts/op_len.sh
./scripts/contention.sh
