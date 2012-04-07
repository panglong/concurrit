

#ifndef _ORIGINALS_H_INCLUDED
#define _ORIGINALS_H_INCLUDED

#include "mypthread.h"

class Originals
{
 public:
  
  static int pthread_create(pthread_t * thread,
			    const pthread_attr_t * attr,
			    void *(*start_routine)(void*), void * arg);
  
  static int pthread_once(pthread_once_t *once_control,
			  void (*init_routine)(void));

  static int pthread_join(pthread_t, void **);

  //pthread_t (*pthread_self_orig)(void);
  //int (*pthread_equal_orig)(pthread_t, pthread_t);

  static int pthread_cond_wait(pthread_cond_t *, pthread_mutex_t *mutex);
  static int pthread_cond_timedwait(pthread_cond_t *cond, 
            pthread_mutex_t *mutex,
	    const struct timespec *abstime);
  static int pthread_cond_signal(pthread_cond_t *);
  static int pthread_cond_broadcast(pthread_cond_t *);


  static int pthread_mutex_destroy (pthread_mutex_t *mutex);
  static int pthread_mutex_init (pthread_mutex_t *mutex,
				 const pthread_mutexattr_t * attr);
  
  static int pthread_mutex_lock (pthread_mutex_t *mutex);
  static int pthread_mutex_trylock (pthread_mutex_t *mutex);  
  static int pthread_mutex_unlock (pthread_mutex_t *mutex);

  static int pthread_equal(pthread_t t1, pthread_t t2);
  static pthread_t pthread_self();
  static int pthread_cond_init(pthread_cond_t * cond,
          pthread_condattr_t * attr);

  static void initialize();
  
  static volatile bool is_initialized();
  
 private:
  
  static void _initialize();
  
  static volatile bool _initialized;
  
  static int (* volatile _pthread_create) (pthread_t *,
					   const pthread_attr_t *,
					   void *(*)(void*), void *);

  static int (* volatile _pthread_join)(pthread_t, void **);

  static int (* volatile _pthread_cond_wait)(pthread_cond_t *, pthread_mutex_t *mutex);
  static int (* volatile _pthread_cond_timedwait) (pthread_cond_t *, pthread_mutex_t *,
		const struct timespec *abstime);
  static int (* volatile _pthread_cond_signal)(pthread_cond_t *);
  static int (* volatile _pthread_cond_broadcast)(pthread_cond_t *);

  
  static int (* volatile _pthread_mutex_destroy) (pthread_mutex_t *);
  static int (* volatile _pthread_mutex_init) (pthread_mutex_t *, 
		const pthread_mutexattr_t *);
  
  static int (* volatile _pthread_mutex_lock) (pthread_mutex_t *);
  static int (* volatile _pthread_mutex_trylock) (pthread_mutex_t *);  
  static int (* volatile _pthread_mutex_unlock) (pthread_mutex_t *);
  static int (* volatile _pthread_equal) (pthread_t, pthread_t);
  static pthread_t (* volatile _pthread_self)();
  static int (* volatile _pthread_cond_init) (pthread_cond_t *,
          pthread_condattr_t *);

 
  static pthread_once_t _once_control;
  
};

#endif // #ifndef _ORIGINALS_H_INCLUDED
