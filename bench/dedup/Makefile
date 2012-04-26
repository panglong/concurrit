BENCH=dedup

LIBSRCS=src/hashtable.c src/queue.c src/util.c src/binheap.c src/tree.c src/decoder.c src/dedup.c src/rabin.c src/encoder.c
LIBFLAGS=-std=c99 -fno-strict-aliasing -D_XOPEN_SOURCE=500 -lz -lm -lssl -DPARALLEL -DERR1

include $(CONCURRIT_HOME)/test-common.mk
