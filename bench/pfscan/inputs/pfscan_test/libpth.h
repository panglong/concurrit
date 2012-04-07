/**
* libpth - Pthreads hooking library
* 
* Author - Chang-Seo Park (parkcs@cs.berkeley.edu)
*
* <Legal matter>
*/

#include "Originals.h"
#include "mypthread.h"
#include <cstdlib>
#include <cstdio>
#include <pthread.h>
#include <dlfcn.h>
#include <stdarg.h>

#define safe_assert(cond, exit_status) if (!(cond)) exit((exit_status))

#if 0
#define __LOCK_T__ pthread_mutex_t
#define __LOCK_INIT__(lock) pthread_mutex_init(lock, NULL)
#define __LOCK_DESTROY__ pthread_mutex_destroy
#define __LOCK__ Originals::pthread_mutex_lock
#define __UNLOCK__ Originals::pthread_mutex_unlock
#endif

#define __LOCK_T__ pthread_spinlock_t
#define __LOCK_INIT__(lock) pthread_spin_init(lock, PTHREAD_PROCESS_PRIVATE)
#define __LOCK_DESTROY__ pthread_spin_destroy
#define __LOCK__ pthread_spin_lock
#define __UNLOCK__ pthread_spin_unlock

// Library initialization
void pth_start(void) __attribute__((constructor));
void pth_end(void) __attribute__((destructor));

// Number of nestings inside instrumentation

class ThreadInfo;

// "null" pthread handler 
class Handler {
public:
	Handler() : _globalClock(0), _single_thread(true) {
		__LOCK_INIT__(&_lock);
	}

	virtual ~Handler() {
		__LOCK_DESTROY__(&_lock);
	}

	int dbgPrint(const char* format, ...) {
            va_list args;
            fprintf(stderr, "libpth@%d:", _globalClock);
            va_start(args, format);
            vfprintf(stderr, format, args);
            va_end(args);
            return 0;

        }

protected:
        inline void lock() {
            ++insideInst;
            __LOCK__(&_lock);
            --insideInst;
        }

        inline void unlock() {
            ++insideInst;
            __UNLOCK__(&_lock);
            --insideInst;
	}

	inline int advanceGlobalClock() {
#if 0
		int gClock;
		lock();
		gClock = ++_globalClock;
		unlock();
		return gClock;
#endif
		return __sync_add_and_fetch(&_globalClock, 1);
	}

#if 0
	inline int globalClock() {
		int gClock;
		lock();
		gClock = _globalClock;
		unlock();
		return gClock;
	}
#endif

	virtual void AfterInitialize() { }
        virtual void ThreadStart(void * arg) {}
        virtual void ThreadFinish(void * status) {}

	virtual bool BeforeCreate(pthread_t* newthread, 
                                    const pthread_attr_t *attr,
			            void *(*start_routine) (void *), 
                                    void *arg, ThreadInfo* &tInfo) {
		return true; }
        virtual int SimulateCreate(pthread_t* newthread, 
                                    const pthread_attr_t *attr,
			            void *(*start_routine) (void *), 
                                    void *arg, ThreadInfo* &tInfo) {
		return 0; }
                                    
	virtual void AfterCreate(int& ret_val, 
                                    pthread_t* newthread, 
			            const pthread_attr_t *attr,
                                    void *(*start_routine) (void *), 
			            void *arg) {}
	virtual bool BeforeJoin(pthread_t th, void** thread_return) {
		return true; }
	virtual int SimulateJoin(pthread_t th, void** thread_return) {
		return 0; }
	
	virtual void AfterJoin(int& ret_val, 
                                pthread_t th, 
                                void** thread_return) {}
	virtual bool BeforeMutexLock(pthread_mutex_t *mutex) {
		return true; }
	virtual int SimulateMutexLock(pthread_mutex_t *mutex) {
                return 0;}
	virtual void AfterMutexLock(int& ret_val, pthread_mutex_t *mutex) {}
	virtual bool BeforeMutexUnlock(pthread_mutex_t *mutex) {
		return true; }
	virtual int SimulateMutexUnlock(pthread_mutex_t *mutex) {
		return 0; }
	virtual void AfterMutexUnlock(int& ret_val, pthread_mutex_t *mutex) {}
	virtual bool BeforeMutexTrylock(pthread_mutex_t *mutex) {
		return true; }
        virtual int SimulateMutexTrylock(pthread_mutex_t *mutex) {
		return 0; }
	virtual void AfterMutexTrylock(int& ret_val, pthread_mutex_t *mutex) {}
	virtual bool BeforeCondWait(pthread_cond_t *cond, 
                                        pthread_mutex_t *mutex) {
		return true; }
        virtual int SimulateCondWait(pthread_cond_t *cond, 
                                        pthread_mutex_t *mutex) {
		return 0; }
	virtual void AfterCondWait(int& ret_val, pthread_cond_t *cond, 
			pthread_mutex_t *mutex) {}
	virtual bool BeforeCondTimedwait(pthread_cond_t *cond, 
                                            pthread_mutex_t *mutex,
			                    const struct timespec *abstime) {
		return true; }
	virtual int SimulateCondTimedwait(pthread_cond_t *cond, 
                                            pthread_mutex_t *mutex,
			                    const struct timespec *abstime) {
		return 0; }
        virtual void AfterCondTimedwait(int& ret_val, pthread_cond_t *cond, 
                pthread_mutex_t *mutex, const struct timespec *abstime) {}
	virtual bool BeforeCondSignal(pthread_cond_t *cond) {
		return true; }
        virtual int SimulateCondSignal(pthread_cond_t *cond) {
		return 0; }
	
	virtual void AfterCondSignal(int& ret_val, pthread_cond_t *cond) {}
	virtual bool BeforeCondBroadcast(pthread_cond_t *cond) {
		return true; }
        virtual int SimulateCondBroadcast(pthread_cond_t *cond) {
		return 0; }
	virtual void AfterCondBroadcast(int& ret_val, pthread_cond_t *cond) {}
	
protected:
	__LOCK_T__ _lock;
	volatile int _globalClock;
	volatile bool _single_thread;

private:
	static const char* UNKNOWN;
	static __thread int insideInst;

	int Create(pthread_t* newthread, const pthread_attr_t *attr, 
		   void *(*start_routine) (void *), void *arg);
	
	int Join(pthread_t th, void** thread_return);
	int Mutex_Lock(pthread_mutex_t *mutex);
	int Mutex_Unlock(pthread_mutex_t *mutex);
	int Mutex_Trylock(pthread_mutex_t *mutex);
	int Cond_Wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
	int Cond_Timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
			const struct timespec *abstime);
	int Cond_Signal(pthread_cond_t *cond);
	int Cond_Broadcast(pthread_cond_t *cond);

friend class Initializer;
friend int pthread_create (pthread_t* newthread, const pthread_attr_t *attr, 
		void *(*start_routine) (void *), void *arg);
friend int pthread_join (pthread_t th, void** thread_return);
friend int pthread_mutex_lock (pthread_mutex_t *mutex);
friend int pthread_mutex_unlock (pthread_mutex_t *mutex);
friend int pthread_mutex_trylock (pthread_mutex_t *mutex);
friend int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
friend int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
		const struct timespec *abstime);
friend int pthread_cond_signal(pthread_cond_t *cond);
friend int pthread_cond_broadcast(pthread_cond_t *cond);
friend void * my_start_routine(void * my_arg);
};

extern Handler* create_handler();

extern Handler * volatile pHandler;



