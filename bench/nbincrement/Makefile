TARGET=inctest
SRCS=src/inctest.cpp

TEST_LIB_FLAGS=-lpthread -lincrement

include $(CONCURRIT_HOME)/test-common.mk

shared: libincrement.so

libincrement.so: src/increment.cpp src/increment.h
	g++ -g -Wall -Winline -fPIC -gdwarf-2 -O1 -shared -o $@ $^
