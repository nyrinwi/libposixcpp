# vim: noet
GTESTHOME=/usr/src/googletest/googletest
CXXFLAGS=-Wall -ggdb

CXXFLAGS+=-I $(GTESTHOME)/include
LDLIBS+=-L $(GTESTHOME)/lib -lgtest_main -lgtest -lpthread

ifdef gcov
CXXFLAGS+=--coverage
LDLIBS+=-lgcov
endif

LIBSOURCES=File.cpp PosixError.cpp
TESTSOURCES=FileTester.cpp PosixErrorTester.cpp Pipe.cpp

LIBOBJS=$(LIBSOURCES:.cpp=.o)
TESTOBJS=$(TESTSOURCES:.cpp=.o)

all:: tester

test:
	./tester

tester:: $(TESTOBJS) libposixcpp.a
	$(CXX) -o $@ $(TESTOBJS) -L. -l posixcpp $(LDLIBS)

libposixcpp.a: $(LIBOBJS)
	rm -f $@
	ar cq $@ $(LIBOBJS)

coverage:
	make -Bj gcov=1 tester

.PHONY: html
html:
	-./tester
	lcov -c -d . -o coverage.info
	genhtml -o html coverage.info

clean::
	rm -rf File *.o tester *.a bytes html coverage.info *.gcda *.gcno
