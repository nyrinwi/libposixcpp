# vim: noet
GTESTHOME=/usr/src/googletest/googletest
CXXFLAGS=-Wall -ggdb --std=c++14 

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

test: libposixcpp.a
	make -C tests test

libposixcpp.a: $(LIBOBJS)
	ar rcs $@ $(LIBOBJS)
	ranlib $@

.PHONY: html
html:
	lcov -c -d . -o coverage.info
	genhtml -o html coverage.info

clean::
	rm -rf File *.o tester *.a bytes html coverage.info *.gcda *.gcno
	make -C tests clean

depends:
	rm -f .depends	
	for f in *.cpp ; do g++ -MM -MF - -I. $$f >> .depends ; done

-include .depends

