TARGET=bb
SRCS=src/bbtest.cpp

TEST_LIB_FLAGS=-lpthread -lbb

include $(CONCURRIT_HOME)/test-common.mk

shared: libbb.so

libbb.so: src/boundedBuffer.cpp src/boundedBuffer.h
	g++ -g -Wall -Winline -fPIC -gdwarf-2 -O1 -shared -o $@ $^
