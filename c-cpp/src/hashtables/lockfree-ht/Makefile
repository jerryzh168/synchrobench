ROOT = ../../..

include $(ROOT)/common/Makefile.common

ifeq ($(STM),SEQUENTIAL)
  BINS = $(BINDIR)/sequential-hashtable
else ifeq ($(STM),LOCKFREE)
  BINS = $(BINDIR)/lockfree-hashtable
else
  BINS = $(BINDIR)/$(STM)-hashtable
endif

LLREP = $(ROOT)/src/linkedlists/lockfree-list
# CFLAGS += -std=gnu89
CXXFLAGS = $(CFLAGS)
CXXFLAGS += -std=c++11 -mcx16 -O3
# Need to add export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib to your .bashrc

.PHONY:	all clean

all:	main

linkedlist.o:
	$(CXX) $(CXXFLAGS) -c -o $(BUILDIR)/linkedlist.o $(LLREP)/linkedlist.c

harris.o: $(LLREP)/linkedlist.h linkedlist.o
	$(CXX) $(CXXFLAGS) -c -o $(BUILDIR)/harris.o $(LLREP)/harris.c

ll-intset.o: $(LLREP)/linkedlist.h linkedlist.o harris.o
	$(CXX) $(CXXFLAGS) -c -o $(BUILDIR)/ll-intset.o $(LLREP)/intset.c

hashtable.o: $(LLREP)/linkedlist.h linkedlist.o harris.o ll-intset.o
	$(CXX) $(CXXFLAGS) -c -o $(BUILDIR)/hashtable.o hashtable.c

intset.o: $(LLREP)/linkedlist.h harris.o hashtable.o 
	$(CXX) $(CXXFLAGS) -c -o $(BUILDIR)/intset.o intset.c

test.o: linkedlist.o harris.o intset.o hashtable.o intset.o
	$(CXX) $(CXXFLAGS) -c -o $(BUILDIR)/test.o test.c

main: linkedlist.o harris.o intset.o hashtable.o intset.o test.o 
	$(CXX) $(CXXFLAGS) $(BUILDIR)/linkedlist.o $(BUILDIR)/harris.o $(BUILDIR)/ll-intset.o $(BUILDIR)/hashtable.o $(BUILDIR)/intset.o $(BUILDIR)/test.o -o $(BINS) $(LDFLAGS)

clean:
	-rm -f $(BINS)
