include $(CONCURRIT_HOME)/common.mk

# variables to set: BENCH, SRCS, optional: HEADERS
# others set when running pin: BENCHDIR

CC?=gcc

BENCHDIR?=$(CONCURRIT_BENCHDIR)/$(BENCH)
TARGET?=$(BENCH)
SRCS?=src/$(TARGET)test.cpp

TEST_FLAGS=$(CONCURRIT_TEST_INC_FLAGS) $(CONCURRIT_TEST_LIB_FLAGS) $(CONCURRIT_C_STD) \
			-I. -Isrc -L. -Llib \
			-g -gdwarf-2 -O1 -w -fPIC -fexceptions

#ifneq ($(wildcard lib/lib$(TARGET).so),)
#    TEST_FLAGS+=-l$(TARGET)
    TARGETLIB=lib$(TARGET).so
#else
#	TARGETLIB=
#endif

TARGETLIBPATH?=$(BENCHDIR)/lib/$(TARGETLIB)

makedirs::
	mkdir -p bin
	mkdir -p obj
	mkdir -p lib

clean::
	rm -f bin/* obj/* lib/*

clean-test::
	rm -f bin/*
	
clean-script::
	rm -f bin/$(TARGET)

bin/$(TARGET): lib/$(TARGETLIB) $(SRCS) $(HEADERS)
	$(CC) $(TEST_FLAGS) -o $@ $(SRCS)
#	$(CC) $(TEST_FLAGS) -c -o obj/$(TARGET).o $(SRCS)
#	ar rcs lib/$(TARGET).a obj/$(TARGET).o
	chmod +x bin/$(TARGET)

test: bin/$(TARGET)
	bin/$(TARGET)
	
pin: lib/$(TARGETLIB) bin/$(TARGET)
	LD_PRELOAD=$(CONCURRIT_HOME)/lib/libconcurrit.so:$(LD_PRELOAD) \
	PATH="$(BENCHDIR)/bin:$(PATH)" \
	LD_LIBRARY_PATH="$(BENCHDIR)/lib:$(LD_LIBRARY_PATH)" \
	DYLD_LIBRARY_PATH="$(BENCHDIR)/lib:$(DYLD_LIBRARY_PATH)" \
		$(CONCURRIT_HOME)/scripts/run_pintool.sh $(BENCHDIR)/bin/$(TARGET) $(ARGS)

LIBFLAGS+=-g -gdwarf-2 -O1 -w -fPIC -shared -ldummy -lpthread -fexceptions -I$(CONCURRIT_INCDIR) -L$(CONCURRIT_LIBDIR)

lib/lib$(TARGET).so: $(LIBSRCS) $(LIBHEADERS)
	$(CC) -I. -Isrc  $(LIBFLAGS) -o $@ $(LIBSRCS)

shared: makedirs lib/lib$(TARGET).so

script: bin/$(TARGET)

driver: shared

all: makedirs shared script

