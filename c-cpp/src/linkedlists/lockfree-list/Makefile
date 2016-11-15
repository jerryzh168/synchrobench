ROOT = ../../..

include $(ROOT)/common/Makefile.common

BINS = $(BINDIR)/lockfree-linkedlist
CXXFLAGS = $(CFLAGS)
CXXFLAGS += -std=c++11 -g -O3

.PHONY:	all clean

all:	main

list.o: list.h list.c
	$(CXX) $(CXXFLAGS) -c -o $(BUILDIR)/list.o list.c

linkedlist.o: linkedlist.h linkedlist.c
	$(CXX) $(CXXFLAGS) -c -o $(BUILDIR)/linkedlist.o linkedlist.c

hprectype.o: hprectype.h hprectype.c
	$(CXX) $(CXXFLAGS) -c -o $(BUILDIR)/hprectype.o hprectype.c

harris.o: linkedlist.h linkedlist.o hprectype.h hprectype.o harris.h harris.c
	$(CXX) $(CXXFLAGS) -c -o $(BUILDIR)/harris.o harris.c

intset.o: linkedlist.h harris.h
	$(CXX) $(CXXFLAGS) -c -o $(BUILDIR)/intset.o intset.c

test.o: linkedlist.h harris.h intset.h hprectype.h list.h
	$(CXX) $(CXXFLAGS) -c -o $(BUILDIR)/test.o test.c

main: list.o linkedlist.o hprectype.o harris.o intset.o test.o
	$(CXX) $(CXXFLAGS) $(BUILDIR)/list.o $(BUILDIR)/linkedlist.o $(BUILDIR)/hprectype.o $(BUILDIR)/harris.o $(BUILDIR)/intset.o $(BUILDIR)/test.o -o $(BINS) $(LDFLAGS)

clean:
	-rm -f $(BINS)
