ROOT = ../../..

include $(ROOT)/common/Makefile.common

BINS = $(BINDIR)/lockfree-linkedlist
CXXFLAGS = $(CFLAGS)
CXXFLAGS += -std=c++11 -g -O3

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

main: linkedlist.o harris.o intset.o test.o
	$(CXX) $(CXXFLAGS) $(BUILDIR)/linkedlist.o $(BUILDIR)/harris.o $(BUILDIR)/intset.o $(BUILDIR)/test.o -o $(BINS) $(LDFLAGS)

clean:
	-rm -f $(BINS)
