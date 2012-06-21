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

static volatile bool test_started = false;

/********************************************************************************/

// TODO(elmas): delete unused shadow threads

class ServerShadowThread : public ShadowThread {
public:
	ServerShadowThread(THREADID tid, EventPipe* pipe) : ShadowThread(tid, pipe)  {
		start_sem_.Init(0);
	}

	~ServerShadowThread(){}

	// override
	void* Run() {

		ConcurrentPipe* concpipe = static_cast<ConcurrentPipe*>(pipe_);
		safe_assert(concpipe != NULL);

		// already registered to pipe in the constructor

		EventBuffer event;

		// signal the creator
		start_sem_.Signal();

		for(;;) {
			Recv(&event);

			// check threadid
			safe_assert(event.threadid == tid_);

			MYLOG(1) << "SERVER: Thread " << tid_ << " received event kind " << EventKindToString(event.type);

			// handle event
			switch(event.type) {
				case ThreadEndIntern:
					// exit loop and return
					goto L_THREADEND;

				case MemAccessBefore:
					concurritMemAccessBefore();
					break;

				case MemAccessAfter:
					concurritMemAccessAfter();
					break;

				case MemWrite:
					concurritMemWrite(event.addr, event.size);
					break;

				case MemRead:
					concurritMemRead(event.addr, event.size);
					break;

				case FuncCall:
					concurritFuncCall(event.addr, event.addr_target, event.arg0, event.arg1);
					break;

				case FuncEnter:
					concurritFuncEnter(event.addr, event.arg0, event.arg1);
					break;

				case FuncReturn:
					concurritFuncReturn(event.addr, event.retval);
					break;

				case ThreadEnd:
					concurritThreadEnd();
					break;

				case AtPc:
					concurritAtPc(event.pc);
					break;

				case AddressOfSymbol:
					concurritAddressOfSymbol(event.str, event.addr);
					break;

				default:
					safe_fail("Unexpected event type: %d", event.type);
					break;
			}

			MYLOG(1) << "SERVER: Thread " << tid_ << " handled event kind " << EventKindToString(event.type);

			// send continue to remote
			this->SendContinue();

			if(event.type == ThreadEnd) {
				// exit loop and return
				goto L_THREADEND;
			}

		} // end for(;;)

	L_THREADEND:

		concpipe->UnregisterShadowThread(this);

		MYLOG(1) << "SERVER: Thread " << tid_ << " is ending.";

		// ending the thread will trigger the event ThreadEnd in concurrit.

		return NULL;
	}
private:
	DECL_FIELD_REF(Semaphore, start_sem)
};

/********************************************************************************/

class ServerEventHandler : public EventHandler {
public:
	ServerEventHandler() : EventHandler() {
		test_end_sem_.Init(0);
	}
	virtual ~ServerEventHandler(){}

	//override
	bool OnRecv(EventPipe* pipe, EventBuffer* event) {
		safe_assert(event != NULL);

		ConcurrentPipe* concpipe = static_cast<ConcurrentPipe*>(pipe);
		safe_assert(concpipe != NULL);

		MYLOG(1) << "SERVER: Received event " << EventKindToString(event->type);

		const THREADID tid = event->threadid;

		// handle event
		switch(event->type) {
		case TestStart:
		{
			safe_assert(tid == 0);

			if(test_started) {
				safe_fail("Received TestStart before TestEnd!");
			}
			test_started = true;

			MYLOG(1) << "SERVER: Test starting.";

			// run monitor callback
			concurritStartTest();

			return false; // ignore it
		}

		case TestEnd:
		{
			safe_assert(tid == 0);

			if(!test_started) {
				safe_fail("Received TestEnd before TestStart!");
			}
			test_started = false;

			// run monitor callback
			concurritEndTest();

			// broadcast thread-end signal to all threads
			EventBuffer e;
			e.type = ThreadEndIntern;
			concpipe->Broadcast(&e);

			MYLOG(1) << "SERVER: after broadcast.";

			// wait until all threads end
			while(concpipe->NumShadowThreads() > 0) {
				Thread::Yield(true);
			}

			MYLOG(1) << "SERVER: Signaling test_end_sem_.";

			// signal semaphore to end the program
			test_end_sem_.Signal();

			MYLOG(1) << "SERVER: Test ending.";

			return false; // ignore it
		}

		case ThreadStart:
		{
			safe_assert(tid >= 2);

			ShadowThread* thread = CreateShadowThread(concpipe, tid);
			thread->SendContinue();

			return false; // ignore it
		}

		case AddressOfSymbol:
		{
			HandleEvent(concpipe, event, tid);

			return true;
		}

		default:
			if(!test_started) {
				// response immediately and ignore
				concpipe->SendContinue(tid);
				return false;
			}

			HandleEvent(concpipe, event, tid);

			// forward to the recipient
			return true;
		}
	}

	void HandleEvent(ConcurrentPipe* pipe, EventBuffer* event, const THREADID tid) {
		MYLOG(1) << "SERVER: Handling event " << EventKindToString(event->type);

		// before forwarding, check if the thread exists, otherwise, start it
		ShadowThread* thread = pipe->GetShadowThread(tid);
		if(thread == NULL) {
			safe_assert(tid >= 2);

			CreateShadowThread(pipe, tid);
		}
	}

	ShadowThread* CreateShadowThread(ConcurrentPipe* pipe, THREADID tid) {
		MYLOG(1) << "SERVER: Starting new thread with tid = " << tid;

		// create new thread, imitating the interpositioned threads
		ServerShadowThread* shadowthread = new ServerShadowThread(tid, pipe);

		// we do not register it in Run() of the thread, but here to avoid an ordering problem
		pipe->RegisterShadowThread(shadowthread);

		// this triggers the thread creation callback
		shadowthread->SpawnAsThread();

		// wait for the thread to start waiting
		shadowthread->start_sem()->Wait();

		return shadowthread;
	}

private:
	DECL_FIELD_REF(Semaphore, test_end_sem)
};

/********************************************************************************/

static
int main0(int argc, char* argv[]) {

	test_started = false;

	MYLOG(1) << "SERVER: __main__ starting.";

	// TODO(elmas): make them static and global
	ServerEventHandler handler;

	ConcurrentPipe* pipe = ConcurrentPipe::OpenForDSL(&handler);

	MYLOG(1) << "SERVER: Done with initialization. Waiting on test_end_semaphore.";

	handler.test_end_sem()->Wait();

	MYLOG(1) << "SERVER: After waiting test_end_semaphore.";

	pipe->Close();

	delete(pipe);

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


