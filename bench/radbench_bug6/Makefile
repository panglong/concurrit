
BENCH=radbench_bug6

include $(CONCURRIT_HOME)/test-common.mk

RADBENCHDIR=$(RADBENCH_HOME)/bug6

TEST_FLAGS+=
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH)
DYLD_LIBRARY_PATH:=$(DYLD_LIBRARY_PATH)

makedirs::
	mkdir -p $(RADBENCHDIR)/bin
	mkdir -p $(RADBENCHDIR)/obj
	mkdir -p $(RADBENCHDIR)/lib

clean::
	$(RADBENCHDIR)/scripts/clean.sh
	rm -f $(RADBENCHDIR)/bin/*
	rm -f $(RADBENCHDIR)/obj/*

copyfiles:
	cp -rf $(BENCHDIR)/scripts/* $(RADBENCHDIR)/scripts/
	cp -rf $(BENCHDIR)/src/* $(RADBENCHDIR)/src/

all: makedirs copyfiles shared script

nspr:
	$(RADBENCHDIR)/scripts/build-nspr.sh

testharness:
	TARGETLIBPATH="$(TARGETLIBPATH)" $(RADBENCHDIR)/scripts/build-testharness.sh default
	cp -f $(RADBENCHDIR)/src/runme.sh $(RADBENCHDIR)/bin/runme.sh
	
testharness-sleep:
	TARGETLIBPATH="$(TARGETLIBPATH)" $(RADBENCHDIR)/scripts/build-testharness.sh sleep
	cp -f $(RADBENCHDIR)/src/runme.sh $(RADBENCHDIR)/bin/runme.sh

shared: makedirs copyfiles nspr testharness

driver: copyfiles testharness
