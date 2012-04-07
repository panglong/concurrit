

#ifndef _MYPTHREAD_H_INCLUDED
#define _MYPTHREAD_H_INCLUDED

#include <pthread.h>
#include <iostream>

using namespace std;

#define call_pthread(stmt) { const int rc = (stmt); if (rc) { cerr << "pthread failure at " << __FILE__ << ":" << __LINE__ << ": error code " << rc << endl; exit(1); } }

#define call_pthread_safe(stmt) { const int rc = (stmt); if (rc) { exit(44); } }

extern volatile pthread_mutex_t mypthread_atomic_print_lock;

#define atomic_print(stream, args) { call_pthread( pthread_mutex_lock(const_cast<pthread_mutex_t *>(&mypthread_atomic_print_lock)) ); stream << args; call_pthread( pthread_mutex_unlock(const_cast<pthread_mutex_t *>(&mypthread_atomic_print_lock)) ); }

#endif // #ifndef _MYPTHREAD_H_INCLUDED
