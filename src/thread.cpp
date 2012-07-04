/**
 * Copyright (c) 2010-2011,
 * Tayfun Elmas    <elmas@cs.berkeley.edu>
 * All rights reserved.
 * <p/>
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * <p/>
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * <p/>
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * <p/>
 * 3. The names of the contributors may not be used to endorse or promote
 * products derived from this software without specific prior written
 * permission.
 * <p/>
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "concurrit.h"

namespace concurrit {

int __pthread_errno__ = PTH_SUCCESS;

/********************************************************************************/

pthread_key_t Thread::tls_key_;

/********************************************************************************/

void Thread::init_tls_key() {
	pthread_key_t key;
	__pthread_errno__ = pthread_key_create(&key, NULL);
	safe_assert(__pthread_errno__ == PTH_SUCCESS);

	tls_key_ = key;
}

/********************************************************************************/

void Thread::delete_tls_key() {
	pthread_key_t key = tls_key_;

	__pthread_errno__ = pthread_key_delete(key);
	safe_assert(__pthread_errno__ == PTH_SUCCESS);
}

/********************************************************************************/

Thread::Thread(THREADID tid, ThreadEntryFunction entry_function, void* entry_arg, int stack_size)
: entry_function_(entry_function),
  entry_arg_(entry_arg),
  stack_size_(stack_size) {
	safe_assert(stack_size_ >= 0);
	set_tid(tid);
	set_pthread(PTH_INVALID_THREAD);
	set_return_value(NULL);
}

/********************************************************************************/

void* Thread::call_function() {
	ThreadEntryFunction func = entry_function_;
	void* arg = entry_arg_;
	safe_assert(func != NULL);
	return_value_ = NULL;
	return_value_ = func(arg);
	return return_value_;
}

/********************************************************************************/

void* Thread::Run() {
	return call_function();
}

/********************************************************************************/

void Thread::CleanupHandler(void* arg) {
	safe_assert(arg != NULL);

	Thread* thread = reinterpret_cast<Thread*>(arg);

	pthread_t self = pthread_self();
	safe_assert(self != PTH_INVALID_THREAD);
	safe_assert(thread->pthread() == self);

	thread->detach_pthread(self);
}

/********************************************************************************/

void* Thread::ThreadEntry(void* arg) {
	// sometimes the threads starts before pthread_create takes affect,
	// so ensure that we give enough time to pthread_create
	sched_yield();

	void* ret_val = NULL;

	Thread* thread = reinterpret_cast<Thread*>(arg);

	pthread_cleanup_push(Thread::CleanupHandler, thread);

	pthread_t self = pthread_self();
	safe_assert(self != PTH_INVALID_THREAD);
	safe_assert(thread->pthread() == self);
	thread->attach_pthread(self);

	Thread::SetCancellable();

	ret_val = thread->Run();

	// runs CleanupHandler
	pthread_cleanup_pop(1);

	return ret_val;
}

/********************************************************************************/

void Thread::SetCancellable() {
	// set cancellable
	int oldstate;
	__pthread_errno__ = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
	safe_assert(__pthread_errno__ == PTH_SUCCESS);

	__pthread_errno__ = pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldstate);
	safe_assert(__pthread_errno__ == PTH_SUCCESS);
}

void Thread::attach_pthread(pthread_t self) {
	safe_assert(pthread_ == self || pthread_ == PTH_INVALID_THREAD);

	set_pthread(self);

	pthread_key_t key = Thread::tls_key();
	safe_assert(NULL == pthread_getspecific(key));

	__pthread_errno__ = pthread_setspecific(key, this);
	safe_assert(__pthread_errno__ == PTH_SUCCESS);
}

/********************************************************************************/

void Thread::detach_pthread(pthread_t self) {
	safe_assert(self != PTH_INVALID_THREAD);
	safe_assert(pthread_ == self);

	pthread_key_t key = Thread::tls_key();
	safe_assert(NULL != pthread_getspecific(key));

	__pthread_errno__ = pthread_setspecific(key, NULL);
	safe_assert(__pthread_errno__ == PTH_SUCCESS);

	set_pthread(PTH_INVALID_THREAD);
}

/********************************************************************************/

Thread* Thread::GetThread(pthread_t self) {
	pthread_key_t key = Thread::tls_key();
	Thread* thread = static_cast<Thread*>(pthread_getspecific(key));
	safe_assert(thread != NULL); // TODO(elmas): can be NULL ?
	safe_assert(self == thread->pthread());
	return thread;
}

/********************************************************************************/

Thread* Thread::Current() {
	pthread_t self = pthread_self();
	safe_assert(self != PTH_INVALID_THREAD);
	return Thread::GetThread(self);
}

