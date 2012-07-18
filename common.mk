# variables to set: CONCURRIT_HOME=... BOOST_ROOT=... RADBENCH_HOME=...

CONCURRIT_SRCDIR=$(CONCURRIT_HOME)/src
CONCURRIT_INCDIR=$(CONCURRIT_HOME)/include
CONCURRIT_BINDIR=$(CONCURRIT_HOME)/bin
CONCURRIT_OBJDIR=$(CONCURRIT_HOME)/obj
CONCURRIT_LIBDIR=$(CONCURRIT_HOME)/lib
CONCURRIT_TESTDIR=$(CONCURRIT_HOME)/tests
CONCURRIT_TPDIR=$(CONCURRIT_HOME)/third_party
CONCURRIT_WORKDIR=$(CONCURRIT_HOME)/work
CONCURRIT_BENCHDIR=$(CONCURRIT_HOME)/bench

CXXTESTDIR=$(CONCURRIT_TPDIR)/cxxtest

CONCURRIT_LIB_FLAG=-lconcurrit

CONCURRIT_C_STD=-std=c++0x

CONCURRIT_DEBUG_FLAGS=-DSAFE_ASSERT
CONCURRIT_PIN_DEBUG_FLAGS=-DSAFE_ASSERT

# flags to use when compiling concurrit
CONCURRIT_INC_FLAGS=$(CONCURRIT_DEBUG_FLAGS) -I$(CONCURRIT_INCDIR) -I$(CONCURRIT_SRCDIR) -I$(CONCURRIT_TPDIR)/glog/include -I$(BOOST_ROOT) -I$(CONCURRIT_TPDIR)/tbb/include -I$(CONCURRIT_TPDIR)/Mersenne-1.1
CONCURRIT_LIB_FLAGS=-L$(CONCURRIT_LIBDIR) -L$(CONCURRIT_TPDIR)/glog/lib -L$(CONCURRIT_TPDIR)/tbb/lib/intel64/cc4.1.0_libc2.4_kernel2.6.16.21 -lpthread -lglog -ltbb
# -L/usr/lib/gcc/x86_64-linux-gnu/4.4

# flags to use when compiling tests with concurrit
CONCURRIT_TEST_INC_FLAGS=$(CONCURRIT_INC_FLAGS)
CONCURRIT_TEST_LIB_FLAGS=-L$(CONCURRIT_LIBDIR) $(CONCURRIT_LIB_FLAG) 

# flags to use when compiling programs under test with concurrit
#CONCURRIT_PROG_INC_FLAGS=-I$(CONCURRIT_INCDIR)
#CONCURRIT_PROG_LIB_FLAGS=-L$(CONCURRIT_LIBDIR) $(CONCURRIT_LIB_FLAG)

# flags to use when compiling the pin tool
CONCURRIT_PINTOOL_INC_FLAGS=$(CONCURRIT_PIN_DEBUG_FLAGS) -I$(CONCURRIT_INCDIR) -I$(CONCURRIT_TPDIR)/tbb/include -fomit-frame-pointer -g -gdwarf-2 -O3
CONCURRIT_PINTOOL_LIB_FLAGS=-L$(CONCURRIT_LIBDIR) $(CONCURRIT_LIB_FLAG) -L$(CONCURRIT_TPDIR)/tbb/lib/intel64/cc4.1.0_libc2.4_kernel2.6.16.21 -ltbb
