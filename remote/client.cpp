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

#include <cstdatomic>

namespace concurrit {

/********************************************************************************/

static std::atomic<THREADID> next_threadid(2);
static THREADID get_next_threadid() {
	return next_threadid.fetch_add(1, std::memory_order_seq_cst);
}

/********************************************************************************/

class ClientShadowThread;

static pthread_key_t client_tls_key_;

static ConcurrentPipe* pipe_ = NULL;

/********************************************************************************/

static void create_tls_key() {
	if(pthread_key_create(&client_tls_key_, NULL) != PTH_SUCCESS) {
		safe_fail("Count not create tls key!");
	}
}

static void delete_tls_key() {
	if(pthread_key_delete(client_tls_key_) != PTH_SUCCESS) {
		safe_fail("Count not destroy tls key!");
	}
}

// value may not be NULL
static ClientShadowThread* get_tls() {
	ClientShadowThread* thread = static_cast<ClientShadowThread*>(pthread_getspecific(client_tls_key_));
	return thread;
}

// value may be NULL
static void set_tls(ClientShadowThread* value) {
	safe_assert(get_tls() == NULL);
	if(pthread_setspecific(client_tls_key_, value) != PTH_SUCCESS) {
		safe_fail("Could not set tls value!");
	}
}

/********************************************************************************/

class ClientShadowThread : public ShadowThread {
public:
	ClientShadowThread(THREADID tid, EventPipe* pipe)
	: ShadowThread(tid, pipe)  {}

	~ClientShadowThread(){}

	void OnStart() {
		safe_assert(pipe_ != NULL);
		ConcurrentPipe* concpipe = static_cast<ConcurrentPipe*>(pipe_);
		concpipe->RegisterShadowThread(this);

		// set tls data
		set_tls(this);

		// send thread start
		EventBuffer event;
		event.type = ThreadStart;
		event.threadid = tid_;
		SendRecvContinue(&event);
	}

	void OnEnd() {
		// send thread end
		EventBuffer event;
		event.type = ThreadEnd;
		event.threadid = tid_;
		SendRecvContinue(&event);

		// clear tls data
		set_tls(NULL);

		safe_assert(pipe_ != NULL);
		ConcurrentPipe* concpipe = static_cast<ConcurrentPipe*>(pipe_);
		concpipe->UnregisterShadowThread(this);
	}

	// override
	void* Run() {
		unimplemented();
		return NULL;
	}
};

/********************************************************************************/
class ClientInstrHandler : public InstrHandler {
public:
	ClientInstrHandler() : InstrHandler() {}
	~ClientInstrHandler() {}

	/********************************************************************************/

	static ClientShadowThread* GetShadowThread() {
		ClientShadowThread* thread = get_tls();
		if(thread == NULL) {
			safe_assert(pipe_ != NULL);
			thread = new ClientShadowThread(get_next_threadid(), pipe_);
			thread->OnStart();
		}
		return thread;
	}

	/********************************************************************************/

	// override
	void concurritStartTest() {
		MYLOG(1) << "CLIENT: concurritStartTest.";

		// send test start
		EventBuffer e;
		e.type = TestStart;
		e.threadid = 0;
		pipe_->Send(NULL, &e);
	}

	/********************************************************************************/

	// override
	void concurritEndTest() {
		MYLOG(1) << "CLIENT: concurritStartTest.";

		// send test end
		EventBuffer e;
		e.type = TestEnd;
		e.threadid = 0;
		pipe_->Send(NULL, &e);
	}

	/********************************************************************************/

//	void concurritEndSearch();
//
//	void concurritStartInstrumentEx(const char* filename, const char* funcname, int line);
//	void concurritEndInstrumentEx(const char* filename, const char* funcname, int line);
//
	void concurritAtPcEx(int pc, const char* filename, const char* funcname, int line) {
		ClientShadowThread* thread = GetShadowThread();

		// send event
		EventBuffer e;
		e.type = AtPc;
		e.pc = pc;
		thread->SendRecvContinue(&e);
	}

	void concurritFuncEnterEx(void* addr, uintptr_t arg0, uintptr_t arg1, const char* filename, const char* funcname, int line) {
		ClientShadowThread* thread = GetShadowThread();

		// send event
		EventBuffer e;
		e.type = FuncEnter;
		e.addr = PTR2ADDRINT(addr);
		e.arg0 = arg0;
		e.arg1 = arg1;
		thread->SendRecvContinue(&e);

	}
	void concurritFuncReturnEx(void* addr, uintptr_t retval, const char* filename, const char* funcname, int line) {
		ClientShadowThread* thread = GetShadowThread();

		// send event
		EventBuffer e;
		e.type = FuncReturn;
		e.addr = PTR2ADDRINT(addr);
		e.retval = retval;
		thread->SendRecvContinue(&e);
	}
//
//	void concurritFuncCallEx(void* from_addr, void* to_addr, uintptr_t arg0, uintptr_t arg1, const char* filename, const char* funcname, int line);
//
//	void concurritMemReadEx(void* addr, size_t size, const char* filename, const char* funcname, int line);
//	void concurritMemWriteEx(void* addr, size_t size, const char* filename, const char* funcname, int line);
//
//	void concurritMemAccessBeforeEx(const char* filename, const char* funcname, int line);
//	void concurritMemAccessAfterEx(const char* filename, const char* funcname, int line);
//
	// override
//	void concurritThreadStartEx(const char* filename, const char* funcname, int line) {
//		ClientShadowThread* thread = GetShadowThread();
//
//		// GetShadowThread does all
//	}
//
//	// override
//	void concurritThreadEndEx(const char* filename, const char* funcname, int line) {
//		ClientShadowThread* thread = GetShadowThread();
//
//		// this call does all
//		thread->OnEnd();
//	}

//
//	void concurritTriggerAssert(const char* expr, const char* filename, const char* funcname, int line);
};

/********************************************************************************/

extern "C"
__attribute__((constructor))
void construct_client() {

	InstrHandler::Current = new ClientInstrHandler();

	create_tls_key();

	pipe_ = new ConcurrentPipe(PipeNamesForSUT());
	pipe_->Open(false);
}

/********************************************************************************/

extern "C"
__attribute__((destructor))
void destroy_client() {
	safe_assert(pipe_ != NULL);
	pipe_->Close();

	safe_delete(pipe_);

	delete_tls_key();
}

/********************************************************************************/

} // end namespace


