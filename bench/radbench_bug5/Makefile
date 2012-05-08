
BENCH=radbench_bug5

include $(CONCURRIT_HOME)/test-common.mk

RADBENCHDIR=$(RADBENCH_HOME)/bug5

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

patch: nspr-patch testharness

sleep: nspr-sleep testharness

nspr-bug:
	$(RADBENCHDIR)/scripts/build-nspr.sh default

nspr-sleep:
	$(RADBENCHDIR)/scripts/build-nspr.sh sleep

testharness:
	TARGETLIBPATH="$(TARGETLIBPATH)" $(RADBENCHDIR)/scripts/build-testharness.sh
	cp -f $(RADBENCHDIR)/src/runme.sh $(RADBENCHDIR)/bin/runme.sh

shared: makedirs copyfiles nspr-bug testharness

driver: copyfiles testharness
