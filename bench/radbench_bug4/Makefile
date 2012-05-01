
BENCH=radbench_bug4

include $(CONCURRIT_HOME)/test-common.mk

RADBENCHDIR=$(RADBENCH_HOME)/bug4

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
	cp -f $(BENCHDIR)/scripts/* $(RADBENCHDIR)/scripts/
	cp -f $(BENCHDIR)/src/* $(RADBENCHDIR)/src/

all: makedirs copyfiles shared script

patch: nspr-patch testharness

sleep: nspr-sleep testharness

nspr-bug:
	$(RADBENCHDIR)/scripts/build-nspr.sh default

nspr-sleep:
	$(RADBENCHDIR)/scripts/build-nspr.sh sleep

nspr-patch:
	$(RADBENCHDIR)/scripts/build-nspr.sh patch

testharness:
	TARGETLIBPATH="$(TARGETLIBPATH)" $(RADBENCHDIR)/scripts/build-testharness.sh
	cp -f $(RADBENCHDIR)/src/runme.sh $(RADBENCHDIR)/bin/runme.sh

shared: makedirs copyfiles nspr-bug testharness

driver: copyfiles testharness