/********************************************************************************/

void Thread::Start(pthread_t* pid /*= NULL*/, const pthread_attr_t* attr /*= NULL*/) {
	const pthread_attr_t* attr_ptr = attr;
	pthread_attr_t attr_local;
	if(attr_ptr == NULL && stack_size_ > 0) {
		pthread_attr_init(&attr_local);
		pthread_attr_setstacksize(&attr_local, static_cast<size_t>(stack_size_));
		attr_ptr = &attr_local;
	}
	return_value_ = NULL;
	__pthread_errno__ = PthreadOriginals::pthread_create(&pthread_, attr_ptr, Thread::ThreadEntry, static_cast<void*>(this));
	if(__pthread_errno__ != PTH_SUCCESS) {
		safe_fail("Create error: %s\n", PTHResultToString(__pthread_errno__));
	}
	safe_assert(pthread_ != PTH_INVALID_THREAD);
	if(pid != NULL) *pid = pthread_;
}

/********************************************************************************/

void Thread::Join(void ** value_ptr /*= NULL*/) {
	pthread_t pth = pthread_;
	if(pth == PTH_INVALID_THREAD) {
		// already cancelled or ended, so just exit
		return;
	}

	__pthread_errno__ = PthreadOriginals::pthread_join(pth, value_ptr);
	if(__pthread_errno__ != PTH_SUCCESS && __pthread_errno__ != ESRCH) {
		safe_fail("Join error: %s\n", PTHResultToString(__pthread_errno__));
	}
	if(__pthread_errno__ == PTH_SUCCESS && value_ptr != NULL) {
		safe_assert(*value_ptr == return_value_);
	}
}

/********************************************************************************/

void Thread::Cancel() {
	pthread_t pth = pthread_;
	if(pth == PTH_INVALID_THREAD) {
		// already cancelled or ended, so just exit
		return;
	}
	__pthread_errno__ = PthreadOriginals::pthread_cancel(pth);
	if(__pthread_errno__ != PTH_SUCCESS && __pthread_errno__ != ESRCH) {
		safe_fail("Cancel error: %s\n", PTHResultToString(__pthread_errno__));
	}
}

/********************************************************************************/

void Thread::CancelJoin() {
	this->Cancel();
	this->Join();
	MYLOG(1) << "Cancelled and joined " << tid_;
}

/********************************************************************************/

void Thread::Kill(int signal_number) {
	__pthread_errno__ = pthread_kill(pthread_, signal_number);
	if(__pthread_errno__ != PTH_SUCCESS && __pthread_errno__ != ESRCH) {
		safe_fail("Kill error: %s\n", PTHResultToString(__pthread_errno__));
	}
}

/********************************************************************************/

void Thread::Yield(bool force /*false*/) {
	if (force) {
		sched_yield();
	} else {
		sched_yield(); // _np() !?
	}
}

/********************************************************************************/

Mutex::Mutex() {
//	pthread_mutexattr_t attrs;
//	__pthread_errno__ = pthread_mutexattr_init(&attrs);
//	safe_assert(__pthread_errno__ == PTH_SUCCESS);
//	__pthread_errno__ = pthread_mutexattr_settype(&attrs, PTHREAD_MUTEX_NORMAL);
//	safe_assert(__pthread_errno__ == PTH_SUCCESS);
//	__pthread_errno__ = pthread_mutex_init(&mutex_, &attrs);
//	safe_assert(__pthread_errno__ == PTH_SUCCESS);

	mutex_ = PTHREAD_MUTEX_INITIALIZER;

	owner_ = PTH_INVALID_THREAD;
	count_ = 0;
}

/********************************************************************************/

Mutex::Mutex(pthread_mutex_t m) {
	set_mutex(m);

	owner_ = PTH_INVALID_THREAD;
	count_ = 0;
}

/********************************************************************************/

Mutex::~Mutex() {
	__pthread_errno__ = pthread_mutex_destroy(&mutex_);
	safe_assert(__pthread_errno__ == PTH_SUCCESS);
}

/********************************************************************************/

int Mutex::Lock() {
	pthread_t self = pthread_self();
	safe_assert(self != PTH_INVALID_THREAD);

	if(owner_ == self) {
		safe_assert(count_ >= 1);
		count_++;
	} else {
		__pthread_errno__ = pthread_mutex_lock(&mutex_);
		safe_assert(__pthread_errno__ == PTH_SUCCESS);  // Verify no other errors.

		safe_assert(owner_ == PTH_INVALID_THREAD);
		owner_ = self;

		safe_assert(count_ == 0);
		count_ = 1;

	}
	return PTH_SUCCESS;
}

/********************************************************************************/

