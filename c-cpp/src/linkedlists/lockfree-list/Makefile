ROOT = ../../..

include $(ROOT)/common/Makefile.common

ifeq ($(STM),SEQUENTIAL)
  BINS = $(BINDIR)/sequential-linkedlist
else ifeq ($(STM),LOCKFREE)
  BINS = $(BINDIR)/lockfree-linkedlist
else
  BINS = $(BINDIR)/$(STM)-linkedlist
endif
CXXFLAGS = $(CFLAGS)
#CFLAGS += -std=gnu89
CXXFLAGS += -std=c++11 -mcx16 -g -O3

.PHONY:	all clean

all:	main

linkedlist.o:
	$(CXX) $(CXXFLAGS) -c -o $(BUILDIR)/linkedlist.o linkedlist.c

harris.o: linkedlist.h linkedlist.o
	$(CXX) $(CXXFLAGS) -c -o $(BUILDIR)/harris.o harris.c

intset.o: linkedlist.h harris.h
	$(CXX) $(CXXFLAGS) -c -o $(BUILDIR)/intset.o intset.c

test.o: linkedlist.h harris.h intset.h
	$(CXX) $(CXXFLAGS) -c -o $(BUILDIR)/test.o test.c

main: linkedlist.o harris.o intset.o test.o $(TMILB)
	$(CXX) $(CXXFLAGS) $(BUILDIR)/linkedlist.o $(BUILDIR)/harris.o $(BUILDIR)/intset.o $(BUILDIR)/test.o -o $(BINS) $(LDFLAGS)

clean:
	-rm -f $(BINS)
