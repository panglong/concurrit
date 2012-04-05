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


/********************************************************************************/

pthread_key_t Thread::tls_key_;

/********************************************************************************/

void Thread::init_tls_key() {
	pthread_key_t key;
	int result = pthread_key_create(&key, NULL);
	safe_assert(result == PTH_SUCCESS);

	tls_key_ = key;
}

/********************************************************************************/

void Thread::delete_tls_key() {
	pthread_key_t key = tls_key_;

	int result = pthread_key_delete(key);
	safe_assert(result == PTH_SUCCESS);
}

/********************************************************************************/

Thread::Thread(THREADID tid, ThreadEntryFunction entry_function, void* entry_arg, int stack_size)
: entry_function_(entry_function),
  entry_arg_(entry_arg),
  stack_size_(stack_size) {
	safe_assert(stack_size_ >= 0);
	set_tid(tid);
	set_pthread(PTH_INVALID_THREAD);
}

/********************************************************************************/

void* Thread::call_function() {
	ThreadEntryFunction func = entry_function_;
	void* arg = entry_arg_;
	safe_assert(func != NULL);
	return func(arg);
}

/********************************************************************************/

void* Thread::Run() {
	return call_function();
}

/********************************************************************************/

void* ThreadEntry(void* arg) {
	// sometimes the threads starts before pthread_creates takes affect,
	// so ensure that we give enough time to pthread_create
	sched_yield();

	Thread* thread = reinterpret_cast<Thread*>(arg);

	pthread_t self = pthread_self();
	safe_assert(self != PTH_INVALID_THREAD);
	safe_assert(thread->pthread() == self);
	thread->attach_pthread(self);

	// set cancellable
	int oldstate;
	int result = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
	safe_assert(result == PTH_SUCCESS);

	result = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldstate);
	safe_assert(result == PTH_SUCCESS);

	void* return_value = thread->Run();

	thread->detach_pthread(self);

	// no need to call pthread_exit
	return return_value;
}

/********************************************************************************/

void Thread::attach_pthread(pthread_t self) {
	safe_assert(pthread_ == self || pthread_ == PTH_INVALID_THREAD);

	set_pthread(self);

	pthread_key_t key = Thread::tls_key();
	safe_assert(NULL == pthread_getspecific(key));

	int result = pthread_setspecific(key, this);
	safe_assert(result == PTH_SUCCESS);
}

/********************************************************************************/

