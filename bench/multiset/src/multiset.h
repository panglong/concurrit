
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>
#include <sys/types.h>
#include "dummy.h"

#define MS_SIZE	4

typedef struct {
	pthread_mutex_t mutex;
	int value;
	int valid;
} element_t;

typedef struct {
	element_t elements[MS_SIZE];
	int size;
} multiset_t;


#ifdef __cplusplus
extern "C" {
#endif

void lock(element_t* elt);

void unlock(element_t* elt);

void multiset_init(multiset_t* ms);

int multiset_lookup(multiset_t* ms, int value);

int multiset_allocate(multiset_t* ms, int value);

int multiset_insert_pair(multiset_t* ms, int value1, int value2);


#ifdef __cplusplus
} // extern "C"
#endif
