
TARGET=inc
SRCS=src/$(TARGET)test.cpp

TEST_LIB_FLAGS=-lpthread -l$(TARGET)

TARGETLIB=lib$(TARGET).so
lib$(TARGET).so: src/increment.cpp src/increment.h
	g++ -g -Wall -Winline -fPIC -gdwarf-2 -O1 -shared -o $@ $^

include $(CONCURRIT_HOME)/test-common.mk
