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

#include "ipc.h"

namespace concurrit {

/********************************************************************************/

const THREADID MAINTID = THREADID(1);

/********************************************************************************/

class ShadowThread {
public:
	ShadowThread(THREADID threadid) : tid_(threadid) {
		memset(&event_, 0, sizeof(EventBuffer));

		PipeNamePair pipe_names = PipeNamesForDSL(tid_);
		pipe_.Init(pipe_names);
		pipe_.Open();
	}

	virtual ~ShadowThread(){
		if(pipe_.is_open()) {
			pipe_.Close();
		}
	}

	void SendContinue() {
		event_.type = Continue;
		event_.threadid = tid_;
		pipe_.Send(&event_);
	}

	virtual void* Run();
private:
	DECL_FIELD(THREADID, tid)
	DECL_FIELD(EventPipe, pipe)
	DECL_FIELD(EventBuffer, event)
};

/********************************************************************************/

void* shadow_thread_func(void* arg) {
	safe_assert(arg != NULL);
	ShadowThread* thread = static_cast<ShadowThread*>(arg);
	safe_assert(thread != NULL);

	return thread->Run();
}

/********************************************************************************/

void* ShadowThread::Run() {
	// main loop
	for(pipe_.Recv(&event_); event_.type != TestEnd; pipe_.Recv(&event_)) {

		ShadowThread* thread = NULL;

		// handle event
		switch(event_.type) {
		case ThreadStart:

			safe_assert(event_.threadid >= 0);
			// create new thread, imitating the interpositioned threads
			pthread_t pt;
			thread = new ShadowThread(event_.threadid);
			pthread_create(&pt, NULL, shadow_thread_func, thread);

			SendContinue();

			break;

		case MemAccessBefore:
		case MemAccessAfter:
		case MemWrite:
		case MemRead:
		case FuncCall:
		case FuncEnter:
		case FuncReturn:

			// notify concurrit
			CallPinMonitor(&event_);

			// send continue to remote
			SendContinue();

			break;

		case TestEnd:
			if(tid_ != MAINTID)
				safe_fail("Invalid event type: %d", event_.type);
			break;
		default:
			safe_fail("Invalid event type: %d", event_.type);
			break;
		}
	}

	return NULL;
}

/********************************************************************************/

class MainShadowThread : public ShadowThread {
public:
	MainShadowThread() : ShadowThread(MAINTID) {}
	~MainShadowThread(){}

	void* Run();
};

/********************************************************************************/

void* MainShadowThread::Run() {
	// wait for TestBegin
	int timer = 0;
	for(pipe_.Recv(&event_); event_.type != TestStart; pipe_.Recv(&event_)) {
		if(timer > 1000) {
			safe_fail("Too many iterations for waiting TestBegin!");
		}
		++timer;
	}
	SendContinue();

	// main loop
	ShadowThread::Run();

	// uncontrolled mode
	timer = 0;
	for(; event_.type != TestEnd; pipe_.Recv(&event_)) {
		if(timer > 1000) {
			safe_fail("Too many iterations for waiting TestEnd!");
		}
		++timer;
	}
	SendContinue();

	return NULL;
}


/********************************************************************************/

static
int main0(int argc, char* argv[]) {

	MainShadowThread* thread = new MainShadowThread();
	thread->Run();

	return EXIT_SUCCESS;
}

/********************************************************************************/

} // end namespace

/********************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif
int __main__(int argc, char* argv[]) {
	return concurrit::main0(argc, argv);
}
#ifdef __cplusplus
} // extern "C"
#endif


