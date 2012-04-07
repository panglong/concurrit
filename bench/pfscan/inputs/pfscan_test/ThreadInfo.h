/** 

ThreadId is just a wrapper class for a bunch of static variables.

How to keep thread ids: we use thread-specific storage using a key.

Steps:

Initialization:

pthread_key_create

Give id to each thread:

We keep a counter and 

Get id: that's easy -- just do pthread_getspecific.

*/

#ifndef _THREADINFO_H_INCLUDED
#define _THREADINFO_H_INCLUDED

#include "mypthread.h"


class ThreadInfo
{
 public:
  
  typedef unsigned long id_t;

  static void initialize();

  static ThreadInfo * get();
  static void set(ThreadInfo * tInfo);

  ThreadInfo(const id_t id)
    : _id(id)
    {}

  virtual ~ThreadInfo() {}

  id_t id() const { return _id; }

 private:

  id_t _id;

  static void _make_key();

  static pthread_key_t _key;
  static pthread_once_t _key_once;

  static volatile id_t _next_id;
  static pthread_mutex_t _lock;
};

#endif // #ifndef _THREADINFO_H_INCLUDED
