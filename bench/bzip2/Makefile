TARGET=bzip2
SRCS=src/$(TARGET)test.cpp

LIBSRCS=src/bzip2smp.c src/note.c src/detectht.c src/bzlib.c src/compress.c src/decompress.c src/crctable.c src/blocksort.c src/huffman.c src/randtable.c
LIBFLAGS=-D_FILE_OFFSET_BITS=64 -D_REENTRANT -D_GNU_SOURCE -DNICKEDIT -DERR1

CC=gcc

include $(CONCURRIT_HOME)/test-common.mk
