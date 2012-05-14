
BENCH=radbench_bug3

include $(CONCURRIT_HOME)/test-common.mk

RADBENCHDIR=$(RADBENCH_HOME)/bug3

TEST_FLAGS+=-L $(RADBENCHDIR)/src/js/src/*.OBJ/ -I $(RADBENCHDIR)/src/js/src/ -I $(RADBENCHDIR)/src/js/src/*.OBJ/ -I $(RADBENCHDIR)/bin/install/include/nspr/ -DXP_UNIX -DJS_THREADSAFE
LD_LIBRARY_PATH:=$(RADBENCHDIR)/lib:$(RADBENCHDIR)/src/js/src/Linux_All_DBG.OBJ:$(LD_LIBRARY_PATH)
DYLD_LIBRARY_PATH:=$(RADBENCHDIR)/lib:$(RADBENCHDIR)/src/js/src/Linux_All_DBG.OBJ:$(DYLD_LIBRARY_PATH)

makedirs::
	mkdir -p $(RADBENCHDIR)/bin
	mkdir -p $(RADBENCHDIR)/obj
	mkdir -p $(RADBENCHDIR)/lib

clean::
	$(RADBENCHDIR)/scripts/clean.sh
	rm -rf $(RADBENCHDIR)/bin/*
	rm -rf $(RADBENCHDIR)/obj/*

copyfiles:
	cp -rf $(BENCHDIR)/scripts/* $(RADBENCHDIR)/scripts/
	cp -rf $(BENCHDIR)/src/* $(RADBENCHDIR)/src/

all: makedirs copyfiles shared script

patch: js-patch testharness

js-bug:
	$(RADBENCHDIR)/scripts/build-js.sh 1.8

js-nobug:
	$(RADBENCHDIR)/scripts/build-js.sh 1.7

js-patch:
	$(RADBENCHDIR)/scripts/build-js.sh 1.8-patch

testharness:
	TARGETLIBPATH="$(TARGETLIBPATH)" $(RADBENCHDIR)/scripts/build-testharness.sh
	cp -f $(RADBENCHDIR)/src/runme.sh $(RADBENCHDIR)/bin/runme.sh

shared: makedirs copyfiles js-bug testharness

driver: copyfiles testharness
