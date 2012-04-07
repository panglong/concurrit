include $(CONCURRIT_HOME)/common.mk

# variables to set: TARGET, SRCS, optional: HEADERS

CC?=g++

TEST_FLAGS=$(CONCURRIT_TEST_INC_FLAGS) $(CONCURRIT_TEST_LIB_FLAGS) -I. -L. -g -gdwarf-2 -O0 -Wall $(CONCURRIT_C_STD)

if [ -f "lib/lib$(TARGET).so" ];
then
    TEST_FLAGS+=-l$(TARGET)
    TARGETLIB=lib$(TARGET).so
else
	TARGETLIB=
fi

makedirs:
	mkdir -p bin
	mkdir -p obj
	mkdir -p lib

clean:
	rm -f bin/* obj/* lib/*

shared: makedirs lib/lib$(TARGET).so

bin/$(TARGET): makedirs lib/$(TARGETLIB) $(SRCS) $(HEADERS)
	$(CC) $(TEST_FLAGS) -o bin/$(TARGET) $^
	$(CC) $(TEST_FLAGS) -c -o obj/$(TARGET).o $^
	ar rcs lib/$(TARGET).a obj/$(TARGET).o
	chmod +x bin/$(TARGET)

test: bin/$(TARGET)
	bin/$(TARGET)
	
pin: lib/$(TARGETLIB) bin/$(TARGET)
	$(CONCURRIT_HOME)/scripts/run_pintool.sh bin/$(TARGET) $(ARGS)

LIBFLAGS+=-lpthread
lib/lib$(TARGET).so: $(LIBSRCS) $(LIBHEADERS)
	g++ -g -Wall -Winline -fPIC -gdwarf-2 -O1 -shared $(LIBFLAGS) -o $@ $^

all: bin/$(TARGET)
