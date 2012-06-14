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

namespace concurrit {

/********************************************************************************/

const THREADID MAINTID = THREADID(1);

/********************************************************************************/

typedef tbb::concurrent_vector<ShadowThread*> ShadowThreadPtrList;
static ShadowThreadPtrList shadow_threads;

/********************************************************************************/

class ServerShadowThread : public ShadowThread {
public:
	ServerShadowThread(THREADID threadid) : ShadowThread(threadid, true)  {}

	virtual ~ServerShadowThread(){}

	// override
	virtual void* Run();
};

/********************************************************************************/

void* shadow_thread_func(void* arg) {
	safe_assert(arg != NULL);
	ShadowThread* thread = static_cast<ShadowThread*>(arg);
	safe_assert(thread != NULL);

	return thread->Run();
}

/********************************************************************************/

void* ServerShadowThread::Run() {
	thread_ = Coroutine::Current();

	// main loop
	for(pipe_.Recv(&event_); event_.type != ThreadEnd; pipe_.Recv(&event_)) {

		// handle event
		switch(event_.type) {
		case ThreadStart:
		{
			safe_assert(event_.threadid >= 0);
			// create new thread, imitating the interpositioned threads
			pthread_t pt;
			ShadowThread* shadowthread = new ServerShadowThread(event_.threadid);

			// add shadow thread to shadow_threads
			shadow_threads.push_back(shadowthread);

			pthread_create(&pt, NULL, shadow_thread_func, shadowthread);

			SendContinue();

			break;
		}
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

		default:
			safe_fail("Invalid event type: %d", event_.type);
			break;
		}
	}

	return NULL;
}

/********************************************************************************/

class MainShadowThread : public ServerShadowThread {
public:
	MainShadowThread() : ServerShadowThread(MAINTID) {}
	~MainShadowThread(){}

	// override
	void* Run();

	void StartShadowThreads();
	void EndShadowThreads();
};

/********************************************************************************/

void* MainShadowThread::Run() {
	// wait for TestBegin
	WaitForEventAndSkip(TestStart);

	StartShadowThreads();

	// main loop
	ShadowThread::Run();

	// uncontrolled mode
	WaitForEventAndSkip(TestEnd);

	EndShadowThreads();

	return NULL;
}

/********************************************************************************/

void MainShadowThread::StartShadowThreads() {
	// imitate creating threads (but actually restarts coroutines)
	for(ShadowThreadPtrList::iterator itr = shadow_threads.begin(), end = shadow_threads.end(); itr != end; ++itr) {
		ShadowThread* shadowthread = (*itr);

		// create new thread, imitating the interpositioned threads
		pthread_t pt;
		pthread_create(&pt, NULL, shadow_thread_func, shadowthread);
	}
}

/********************************************************************************/

void MainShadowThread::EndShadowThreads() {
	// send other threads ThreadEnd signal
	for(ShadowThreadPtrList::iterator itr = shadow_threads.begin(), end = shadow_threads.end(); itr != end; ++itr) {
		ShadowThread* shadowthread = (*itr);

		EventBuffer e;
		e.type = ThreadEnd;
		e.threadid = tid_;
		shadowthread->pipe()->Send(&e);
	}
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

CONCURRIT_TEST_MAIN(concurrit::main0)


