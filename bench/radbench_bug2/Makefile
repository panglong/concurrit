
BENCH=radbench_bug2

include $(CONCURRIT_HOME)/test-common.mk

RADBENCHDIR=$(RADBENCH_HOME)/bug2

TEST_FLAGS+=-L $(RADBENCHDIR)/src/js/src/*.OBJ/ -I $(RADBENCHDIR)/src/js/src/ -I $(RADBENCHDIR)/src/js/src/*.OBJ/ -I $(RADBENCHDIR)/bin/install/include/nspr/ -DXP_UNIX -DJS_THREADSAFE
LD_LIBRARY_PATH:=$(RADBENCHDIR)/lib:$(RADBENCHDIR)/src/js/src/Linux_All_DBG.OBJ:$(LD_LIBRARY_PATH)
DYLD_LIBRARY_PATH:=$(RADBENCHDIR)/lib:$(RADBENCHDIR)/src/js/src/Linux_All_DBG.OBJ:$(DYLD_LIBRARY_PATH)

makedirs::
	mkdir -p $(RADBENCHDIR)/bin
	mkdir -p $(RADBENCHDIR)/obj
	mkdir -p $(RADBENCHDIR)/lib

clean::
	$(RADBENCHDIR)/scripts/clean.sh
	rm -f $(RADBENCHDIR)/bin/*
	rm -f $(RADBENCHDIR)/obj/*

copyfiles:
	cp -f $(BENCHDIR)/scripts/* $(RADBENCHDIR)/scripts/
	cp -f $(BENCHDIR)/src/* $(RADBENCHDIR)/src/

all: makedirs copyfiles shared script

js:
	$(RADBENCHDIR)/scripts/build-js.sh

testharness:
	TARGETLIBPATH="$(TARGETLIBPATH)" $(RADBENCHDIR)/scripts/build-testharness.sh
	cp -f $(RADBENCHDIR)/src/runme.sh $(RADBENCHDIR)/bin/runme.sh

shared: makedirs copyfiles js testharness

driver: copyfiles testharness
