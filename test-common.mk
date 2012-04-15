include $(CONCURRIT_HOME)/common.mk

# variables to set: TARGET, SRCS, optional: HEADERS
# others set when running pin: BENCH, BENCHDIR

CC?=gcc

TEST_FLAGS=$(CONCURRIT_TEST_INC_FLAGS) $(CONCURRIT_TEST_LIB_FLAGS) -I. -Isrc -L. -Llib -g -gdwarf-2 -O1 -w -fPIC -fexceptions $(CONCURRIT_C_STD)

ifneq ($(wildcard lib/lib$(TARGET).so),)
    TEST_FLAGS+=-l$(TARGET)
    TARGETLIB=lib$(TARGET).so
else
	TARGETLIB=
endif

makedirs:
	mkdir -p bin
	mkdir -p obj
	mkdir -p lib

clean:
	rm -f bin/* obj/* lib/*

clean-test:
	rm -f bin/*

bin/$(TARGET): lib/$(TARGETLIB) $(SRCS) $(HEADERS)
	$(CC) $(TEST_FLAGS) -o $@ $(SRCS)
	$(CC) $(TEST_FLAGS) -c -o obj/$(TARGET).o $(SRCS)
	ar rcs lib/$(TARGET).a obj/$(TARGET).o
	chmod +x bin/$(TARGET)

test: bin/$(TARGET)
	bin/$(TARGET)
	
pin: lib/$(TARGETLIB) bin/$(TARGET)
	LD_PRELOAD=$(CONCURRIT_HOME)/lib/libconcurrit.so:$(LD_PRELOAD) \
	PATH="$(BENCHDIR)/bin:$(PATH)" \
	LD_LIBRARY_PATH="$(BENCHDIR)/lib:$(LD_LIBRARY_PATH)" \
	DYLD_LIBRARY_PATH=="$(BENCHDIR)/lib:$(DYLD_LIBRARY_PATH)" \
		$(CONCURRIT_HOME)/scripts/run_pintool.sh bin/$(TARGET) $(ARGS)

shared: makedirs lib/lib$(TARGET).so

LIBFLAGS+=-ldummy -lpthread -I$(CONCURRIT_INCDIR) -L$(CONCURRIT_LIBDIR)

lib/lib$(TARGET).so: $(LIBSRCS) $(LIBHEADERS)
	$(CC) -I. -Isrc -g -gdwarf-2 -O1 -w -fPIC $(LIBFLAGS) -shared -o $@ $(LIBSRCS)

all: makedirs shared bin/$(TARGET)