int Mutex::Unlock() {
	pthread_t self = pthread_self();
	safe_assert(self != PTH_INVALID_THREAD);

	safe_assert(owner_ == self);
	safe_assert(count_ >= 1);

	count_ --;
	if(count_ == 0) {
		owner_ = PTH_INVALID_THREAD;

		__pthread_errno__ = pthread_mutex_unlock(&mutex_);
		safe_assert(__pthread_errno__ == PTH_SUCCESS);  // Verify no other errors.
	}

	return PTH_SUCCESS;
}

/********************************************************************************/

void Mutex::FullUnlockAux(pthread_t* p_self, int* p_times) {
	// assert locked
	pthread_t self = pthread_self();
	safe_assert(self != PTH_INVALID_THREAD);

	safe_assert(owner_ == self);
	safe_assert(count_ >= 1);

	*p_self = self;
	*p_times = count_;

	owner_ = PTH_INVALID_THREAD;
	count_ = 0;
}

/********************************************************************************/

void Mutex::FullLockAux(pthread_t* p_self, int* p_times) {
	// assert locked
	pthread_t self = pthread_self();
	safe_assert(self != PTH_INVALID_THREAD);

	safe_assert(owner_ == PTH_INVALID_THREAD);
	safe_assert(count_ == 0);

	owner_ = *p_self;
	count_ = *p_times;

	safe_assert(owner_ == self);
	safe_assert(count_ >= 1);
}

/********************************************************************************/

bool Mutex::TryLock() {
	__pthread_errno__ = pthread_mutex_trylock(&mutex_);
	// Return false if the lock is busy and locking failed.
	if (__pthread_errno__ == EBUSY) {
		return false; // already locked
	}
	safe_assert(__pthread_errno__ == PTH_SUCCESS);  // Verify no other errors.

	safe_assert(false);

	return true;
}

/********************************************************************************/

bool Mutex::IsLocked() {
	return owner_ != PTH_INVALID_THREAD;
}

/********************************************************************************/

ConditionVar::ConditionVar() {
	__pthread_errno__ = pthread_cond_init (&cv_, NULL);
	safe_assert(__pthread_errno__ == PTH_SUCCESS);
}

/********************************************************************************/

ConditionVar::ConditionVar(pthread_cond_t v) {
	set_cv(v);
}

/********************************************************************************/

ConditionVar::~ConditionVar() {
	__pthread_errno__ = pthread_cond_destroy(&cv_);
	safe_assert(__pthread_errno__ == PTH_SUCCESS);
}

/********************************************************************************/

int ConditionVar::Signal() {
	__pthread_errno__ = pthread_cond_signal(&cv_);
	safe_assert(__pthread_errno__ == PTH_SUCCESS);
	return __pthread_errno__;
}

/********************************************************************************/

int ConditionVar::Wait(Mutex* mutex) {
	safe_assert(mutex->IsLocked());

	pthread_t self;
	int count;

	mutex->FullUnlockAux(&self, &count);

	__pthread_errno__ = pthread_cond_wait(&cv_, &mutex->mutex_);
	safe_assert(__pthread_errno__ == PTH_SUCCESS);

	mutex->FullLockAux(&self, &count);

	return __pthread_errno__;
}

/********************************************************************************/

int ConditionVar::Broadcast() {
	__pthread_errno__ = pthread_cond_broadcast(&cv_);
	safe_assert(__pthread_errno__ == PTH_SUCCESS);
	return __pthread_errno__;
}

/********************************************************************************/

Semaphore::Semaphore() {
	Init(0);
}

/********************************************************************************/

Semaphore::Semaphore(int count) {
	Init(count);
}

/********************************************************************************/

Semaphore::Semaphore(sem_t* s) {
	set_sem(*s);
}

/********************************************************************************/

Semaphore::~Semaphore() {
	Destroy();
}

/********************************************************************************/

int Semaphore::Get() {
	int count;
	int result = sem_getvalue(&sem_, &count);
	safe_assert(result == PTH_SUCCESS);
	return count;
}

/********************************************************************************/

void Semaphore::Set(int count) {
	if(Get() != count) {
		Destroy();
		Init(count);
	}
}

/********************************************************************************/

void Semaphore::Init(int count) {
	__pthread_errno__ = sem_init(&sem_, 0, count);
	safe_assert(__pthread_errno__ == PTH_SUCCESS);
}


/********************************************************************************/

void Semaphore::Destroy() {
	__pthread_errno__ = sem_destroy(&sem_);
	safe_assert(__pthread_errno__ == PTH_SUCCESS);
}

/********************************************************************************/

void Semaphore::Signal() {
	__pthread_errno__ = sem_post(&sem_);
	safe_assert(__pthread_errno__ == PTH_SUCCESS);
}

