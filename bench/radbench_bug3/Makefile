
BENCH=radbench_bug3

include $(CONCURRIT_HOME)/test-common.mk

RADBENCHDIR=$(RADBENCH_HOME)/bug3

TEST_FLAGS+=-L$(RADBENCHDIR)/src/js/src/Linux_All_DBG.OBJ
LD_LIBRARY_PATH:=$(RADBENCHDIR)/lib:$(RADBENCHDIR)/src/js/src/Linux_All_DBG.OBJ:$(LD_LIBRARY_PATH)
DYLD_LIBRARY_PATH:=$(RADBENCHDIR)/lib:$(RADBENCHDIR)/src/js/src/Linux_All_DBG.OBJ:$(DYLD_LIBRARY_PATH)

makedirs::
	mkdir -p $(RADBENCHDIR)/bin
	mkdir -p $(RADBENCHDIR)/obj
	mkdir -p $(RADBENCHDIR)/lib

clean::
	#$(RADBENCHDIR)/scripts/clean.sh
	rm -f $(RADBENCHDIR)/bin/*
	rm -f $(RADBENCHDIR)/obj/*

copyfiles:
	cp -f $(BENCHDIR)/scripts/* $(RADBENCHDIR)/scripts/
	cp -f $(BENCHDIR)/src/* $(RADBENCHDIR)/src/

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

driver: testharness
