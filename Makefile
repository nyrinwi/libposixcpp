# vim: noet
GTESTHOME=/usr/src/googletest/googletest
CXXFLAGS=-Wall -ggdb -dM

CXXFLAGS+=-I $(GTESTHOME)/include
LDLIBS+=-L $(GTESTHOME)/lib -lgtest_main -lgtest -lpthread

ifdef gcov
CXXFLAGS+=--coverage
LDLIBS+=-lgcov
endif

all::

LIBSOURCES=\
	File.cpp \
	MemMap.cpp \
	PosixError.cpp \
	Pipe.cpp \
	Socket.cpp \
	SocketPair.cpp \
	$()

TESTSOURCES=\
	FileTester.cpp \
	MemMapTester.cpp \
	PosixErrorTester.cpp \
	PipeTester.cpp \
	SocketTester.cpp \
	SocketPairTester.cpp \
	$()

LIBOBJS=$(LIBSOURCES:.cpp=.o)
TESTOBJS=$(TESTSOURCES:.cpp=.o)

all:: tester

test:
	./tester

tester:: $(TESTOBJS) libposixcpp.a
	$(CXX) -o $@ $(TESTOBJS) -L. $(ASAN) -l posixcpp $(LDLIBS)

libposixcpp.a: $(LIBOBJS)
	rm -f $@
	ar cq $@ $(LIBOBJS)

coverage:
	make -Bj gcov=1 tester

.PHONY: html
html:
	lcov -c -d . -o coverage.info
	genhtml -o html coverage.info

clean::
	rm -rf File *.o tester *.a bytes html coverage.info *.gcda *.gcno

File.o: File.cpp PosixError.h File.h
FileTester.o: FileTester.cpp File.h PosixError.h
Pipe.o: Pipe.cpp File.h PosixError.h Pipe.h
PipeTester.o: PipeTester.cpp Pipe.h File.h PosixError.h
PosixError.o: PosixError.cpp PosixError.h
PosixErrorTester.o: PosixErrorTester.cpp File.h PosixError.h
SocketPair.o: SocketPair.cpp SocketPair.h File.h PosixError.h
SocketPairTester.o: SocketPairTester.cpp SocketPair.h File.h PosixError.h
