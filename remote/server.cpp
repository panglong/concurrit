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

const THREADID MAINTID = THREADID(1);

/********************************************************************************/

static volatile bool test_started = false;

/********************************************************************************/

class ServerShadowThread : public ShadowThread {
public:
	ServerShadowThread(THREADID tid, EventPipe* pipe) : ShadowThread(tid, pipe)  {}

	virtual ~ServerShadowThread(){}

	// override
	void* Run() {

		ConcurrentPipe* concpipe = static_cast<ConcurrentPipe*>(pipe_);
		safe_assert(concpipe != NULL);

		concpipe->RegisterShadowThread(this);

		EventBuffer event;

		for(;;) {
			Recv(&event);

			// check threadid
			safe_assert(event.type == tid_);

			// handle event
			switch(event.type) {
				case ThreadEndInternal:
				{
					// exit loop and return
					goto L_THREADEND;
				}

				case MemAccessBefore:
				case MemAccessAfter:
				case MemWrite:
				case MemRead:
				case FuncCall:
				case FuncEnter:
				case FuncReturn:
				case ThreadEnd:

					// notify concurrit
					CallPinMonitor(&event);

					// send continue to remote
					this->SendContinue();

					if(event.type == ThreadEnd) {
						// exit loop and return
						goto L_THREADEND;
					}

					break;

				default:
					safe_fail("Unexpected event type: %d", event.type);
					break;
			}
		}

	L_THREADEND:

		concpipe->UnregisterShadowThread(this);

		MYLOG(1) << "SERVER: Thread is ending tid = " << tid_;

		return NULL;
	}
};

/********************************************************************************/

class ServerEventHandler : public EventHandler {
public:
	ServerEventHandler(ConcurrentPipe* pipe = NULL) : EventHandler(), pipe_(pipe) {
		test_end_sem_.Init(0);
	}
	virtual ~ServerEventHandler(){}

	//override
	bool OnRecv(EventBuffer* event) {

		// handle event
		switch(event->type) {
		case TestStart:
		{
			safe_assert(event->threadid == 0);

			if(test_started) {
				safe_fail("Received TestStart before TestEnd!");
			}
			test_started = true;

			MYLOG(1) << "SERVER: Test starting.";

			concurritStartTest();

			return false; // ignore it
		}

		case TestEnd:
		{
			safe_assert(event->threadid == 0);

			if(!test_started) {
				safe_fail("Received TestEnd before TestStart!");
			}
			test_started = false;

			concurritEndTest();

			// broadcast thread-end signal to all threads
			EventBuffer e;
			e.type = ThreadEndInternal;
			pipe_->Broadcast(&e);

			// signal semaphore to end the program
			test_end_sem_.Signal();

			MYLOG(1) << "SERVER: Test ending.";

			return false; // ignore it
		}

		case ThreadStart:
		{
			safe_assert(event->threadid >= 2);

			ShadowThread* thread = CreateShadowThread(event->threadid);
			thread->SendContinue();

			return false; // ignore it
		}

		default:
			if(!test_started) {
				// response immediately and ignore
				pipe_->SendContinue(event->threadid);
				return false;
			}

			MYLOG(1) << "SERVER: Received event " << event->type;

			// forward to the recipient

			// before forwarding, check if the thread exists, otherwise, start it
			ShadowThread* thread = pipe_->GetShadowThread(event->threadid);
			if(thread == NULL) {
				safe_assert(event->threadid >= 2);

				CreateShadowThread(event->threadid);
			}

			return true;
		}
	}

	ShadowThread* CreateShadowThread(THREADID tid) {
		MYLOG(1) << "SERVER: Starting new thread with tid " << tid;

		// create new thread, imitating the interpositioned threads
		ShadowThread* shadowthread = new ServerShadowThread(tid, safe_notnull(pipe_));
		shadowthread->SpawnAsThread();

		// wait a little
		usleep(10);

		return shadowthread;
	}

private:
	DECL_FIELD_REF(Semaphore, test_end_sem)
	DECL_FIELD(ConcurrentPipe*, pipe)
};

/********************************************************************************/

static
int main0(int argc, char* argv[]) {

	test_started = false;

	MYLOG(1) << "SERVER: __main__ starting.";

	// TODO(elmas): make them static and global
	ServerEventHandler handler;

	ConcurrentPipe pipe(PipeNamesForDSL(), &handler);
	handler.set_pipe(&pipe);

	pipe.Open(true);

	MYLOG(1) << "SERVER: Done with initialization. Waiting on test_end_semaphore.";

	handler.test_end_sem()->Wait();

	pipe.Close();

	MYLOG(1) << "SERVER: __main__ exiting.";

	return EXIT_SUCCESS;
}

/********************************************************************************/

} // end namespace

/********************************************************************************/

extern "C"
int __main__(int argc, char* argv[]) {
	return concurrit::main0(argc, argv);
}


