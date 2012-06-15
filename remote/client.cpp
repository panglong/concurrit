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

#include "thread.cpp"

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
		safe_fail("Could not set tls value!");
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
	ClientShadowThread(THREADID tid, EventPipe* pipe, ThreadFuncType start_routine = NULL, void* arg = NULL)
	: ShadowThread(tid, pipe), start_routine_(start_routine), arg_(arg)  {}

	virtual ~ClientShadowThread(){}

	// override
	void* Run() {

		ConcurrentPipe* concpipe = static_cast<ConcurrentPipe*>(pipe_);
		safe_assert(concpipe != NULL);

		concpipe->RegisterShadowThread(this);

		// set tls data
		set_tls(this);

		EventBuffer event;

		// send thread start
		event.type = ThreadStart;
		event.threadid = tid_;
		SendRecvContinue(&event);

		safe_assert(start_routine_ != NULL);
		void* ret = start_routine_(arg_);

		// send thread end
		event.type = ThreadEnd;
		event.threadid = tid_;
		SendRecvContinue(&event);

		// clear tls data
		set_tls(NULL);

		concpipe->UnregisterShadowThread(this);

		return ret;
	}

private:
	ThreadFuncType start_routine_;
	void *arg_;
};


/********************************************************************************/

static ConcurrentPipe* pipe_ = NULL;

extern "C"
__attribute__((constructor))
void initialize_client() {
	PthreadOriginals::initialize();

	if(pthread_key_create(&client_tls_key_, NULL) != PTH_SUCCESS) {
		safe_fail("Count not create tls key!");
	}

	pipe_ = new ConcurrentPipe(PipeNamesForSUT(), NULL);

	// create shadow thread for main thread
	main_shadow_thread_ = new ClientShadowThread(MAINTID, pipe_);
	set_tls(main_shadow_thread_);
}

/********************************************************************************/

extern "C"
__attribute__((destructor))
void destroy_client() {

	set_tls(NULL);

	if(pthread_key_delete(client_tls_key_) != PTH_SUCCESS) {
		safe_fail("Count not destroy tls key!");
	}
}

/********************************************************************************/

PTHREADORIGINALS_STATIC_FIELD_DEFINITIONS

/********************************************************************************/

PTHREAD_FUNCTION_DEFINITIONS

/********************************************************************************/

class ClientPthreadHandler : public PthreadHandler {
public:
	ClientPthreadHandler() : PthreadHandler() {}
	~ClientPthreadHandler() {}

	// override
	int pthread_create(pthread_t* thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg) {
		safe_assert(PthreadOriginals::is_initialized() && PthreadOriginals::_pthread_create != NULL);

		safe_assert(pipe_ != NULL);
		ClientShadowThread* shadowthread = new ClientShadowThread(get_next_threadid(), pipe_, start_routine, arg);
		return shadowthread->SpawnAsThread(true);
	}
};

/********************************************************************************/

PTHREADHANDLER_CURRENT_DEFINITION(new ClientPthreadHandler())

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

void concurritStartTest() {
	// this thread is main
	safe_assert(main_shadow_thread_ != NULL);
	safe_assert(get_tls() == main_shadow_thread_);
	safe_assert(main_shadow_thread_->tid() == MAINTID);

	// send test start
	EventBuffer e;
	e.type = TestStart;
	e.threadid = MAINTID;
	main_shadow_thread_->SendRecvContinue(&e);

	// there is no ThreadStart event for the main thread
}

/********************************************************************************/

void concurritEndTest() {
	// this thread is main
	safe_assert(main_shadow_thread_ != NULL);
	safe_assert(get_tls() == main_shadow_thread_);
	safe_assert(main_shadow_thread_->tid() == MAINTID);

	EventBuffer e;

	// send thread end for the main thread
	e.type = ThreadEnd;
	e.threadid = MAINTID;
	main_shadow_thread_->SendRecvContinue(&e);

	// send test end
	e.type = TestEnd;
	e.threadid = MAINTID;
	main_shadow_thread_->SendRecvContinue(&e);
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

