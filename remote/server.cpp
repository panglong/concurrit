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
static ConcurrentPipe* clientpipe_ = NULL;
static ConcurrentPipe* controlpipe_ = NULL;

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

		MYLOG(1) << "SERVER: Thread " << tid_ << " is starting!";

		ConcurrentPipe* concpipe = static_cast<ConcurrentPipe*>(clientpipe_);
		safe_assert(concpipe != NULL);

		// already registered to pipe in the constructor

		EventBuffer event;

		// signal the creator
		start_sem_.Signal();

		for(;;) {
//			if(!test_started) goto L_THREADEND;

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
		start_sem_.Init(0);
		end_sem_.Init(0);
	}

	virtual ~ServerEventHandler(){}

	//override
	bool OnRecv(EventPipe* pipe, EventBuffer* event) {
		safe_assert(event != NULL);

		ScopeMutex m(&mutex_);

		ConcurrentPipe* concpipe = static_cast<ConcurrentPipe*>(pipe);
		safe_assert(concpipe != NULL);

		MYLOG(1) << "SERVER: Received event " << EventKindToString(event->type);

		const THREADID tid = event->threadid;

		safe_check(event->type != Continue);

		// handle event
		switch(event->type) {
		case TestStart:
		{
			safe_assert(tid == 0);

			StartTest(concpipe);

			return false; // ignore it
		}

		case TestEnd:
		{
			safe_assert(tid == 0);

			EndTest(concpipe);

			return false; // ignore it
		}

		case ThreadStart:
		{
			if(!test_started) {
				// response immediately and ignore
				concpipe->SendContinue(tid);
				return false;
			}
			safe_assert(tid >= 2);

			ShadowThread* thread = CreateShadowThread(concpipe, tid);
			MYLOG(1) << "Sending continue signal to ThreadStart";
			concpipe->SendContinue(tid);
			MYLOG(1) << "Sent continue signal to ThreadStart";

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

		// wait for the thread to start running
		shadowthread->start_sem()->Wait();

		MYLOG(1) << "SERVER: Started new thread with tid = " << tid;

		return shadowthread;
	}

	void StartTest(ConcurrentPipe* concpipe) {
		if(test_started) {
			// we ignore this event, but let control thread continue
			controlpipe_->SendContinue(0);
			return;
		}

		start_sem_.Wait();

		MYLOG(1) << "SERVER: Test starting.";

		test_started = true;

		// run monitor callback
		concurritStartTest();

		MYLOG(1) << "SERVER: Notifying client about TestStart";
		// notify the client
		EventBuffer e;
		e.type = TestStart;
		e.threadid = 0;
		clientpipe_->Send(NULL, &e);

		MYLOG(1) << "SERVER: Replying TestStart with continue";
		// let control thread continue
		controlpipe_->SendContinue(0);

		MYLOG(1) << "SERVER: Test started.";
	}

	void EndTest(ConcurrentPipe* concpipe) {
		if(!test_started) {
			// we ignore this event, but let control thread continue
			controlpipe_->SendContinue(0);
			return;
		}

		ScopeMutex m(&mutex_);

		MYLOG(1) << "SERVER: Test ending.";

		test_started = false;

		EventBuffer e;

		MYLOG(1) << "SERVER: Notifying client about TestEnd";
		// notify the client
		e.type = TestEnd;
		e.threadid = 0;
		clientpipe_->Send(NULL, &e);

		MYLOG(1) << "SERVER: Broadcasting ThreadEndIntern";

		// broadcast thread-end signal to all threads
		e.type = ThreadEndIntern;
		clientpipe_->Broadcast(&e);

		MYLOG(1) << "SERVER: Broadcast to threads";

		// wait until all threads end
		while(clientpipe_->NumShadowThreads() > 0) {
			Thread::Yield(true);
		}

		if(concpipe != clientpipe_) {
			e.type = ThreadEndIntern;
			concpipe->Broadcast(&e);

			while(concpipe->NumShadowThreads() > 0) {
				Thread::Yield(true);
			}
		}

		// signal semaphore to end the program
//		Scenario::NotNullCurrent()->test_end_sem()->Signal();

		MYLOG(1) << "SERVER: Replying TestEnd with continue";
		// let control thread continue
		controlpipe_->SendContinue(0);

		MYLOG(1) << "SERVER: Test ended.";

		end_sem_.Signal();

//		PthreadOriginals::pthread_exit(NULL);
	}

private:
	DECL_FIELD(Mutex, mutex)
	DECL_FIELD_REF(Semaphore, start_sem)
	DECL_FIELD_REF(Semaphore, end_sem)
};

/********************************************************************************/

ServerEventHandler handler_;
static ConcurrentPipe* clientpipe__ = ConcurrentPipe::OpenForDSL(&handler_);
static ConcurrentPipe* controlpipe__ = ConcurrentPipe::OpenControlForDSL(&handler_);

/********************************************************************************/

static
int main0(int argc, char* argv[]) {

	clientpipe_ = clientpipe__;
	controlpipe_ = controlpipe__;

	safe_assert(handler_.end_sem()->Get() == 0);
	safe_assert(handler_.start_sem()->Get() == 0);

	handler_.start_sem()->Signal();

	MYLOG(1) << "SERVER: __main__ starting.";

//	Scenario::NotNullCurrent()->test_end_sem()->Wait();
	handler_.end_sem()->Wait();

	MYLOG(1) << "SERVER: end_sem signalled!";

//	if(test_started) {
//		handler.EndTest(clientpipe_);
//	}

	// run monitor callback
	concurritEndTest();

	safe_check(!test_started);
	MYLOG(1) << "SERVER: __main__ exiting.";

	return EXIT_SUCCESS;
}

extern "C"
__attribute__((destructor))
void destroy_client() {
	MYLOG(2) << "SERVER: Destroying server!";

	clientpipe_->Close();
	controlpipe_->Close();

	delete(clientpipe_);
	delete(controlpipe_);
}

/********************************************************************************/

} // end namespace

/********************************************************************************/

extern "C"
int __main__(int argc, char* argv[]) {
	return concurrit::main0(argc, argv);
}


