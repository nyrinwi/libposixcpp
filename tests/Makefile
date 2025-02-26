# vim: noet
GTESTHOME=/usr/src/googletest/googletest
CXXFLAGS=-Wall -ggdb

CXXFLAGS+=-I $(GTESTHOME)/include
LDLIBS+=-L $(GTESTHOME)/lib -lgtest_main -lgtest -lpthread

ifdef gcov
CXXFLAGS+=--coverage
LDLIBS+=-lgcov
endif

all::

CXXFLAGS+=-I $(CURDIR)/..
LDLIBS+=-L $(CURDIR)/.. -lposixcpp

TESTSOURCES=\
	FileTester.cpp \
	MemMapTester.cpp \
	PosixErrorTester.cpp \
	PipeTester.cpp \
	SocketTester.cpp \
	SocketPairTester.cpp \
	$()

TESTOBJS=$(TESTSOURCES:.cpp=.o)

all:: tester

SINK=sink.dat

test:
	rm -f $(SINK)
	./tester 

tester:: $(TESTOBJS)
	$(CXX) -o $@ $(TESTOBJS) -L$(CURDIR)/.. -L. $(ASAN) -l posixcpp $(LDLIBS)

coverage:
	make -Bj gcov=1 tester

.PHONY: html
html:
	lcov -c -d . -o coverage.info
	genhtml -o html coverage.info

clean::
	rm -rf File *.o tester *.a bytes html coverage.info *.gcda *.gcno $(SINK)

