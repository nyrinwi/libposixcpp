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
	ClientSocket.cpp \
	SocketPair.cpp \
	$()

LIBOBJS=$(LIBSOURCES:.cpp=.o)

all:: libposixcpp.a
	make -C tests 

test:
	make -C tests test

libposixcpp.a: $(LIBOBJS)
	rm -f $@
	ar cq $@ $(LIBOBJS)

.PHONY: html
html:
	lcov -c -d . -o coverage.info
	genhtml -o html coverage.info

clean::
	rm -rf File *.o tester *.a bytes html coverage.info *.gcda *.gcno
	make -C tests clean

File.o: File.cpp PosixError.h File.h
Pipe.o: Pipe.cpp File.h PosixError.h Pipe.h
PosixError.o: PosixError.cpp PosixError.h
SocketPair.o: SocketPair.cpp SocketPair.h File.h PosixError.h
