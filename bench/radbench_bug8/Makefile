
BENCH=radbench_bug8

include $(CONCURRIT_HOME)/test-common.mk

RADBENCHDIR=$(RADBENCH_HOME)/bug8

TEST_FLAGS+=
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH)
DYLD_LIBRARY_PATH:=$(DYLD_LIBRARY_PATH)

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

memcached:
	$(RADBENCHDIR)/scripts/build-memcached.sh

testharness:
	TARGETLIBPATH="$(TARGETLIBPATH)" $(RADBENCHDIR)/scripts/build-testharness.sh
	cp -f $(RADBENCHDIR)/src/runme.sh $(RADBENCHDIR)/bin/runme.sh
	
shared: makedirs copyfiles memcached testharness

driver: copyfiles testharness
