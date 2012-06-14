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

#include "instrument.h"

#include "tbb/concurrent_vector.h"

#include <cstdatomic>

namespace concurrit {

/********************************************************************************/

const THREADID MAINTID = THREADID(1);

static THREADID get_next_threadid() {
	static std::atomic<THREADID> next_threadid(2);
	return next_threadid.fetch_add(1);
}

/********************************************************************************/

class ClientShadowThread;

static pthread_key_t client_tls_key_;
static ClientShadowThread* main_shadow_thread_ = NULL;

/********************************************************************************/

static void set_tls(ClientShadowThread* value) {
	const pthread_key_t key = client_tls_key_;
	safe_assert(NULL == pthread_getspecific(key));

	if(pthread_setspecific(key, value) != PTH_SUCCESS) {
		safe_fail("Count not set tls value!");
	}
}

static ClientShadowThread* get_tls() {
	const pthread_key_t key = client_tls_key_;
	ClientShadowThread* thread = static_cast<ClientShadowThread*>(pthread_getspecific(key));
	safe_assert(thread != NULL);
	return thread;
}

/********************************************************************************/

class ClientShadowThread : public ShadowThread {
public:
	ClientShadowThread(THREADID threadid, ThreadFuncType start_routine = NULL, void* arg = NULL)
	: ShadowThread(threadid, false), start_routine_(start_routine), arg_(arg)  {}

	virtual ~ClientShadowThread(){}

	// override
	void* Run() {
		safe_assert(start_routine_ != NULL);

		// set tls data
		set_tls(this);

		SendThreadStart();

		void* ret = start_routine_(arg_);

		SendThreadEnd();

		return ret;
	}

	void SendThreadStart() {
		EventBuffer e;
		e.type = ThreadStart;
		e.threadid = tid_;
		pipe_.Send(&e);
	}

	void SendThreadEnd() {
		EventBuffer e;
		e.type = ThreadEnd;
		e.threadid = tid_;
		pipe_.Send(&e);
	}

private:
	ThreadFuncType start_routine_;
	void *arg_;
};

/********************************************************************************/

extern "C"
__attribute__((constructor))
void initialize_client() {
	PthreadOriginals::initialize();

	if(pthread_key_create(&client_tls_key_, NULL) != PTH_SUCCESS) {
		safe_fail("Count not create tls key!");
	}

	// create shadow thread for main thread
	main_shadow_thread_ = new ClientShadowThread(MAINTID);
	set_tls(main_shadow_thread_);
}

/********************************************************************************/

extern "C"
__attribute__((destructor))
void destroy_client() {

	if(pthread_key_delete(client_tls_key_) != PTH_SUCCESS) {
		safe_fail("Count not destroy tls key!");
	}
}

/********************************************************************************/

PTHREADORIGINALS_STATIC_FIELD_DEFINITIONS

/********************************************************************************/

PTHREAD_FUNCTION_DEFINITIONS

/********************************************************************************/

void* shadow_thread_func(void* arg) {
	safe_assert(arg != NULL);
	ShadowThread* thread = static_cast<ShadowThread*>(arg);
	safe_assert(thread != NULL);

	return thread->Run();
}

class ClientPthreadHandler : public PthreadHandler {
public:
	ClientPthreadHandler() : PthreadHandler() {}
	~ClientPthreadHandler() {}

	// override
	int pthread_create(pthread_t* thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg) {
		safe_assert(PthreadOriginals::is_initialized() && PthreadOriginals::_pthread_create != NULL);
		return PthreadOriginals::pthread_create(thread, attr,
					shadow_thread_func,
					new ClientShadowThread(get_next_threadid(), start_routine, arg));
	}
};

/********************************************************************************/

PTHREADHANDLER_CURRENT_DEFINITION(new ClientPthreadHandler())

/********************************************************************************/

/********************************************************************************/

void concurritStartTest() {
	// this thread is main
	safe_assert(main_shadow_thread_ != NULL);
	safe_assert(get_tls() == main_shadow_thread_);

	EventBuffer e;
	e.type = TestStart;
	e.threadid = MAINTID;
	main_shadow_thread_->pipe()->Send(&e);
}

/********************************************************************************/

void concurritEndTest() {
	// this thread is main
	safe_assert(main_shadow_thread_ != NULL);
	safe_assert(get_tls() == main_shadow_thread_);

	EventBuffer e;
	e.type = TestEnd;
	e.threadid = MAINTID;
	main_shadow_thread_->pipe()->Send(&e);
}

/********************************************************************************/

void concurritEndSearch() {
	//...
}

void concurritStartInstrumentEx(const char* filename, const char* funcname, int line) {
	//...
}

void concurritEndInstrumentEx(const char* filename, const char* funcname, int line) {
	//...
}

void concurritAtPcEx(int pc, const char* filename, const char* funcname, int line) {
	//...
}

void concurritFuncEnterEx(void* addr, uintptr_t arg0, uintptr_t arg1, const char* filename, const char* funcname, int line) {
	//...
}
void concurritFuncReturnEx(void* addr, uintptr_t retval, const char* filename, const char* funcname, int line) {
	//...
}

void concurritFuncCallEx(void* from_addr, void* to_addr, uintptr_t arg0, uintptr_t arg1, const char* filename, const char* funcname, int line) {
	//...
}

void concurritMemReadEx(void* addr, size_t size, const char* filename, const char* funcname, int line) {
	//...
}
void concurritMemWriteEx(void* addr, size_t size, const char* filename, const char* funcname, int line) {
	//...
}

void concurritMemAccessBeforeEx(const char* filename, const char* funcname, int line) {
	//...
}
void concurritMemAfterBeforeEx(const char* filename, const char* funcname, int line) {
	//...
}

void concurritThreadEndEx(const char* filename, const char* funcname, int line) {
	//...
}

void concurritTriggerAssert(const char* expr, const char* filename, const char* funcname, int line) {
	//...
}


/********************************************************************************/

} // end namespace

/********************************************************************************/