/********************************************************************************/

void Semaphore::Wait() {
	while (true) {
		__pthread_errno__ = sem_wait(&sem_);
		if (__pthread_errno__ == PTH_SUCCESS) return;  // Successfully got semaphore.
		safe_assert(__pthread_errno__ == -1 && errno == EINTR);  // Signal caused spurious wakeup.
	}
}

/********************************************************************************/

#ifdef LINUX
#ifndef TIMEVAL_TO_TIMESPEC
#define TIMEVAL_TO_TIMESPEC(tv, ts) do {		\
		(ts)->tv_sec = (tv)->tv_sec;                \
		(ts)->tv_nsec = (tv)->tv_usec * 1000;       \
} while (false)
//#else
//#error "TIMEVAL_TO_TIMESPEC already defined!"
#endif


int Semaphore::WaitTimed(long timeout) {
	if(timeout <= 0) { Wait(); return PTH_SUCCESS; }

	const long kOneSecondMicros = 1000000L;  // NOLINT

	// Split timeout into second and nanosecond parts.
	struct timeval delta;
	delta.tv_usec = timeout % kOneSecondMicros;
	delta.tv_sec = timeout / kOneSecondMicros;

	struct timeval current_time;
	// Get the current time.
	if (gettimeofday(&current_time, NULL) == -1) {
		return errno;
	}

	// Calculate time for end of timeout.
	struct timeval end_time;
	timeradd(&current_time, &delta, &end_time);

	struct timespec ts;
	TIMEVAL_TO_TIMESPEC(&end_time, &ts);
	// Wait for semaphore signalled or timeout.
	while (true) {
		__pthread_errno__ = sem_timedwait(&sem_, &ts);
		if (__pthread_errno__ == PTH_SUCCESS) return PTH_SUCCESS;  // Successfully got semaphore.
		if (__pthread_errno__ > 0) {
			// For glibc prior to 2.3.4 sem_timedwait returns the error instead of -1.
			errno = __pthread_errno__;
			__pthread_errno__ = -1;
		}
		if (__pthread_errno__ == -1 && errno == ETIMEDOUT) return errno;  // Timeout.
		safe_assert(__pthread_errno__ == -1 && errno == EINTR);  // Signal caused spurious wakeup.
	}
}
#endif // LINUX

/********************************************************************************/

RWLock::RWLock() {
	rwlock_ = PTHREAD_RWLOCK_INITIALIZER;
}

/********************************************************************************/

RWLock::RWLock(pthread_rwlock_t m) {
	rwlock_ = PTHREAD_RWLOCK_INITIALIZER;
}

/********************************************************************************/

RWLock::~RWLock() {
	__pthread_errno__ = pthread_rwlock_destroy(&rwlock_);
	safe_assert(__pthread_errno__ == PTH_SUCCESS);
}

/********************************************************************************/

int RWLock::RLock() {
	__pthread_errno__ = pthread_rwlock_rdlock(&rwlock_);
	safe_assert(__pthread_errno__ == PTH_SUCCESS);
	return __pthread_errno__;
}

/********************************************************************************/

int RWLock::RUnlock() {
	return Unlock();
}

/********************************************************************************/

int RWLock::WLock() {
	__pthread_errno__ = pthread_rwlock_wrlock(&rwlock_);
	safe_assert(__pthread_errno__ == PTH_SUCCESS);
	return __pthread_errno__;
}

/********************************************************************************/

int RWLock::WUnlock() {
	return Unlock();
}

/********************************************************************************/

int RWLock::Unlock() {
	__pthread_errno__ = pthread_rwlock_unlock(&rwlock_);
	safe_assert(__pthread_errno__ == PTH_SUCCESS);
	return __pthread_errno__;
}

/********************************************************************************/

bool RWLock::TryRLock() {
	__pthread_errno__ = pthread_rwlock_tryrdlock(&rwlock_);
	// Return false if the lock is busy and locking failed.
	if (__pthread_errno__ == EBUSY) {
		return false; // already locked
	}
	safe_assert(__pthread_errno__ == PTH_SUCCESS);  // Verify no other errors.

	safe_assert(false);

	return true;
}

/********************************************************************************/

bool RWLock::TryWLock() {
	__pthread_errno__ = pthread_rwlock_trywrlock(&rwlock_);
	// Return false if the lock is busy and locking failed.
	if (__pthread_errno__ == EBUSY) {
		return false; // already locked
	}
	safe_assert(__pthread_errno__ == PTH_SUCCESS);  // Verify no other errors.

	safe_assert(false);

	return true;
}

/********************************************************************************/

} // end namespace
