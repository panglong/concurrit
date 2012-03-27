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

#ifndef THREAD_H_
#define THREAD_H_

#include "common.h"

namespace concurrit {

// return code from pthread_... functions when the operation is successful
#define PTH_SUCCESS 0
// pthread_t value indicating an invalid thread identifier
#define PTH_INVALID_THREAD 0

static const char* PTHResultToString(int result) {
#define CASE_RESULT(R)	case R: return #R;
	switch(result) {
		CASE_RESULT(EINVAL)
		CASE_RESULT(ESRCH)
		CASE_RESULT(EDEADLK)
		CASE_RESULT(EAGAIN)
		CASE_RESULT(EPERM)
	}
	return "UNKNOWN\n";
}

/********************************************************************************/

static const unsigned int MAX_THREAD_NAME_LENGTH = 16;

typedef void* (*ThreadEntryFunction)(void* arg);
extern void* ThreadEntry(void* arg);

class Thread {
public:

	Thread(const char* name, ThreadEntryFunction entry_function, void* entry_arg = NULL, int stack_size = 0);
	virtual ~Thread() {}

	virtual void Start();
	void Join();
	void Cancel();
	virtual void* Run();
	static void Yield(bool force = false);

	static Thread* GetThread(pthread_t t);
	static Thread* Current();

protected:

	void* call_function();

	inline void set_name(const char* name) {
		safe_assert(strlen(name) < MAX_THREAD_NAME_LENGTH);
		name_ = std::string(name);
	}

	void attach_pthread(pthread_t self);
	void detach_pthread(pthread_t self);

	static void init_tls_key();
	static void delete_tls_key();

private:

	DECL_FIELD_GET(std::string, name)
	DECL_FIELD(ThreadEntryFunction, entry_function)
	DECL_FIELD(void*, entry_arg)
	DECL_FIELD(int, stack_size)
	DECL_FIELD(pthread_t, pthread)

	DECL_STATIC_FIELD(pthread_key_t, tls_key)

	// disallow copy an assign
	DISALLOW_COPY_AND_ASSIGN(Thread)

	friend void* ThreadEntry(void* arg);
	friend void BeginCounit(int argc, char **argv);
	friend void EndCounit();
};


/********************************************************************************/
class ConditionVar;

class Mutex {
public:
	Mutex();
	explicit Mutex(pthread_mutex_t m);
	virtual ~Mutex();

	virtual int Lock();
	virtual int Unlock();
	virtual bool TryLock();
	virtual bool IsLocked();

	virtual void FullLockAux(pthread_t* self, int* times);
	virtual void FullUnlockAux(pthread_t* self, int* times);

private:
	DECL_FIELD(pthread_mutex_t, mutex)
	DECL_VOL_FIELD(pthread_t, owner)
	DECL_VOL_FIELD(int, count)

	friend class ConditionVar;
};
/********************************************************************************/

class ConditionVar {
public:
	ConditionVar();
	explicit ConditionVar(pthread_cond_t v);
	virtual ~ConditionVar();

	virtual int Signal();
	virtual int Wait(Mutex* mutex);
	virtual int Broadcast();

private:
	DECL_FIELD(pthread_cond_t, cv)
};

/********************************************************************************/

class Semaphore {
public:
	Semaphore();
	Semaphore(int count);
	explicit Semaphore(sem_t* s);
	virtual ~Semaphore();

	virtual void Wait();
	virtual void Down() { this->Wait(); }
	virtual void P() { this->Wait(); }

#ifdef LINUX
	virtual int WaitTimed(long timeout);
	virtual int DownTimed(long timeout) { return WaitTimed(timeout); }
	virtual int PTimed(long timeout) { return WaitTimed(timeout); }
#endif

	virtual void Signal();
	virtual void Up() { this->Signal(); }
	virtual void V() { this->Signal(); }

private:
	DECL_FIELD(sem_t, sem)

	friend void BeginCounit(int argc, char **argv);
	friend void EndCounit();
};

} // end namespace

#endif /* THREAD_H_ */
