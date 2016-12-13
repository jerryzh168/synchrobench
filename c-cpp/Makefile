.PHONY:	all

LFBENCHS = src/linkedlists/lockfree-list src/hashtables/lockfree-ht
MALLOC=TC

.PHONY:	clean all $(BENCHS) $(LBENCHS)

all:	lockfree

lockfree: clean-build
	for dir in $(LFBENCHS); do \
	$(MAKE) "STM=LOCKFREE" -C $$dir; \
	done

clean-build: 
	rm -rf build

clean:
	rm -rf build bin results/*

$(BENCHS):
	$(MAKE) -C $@ $(TARGET)

$(LBENCHS):
	$(MAKE) -C $@ $(TARGET)
