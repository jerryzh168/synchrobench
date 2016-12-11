ROOT = ../..

include $(ROOT)/common/Makefile.common

BINS = $(BINDIR)/linkedlist-harris-qsbr
PROF = $(ROOT)/src

CXXFLAGS += $(CFLAGS)
CXXFLAGS += -std=c++11 -mcx16 -O3

.PHONY:	all clean

all:	main

LIST_IMPL = harris_opt

measurements.o:
	$(CXX) $(CXXFLAGS) -c -o $(BUILDIR)/measurements.o $(PROF)/measurements.c

ssalloc.o:
	$(CXX) $(CXXFLAGS) -c -o $(BUILDIR)/ssalloc.o $(PROF)/ssalloc.c

linkedlist.o: 
	$(CXX) $(CXXFLAGS) -c -o $(BUILDIR)/linkedlist.o linkedlist.c

qsbr.o:
	$(CXX) $(CXXFLAGS) -c -o $(BUILDIR)/qsbr.o $(PROF)/qsbr.c

harris.o: linkedlist.h linkedlist.o
	$(CXX) $(CXXFLAGS) -c -o $(BUILDIR)/harris.o $(LIST_IMPL).c

intset.o: linkedlist.h harris.h
	$(CXX) $(CXXFLAGS) -c -o $(BUILDIR)/intset.o intset.c

#test.o: linkedlist.h harris.h intset.h
#	$(CC) $(CFLAGS) -c -o $(BUILDIR)/test.o $(TEST_FILE)


main: ssalloc.o linkedlist.o harris.o harris.o intset.o qsbr.o $(TMILB)
	$(CXX) $(CXXFLAGS) $(BUILDIR)/ssalloc.o $(BUILDIR)/linkedlist.o $(BUILDIR)/harris.o $(BUILDIR)/intset.o $(BUILDIR)/qsbr.o -o $(BINS) $(LDFLAGS)

clean:
	-rm -f $(BINS)
