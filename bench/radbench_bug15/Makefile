
BENCH=radbench_bug15

include $(CONCURRIT_HOME)/test-common.mk

RADBENCHDIR=$(RADBENCH_HOME)/bug15

TEST_FLAGS+=
LD_LIBRARY_PATH:=$(LD_LIBRARY_PATH)
DYLD_LIBRARY_PATH:=$(DYLD_LIBRARY_PATH)

makedirs::
	mkdir -p $(RADBENCHDIR)/bin
	mkdir -p $(RADBENCHDIR)/obj
	mkdir -p $(RADBENCHDIR)/lib

clean::
	rm -rf $(RADBENCHDIR)/bin/*
	rm -rf $(RADBENCHDIR)/obj/*
	make -C $(RADBENCHDIR) clean

copyfiles:
	cp -rf $(BENCHDIR)/scripts/* $(RADBENCHDIR)/scripts/
	cp -rf $(BENCHDIR)/src/* $(RADBENCHDIR)/src/
	cp -f $(BENCHDIR)/scripts/Makefile $(RADBENCHDIR)/

all: makedirs copyfiles mysql harness

harness: makedirs copyfiles
	gcc -lpthread -o $(RADBENCHDIR)/bin/test-mysql $(RADBENCHDIR)/src/test-harness.cpp \
		-g -gdwarf-2 -O1 -w -fPIC -lconcurrit -I$(CONCURRIT_HOME)/include -L$(CONCURRIT_HOME)/lib \
		-lglog -L$(CONCURRIT_TPDIR)/glog/lib -I$(CONCURRIT_TPDIR)/glog/include \
		-ltbb -L$(CONCURRIT_HOME)/third_party/tbb/lib/intel64/cc4.1.0_libc2.4_kernel2.6.16.21 -I$(CONCURRIT_HOME)/third_party/tbb/include

mysql:
	make -C $(RADBENCHDIR) build
	cp -f $(BENCHDIR)/src/mysqld.cc $(RADBENCHDIR)/src/mysql-4.0.12/sql/
	make -C $(RADBENCHDIR) assert
	
	