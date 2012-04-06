#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <stdint.h>

#define PRODUCER_SUM  3
#define CONSUMER_SUM  3

typedef struct bounded_buf_tag
{
  int valid;

  pthread_mutex_t mutex;
  pthread_cond_t  not_full;
  pthread_cond_t  not_empty;
  
  size_t   item_num;
  size_t   max_size;
  size_t   head, rear;

  size_t   p_wait;  // waiting producers
  size_t   c_wait;  // waiting consumers

  void **  buf;
}bounded_buf_t;

#define BOUNDED_BUF_VALID 0xACDEFA

#define BOUNDED_BUF_INITIALIZER \
   { BOUNDED_BUF_VALID,  PTHREAD_MUTEX_INITIALIZER, \
     PTHREAD_COND_INITIALIZER,  PTHREAD_COND_INITIALIZER, \
     0, 0, 0, 0, null }


int bounded_buf_init(bounded_buf_t * bbuf, size_t sz);

int bounded_buf_destroy(bounded_buf_t * bbuf);


void bounded_buf_putcleanup(void * arg);

void bounded_buf_getcleanup(void *arg);

int bounded_buf_put(bounded_buf_t * bbuf, void *item);

int bounded_buf_get(bounded_buf_t *bbuf, void **item);

int bounded_buf_is_empty(bounded_buf_t* bbuf);

int bounded_buf_is_full(bounded_buf_t* bbuf);

typedef struct thread_tag
{
  pthread_t       pid;
  int             id;
  bounded_buf_t * bbuf;
}thread_t;


void * producer_routine(void *arg);

void * consumer_routine(void * arg);

extern "C" int __main__(int argc, char ** argv);