void Thread::detach_pthread(pthread_t self) {
	safe_assert(self != PTH_INVALID_THREAD);
	safe_assert(pthread_ == self);

	set_pthread(PTH_INVALID_THREAD);

	pthread_key_t key = Thread::tls_key();
	safe_assert(NULL != pthread_getspecific(key));

	int result = pthread_setspecific(key, NULL);
	safe_assert(result == PTH_SUCCESS);
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

void Thread::Start(pthread_t* pid /*= NULL*/, pthread_attr_t* attr /*= NULL*/) {
	pthread_attr_t* attr_ptr = attr;
	pthread_attr_t attr_local;
	if(attr_ptr == NULL && stack_size_ > 0) {
		pthread_attr_init(&attr_local);
		pthread_attr_setstacksize(&attr_local, static_cast<size_t>(stack_size_));
		attr_ptr = &attr_local;
	}
	Originals::pthread_create(&pthread_, attr_ptr, ThreadEntry, static_cast<void*>(this));
	safe_assert(pthread_ != PTH_INVALID_THREAD);
	if(pid != NULL) *pid = pthread_;
}

/********************************************************************************/

void Thread::Join() {
	int result = Originals::pthread_join(pthread_, NULL);
	if(result != PTH_SUCCESS && result != ESRCH) {
		printf("Join error: %s\n", PTHResultToString(result));
	}
}

/********************************************************************************/

void Thread::Cancel() {
	int result = pthread_cancel(pthread_);
	if(result != PTH_SUCCESS && result != ESRCH) {
		printf("Cancel error: %s\n", PTHResultToString(result));
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
	pthread_mutexattr_t attrs;
	int result = pthread_mutexattr_init(&attrs);
	safe_assert(result == PTH_SUCCESS);
	result = pthread_mutexattr_settype(&attrs, PTHREAD_MUTEX_NORMAL);
	safe_assert(result == PTH_SUCCESS);
	result = pthread_mutex_init(&mutex_, &attrs);
	safe_assert(result == PTH_SUCCESS);

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
	int result = pthread_mutex_destroy(&mutex_);
	safe_assert(result == PTH_SUCCESS);
}

/********************************************************************************/

int Mutex::Lock() {
	pthread_t self = pthread_self();
	safe_assert(self != PTH_INVALID_THREAD);

	if(owner_ == self) {
		safe_assert(count_ >= 1);
		count_++;
	} else {
		int result = pthread_mutex_lock(&mutex_);
		safe_assert(result == PTH_SUCCESS);  // Verify no other errors.

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

		int result = pthread_mutex_unlock(&mutex_);
		safe_assert(result == PTH_SUCCESS);  // Verify no other errors.
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
	int result = pthread_mutex_trylock(&mutex_);
	// Return false if the lock is busy and locking failed.
	if (result == EBUSY) {
		return false; // already locked
	}
	safe_assert(result == PTH_SUCCESS);  // Verify no other errors.

	safe_assert(false);

	return true;
}

/********************************************************************************/

bool Mutex::IsLocked() {
	return owner_ != PTH_INVALID_THREAD;
}

/********************************************************************************/

ConditionVar::ConditionVar() {
	int result = pthread_cond_init (&cv_, NULL);
	safe_assert(result == PTH_SUCCESS);
}

/********************************************************************************/

ConditionVar::ConditionVar(pthread_cond_t v) {
	set_cv(v);
}

/********************************************************************************/

ConditionVar::~ConditionVar() {
	int result = pthread_cond_destroy(&cv_);
	safe_assert(result == PTH_SUCCESS);
}

/********************************************************************************/

int ConditionVar::Signal() {
	int result = pthread_cond_signal(&cv_);
	safe_assert(result == PTH_SUCCESS);
	return result;
}

/********************************************************************************/

int ConditionVar::Wait(Mutex* mutex) {
	safe_assert(mutex->IsLocked());

	pthread_t self;
	int count;

	mutex->FullUnlockAux(&self, &count);

	int result = pthread_cond_wait(&cv_, &mutex->mutex_);
	safe_assert(result == PTH_SUCCESS);

	mutex->FullLockAux(&self, &count);

	return result;
}

/********************************************************************************/

int ConditionVar::Broadcast() {
	int result = pthread_cond_broadcast(&cv_);
	safe_assert(result == PTH_SUCCESS);
	return result;
}

/********************************************************************************/

Semaphore::Semaphore() {
	int result = sem_init(&sem_, 0, 0);
	safe_assert(result == PTH_SUCCESS);
}

/********************************************************************************/

Semaphore::Semaphore(int count) {
	int result = sem_init(&sem_, 0, count);
	safe_assert(result == PTH_SUCCESS);
}

/********************************************************************************/

Semaphore::Semaphore(sem_t* s) {
	set_sem(*s);
}

/********************************************************************************/

Semaphore::~Semaphore() {
	int result = sem_destroy(&sem_);
	safe_assert(result == PTH_SUCCESS);
}

/********************************************************************************/

void Semaphore::Signal() {
	int result = sem_post(&sem_);
	safe_assert(result == PTH_SUCCESS);
}

/********************************************************************************/

void Semaphore::Wait() {
	while (true) {
		int result = sem_wait(&sem_);
		if (result == PTH_SUCCESS) return;  // Successfully got semaphore.
		safe_assert(result == -1 && errno == EINTR);  // Signal caused spurious wakeup.
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
		int result = sem_timedwait(&sem_, &ts);
		if (result == PTH_SUCCESS) return 0;  // Successfully got semaphore.
		if (result > 0) {
			// For glibc prior to 2.3.4 sem_timedwait returns the error instead of -1.
			errno = result;
			result = -1;
		}
		if (result == -1 && errno == ETIMEDOUT) return errno;  // Timeout.
		safe_assert(result == -1 && errno == EINTR);  // Signal caused spurious wakeup.
	}
}
#endif // LINUX

} // end namespace
