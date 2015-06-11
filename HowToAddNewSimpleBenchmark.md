# Adding a simple benchmark #

Suppose that we want to add a benchmark named `queue` for testing a concurrent queue.
  1. Create a new directory `$CONCURRIT_HOME/bench/queue`
  1. Put the source files in `$CONCURRIT_HOME/bench/queue/src`
  1. Add file `$CONCURRIT_HOME/bench/queue/Makefile` with the following contents:
```
# bench name must be the same as the directory name
BENCH=queue
# source files (e.g., .c, .cpp) in the benchmark
LIBSRCS=src/queue1.c src/queue2.c
# additional flags to use when compiling benchmark sources
LIBFLAGS= -DDEBUG
# include common makefile definitions for benchmarks
include $(CONCURRIT_HOME)/test-common.mk
```
  1. Add file `$CONCURRIT_HOME/bench/queue/finfile.txt` with each line naming a function that must be instrumented
  1. Add file `$CONCURRIT_HOME/bench/queue/bench_args.txt` to contain a command line to be used when running the benchmark SUT.
  1. Add file `$CONCURRIT_HOME/bench/queue/queuetest.cpp` to contain the CONCURRIT test for the benchmark. The naming is important to use bash scripts for compiling and running the tests. The filename must start with the benchmark name and end with "test".

To write the test in CONCURRIT continue with Wiki page WritingTestsInConcurrit.

<a href='Hidden comment: 
Add your content here.  Format your content with:
* Text in *bold* or _italic_
* Headings, paragraphs, and lists
* Automatic links to other wiki pages
'></a>