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

/**********************************************************************************/

PipeNamePair PipeNamesForSUT(int id /* = 0 */) {
	safe_assert(id >= 0);

	PipeNamePair pair;
	char buff[256];

	snprintf(buff, 256, PIPEDIR "/fifo_in_%d", id);
	pair.in_name = std::string(buff);

	snprintf(buff, 256, PIPEDIR "/fifo_out_%d", id);
	pair.out_name = std::string(buff);

	return pair;
}

/**********************************************************************************/

PipeNamePair PipeNamesForDSL(int id /* = 0 */) {
	PipeNamePair pair = PipeNamesForSUT(id);
	return {pair.out_name, pair.in_name};
}

/**********************************************************************************/

EventPipe::EventPipe() {
	Init(NULL, NULL);
}

/**********************************************************************************/

EventPipe::EventPipe(const PipeNamePair& names) {
	Init(names);
}

/**********************************************************************************/

void EventPipe::Init(const PipeNamePair& names) {
	Init(names.in_name.c_str(), names.out_name.c_str());
}

/**********************************************************************************/

void EventPipe::Init(const char* in_name, const char* out_name) {
	in_name_ = in_name;
	if(in_name_ != NULL) {
		in_name_ = strdup(in_name_);
		MkFifo(in_name_);
	}

	out_name_ = out_name;
	if(out_name_ != NULL) {
		out_name_ = strdup(out_name_);
		MkFifo(out_name_);
	}

	in_fd_ = out_fd_ = -1;

	is_open_ = false;
}

/**********************************************************************************/

EventPipe::~EventPipe() {
	if(is_open_) {
		Close();
	}

	if(in_name_ != NULL) delete in_name_;
	if(out_name_ != NULL) delete out_name_;
}

/**********************************************************************************/

void EventPipe::MkFifo(const char* name) {
	safe_assert(name != NULL);

	struct stat stt;

	if(stat(name, &stt) != 0) {
		if(errno == ENOENT) {
			int ret_val = mkfifo(name, 0777);

			if ((ret_val == -1) && (errno != EEXIST)) {
				perror("Error in mkfifo\n");
				safe_fail("Error creating the named pipe %s", name);
			}
			safe_assert(stat(name, &stt) == 0);
		} else {
			perror("Could not stat fifo file!\n");
			safe_fail("Error creating the named pipe %s", name);
		}
	}
}

/**********************************************************************************/

void EventPipe::Open(bool open_for_read_first) {
	safe_assert(!is_open_);

	if(open_for_read_first) {

		if(in_name_ != NULL) {
			in_fd_ = open(in_name_, O_RDONLY & ~O_NONBLOCK);
			safe_assert(in_fd_ >= 0);
		}

		if(out_name_ != NULL) {
			out_fd_ = open(out_name_, O_WRONLY & ~O_NONBLOCK);
			safe_assert(out_fd_ >= 0);
		}

	} else {

		if(out_name_ != NULL) {
			out_fd_ = open(out_name_, O_WRONLY & ~O_NONBLOCK);
			safe_assert(out_fd_ >= 0);
		}

		if(in_name_ != NULL) {
			in_fd_ = open(in_name_, O_RDONLY & ~O_NONBLOCK);
			safe_assert(in_fd_ >= 0);
		}
	}

	is_open_ = true;
}

/**********************************************************************************/

void EventPipe::Close() {
	safe_assert(is_open_);

	if(in_name_ != NULL) {
		close(in_fd_);
	}

	if(out_name_ != NULL) {
		close(out_fd_);
	}

	is_open_ = false;
}

/**********************************************************************************/

ShadowThread::ShadowThread(THREADID tid, EventPipe* pipe) : tid_(tid), pipe_(pipe) {
	sem_.Init(0);
	sem2_.Init(0);
	event_.Clear();
}


/**********************************************************************************/

void ShadowThread::SendContinue() {
	EventBuffer e;
	e.type = Continue;
	e.threadid = tid_;
	Send(&e);
}

/**********************************************************************************/

void ShadowThread::Send(EventBuffer* e) {
	e->threadid = tid_;
	pipe_->Send(this, e);

	MYLOG(2) << "IPC: Sent message to " << pipe_->out_name();
}

/**********************************************************************************/

void ShadowThread::Recv(EventBuffer* e) {
	pipe_->Recv(this, e);
}

/**********************************************************************************/

void ShadowThread::SendRecvContinue(EventBuffer* e) {
	// send the event
	this->Send(e);

	// wait for continue
	this->Recv(e);

	if(e->type != Continue) {
		safe_fail("Unexpected event type: %d. Expected continue.", e->type);
	}
}

/**********************************************************************************/

// used by pipe implementation
// when thread wants to receive an event, it waits on its semaphore
void ShadowThread::WaitRecv(EventBuffer* event) {
	safe_assert(event != NULL);

	sem2_.Signal();

	safe_assert(sem_.Get() <= 1);
	sem_.Wait();

	*event = event_;
//	memcpy(event, &event_, sizeof(EventBuffer));

	safe_check(event->Check());
	safe_check(event->threadid == tid_);
}

/**********************************************************************************/

// copy event from argument and signal the semaphore
void ShadowThread::SignalRecv(EventBuffer* event) {
	safe_assert(event != NULL);

	MYLOG(1) << "SignalRecv to thread " << event->threadid << " for event " << EventKindToString(event->type);
	safe_assert(event->threadid == tid_);

	sem2_.Wait();

	// copy to the buffer
	event_ = *event;
//	memcpy(&event_, event_, sizeof(EventBuffer));

	// signal
	if(sem_.Get() > 0) { fprintf(stderr, "Problem is with thread %d and sem %d\n", tid_, sem_.Get()); safe_fail("Assertion!\n"); }
	sem_.Signal();
}

/**********************************************************************************/

void* ShadowThread::thread_func(void* arg) {
	safe_assert(arg != NULL);
	ShadowThread* thread = static_cast<ShadowThread*>(arg);
	safe_assert(thread != NULL);
	return thread->Run();
}

/**********************************************************************************/

int ShadowThread::SpawnAsThread() {
	pthread_t pt;
	return pthread_create(&pt, NULL, thread_func, this);
}

/**********************************************************************************/

ConcurrentPipe::ConcurrentPipe(const PipeNamePair& names, EventHandler* event_handler /*= NULL*/)
: EventPipe(names), event_handler_(event_handler), worker_thread_(NULL) {}

/**********************************************************************************/

void ConcurrentPipe::Send(ShadowThread* thread, EventBuffer* event) {
	safe_assert(event != NULL);
	safe_assert(thread == NULL || thread->tid() == event->threadid);

	ScopeMutex m(&send_mutex_);

	bool cancel = event_handler_ == NULL ? false : !event_handler_->OnSend(this, event);

	if(!cancel) {
		EventPipe::Send(event);
	}
}

/**********************************************************************************/

void ConcurrentPipe::Recv(ShadowThread* thread, EventBuffer* event) {
	safe_assert(event != NULL);
	if(thread == NULL) {
		EventPipe::Recv(event);
	} else {
		thread->WaitRecv(event);
	}
}

/**********************************************************************************/

//override
void ConcurrentPipe::Open(bool open_for_read_first, bool create_worker /*= true*/) {
	EventPipe::Open(open_for_read_first);

	// start thread
	if(create_worker) {
		worker_thread_ = new Thread(56789, ConcurrentPipe::thread_func, this);
		worker_thread_->Start();
	} else {
		worker_thread_ = NULL;
	}
}

ConcurrentPipe* ConcurrentPipe::OpenForDSL(EventHandler* event_handler /*= NULL*/, bool create_worker /*= true*/) {
	ConcurrentPipe* pipe = new ConcurrentPipe(PipeNamesForDSL(0), event_handler);
	pipe->Open(true, create_worker);
	return pipe;
}

ConcurrentPipe* ConcurrentPipe::OpenForSUT(EventHandler* event_handler /*= NULL*/, bool create_worker /*= true*/) {
	ConcurrentPipe* pipe = new ConcurrentPipe(PipeNamesForSUT(0), event_handler);
	pipe->Open(false, create_worker);
	return pipe;
}

ConcurrentPipe* ConcurrentPipe::OpenControlForDSL(EventHandler* event_handler /*= NULL*/, bool create_worker /*= true*/) {
	ConcurrentPipe* pipe = new ConcurrentPipe(PipeNamesForDSL(1), event_handler);
	pipe->Open(true, create_worker);
	return pipe;
}

ConcurrentPipe* ConcurrentPipe::OpenControlForSUT(EventHandler* event_handler /*= NULL*/, bool create_worker /*= true*/) {
	ConcurrentPipe* pipe = new ConcurrentPipe(PipeNamesForSUT(1), event_handler);
	pipe->Open(false, create_worker);
	return pipe;
}

/**********************************************************************************/

//override
void ConcurrentPipe::Close() {
	if(worker_thread_ != NULL) {
		worker_thread_->CancelJoin();
	}
	EventPipe::Close();
}

/**********************************************************************************/

void cleanup(void* name) {
	MYLOG(2) << "IPC: Returning from ConcurrentPipe::thread_func " << (const char*) name;
}

void* ConcurrentPipe::thread_func(void* arg) {
	ConcurrentPipe* pipe = static_cast<ConcurrentPipe*>(arg);
	safe_assert(pipe != NULL);

	EventHandler* event_handler = pipe->event_handler();

	EventBuffer event;

	pthread_cleanup_push(cleanup, (void*)pipe->in_name());

	for(;;) {

		// do receive
		pipe->EventPipe::Recv(&event);

		MYLOG(2) << "IPC: Received message of type "  << EventKindToString(event.type) << " for thread " << event.threadid << " " << pipe->in_name();

		bool cancel = event_handler == NULL ? false : !event_handler->OnRecv(pipe, &event);

		const THREADID tid = event.threadid;
		safe_assert(tid >= 0);

		if(!cancel) {
			// notify receiver
			ShadowThread* thread = pipe->GetShadowThread(tid);
			if(thread != NULL) { // otherwise ignore the message
				safe_assert(thread->tid() == tid);
				thread->SignalRecv(&event);
			} else {
				MYLOG(2) << "IPC: Ignoring message " << EventKindToString(event.type) << " to thread " << event.threadid << " " << pipe->in_name();
				safe_assert(event.type != Continue); // should not ignore continue events!
			}
		}
	}

	pthread_cleanup_pop(1);

	return NULL;
}

/**********************************************************************************/

void ConcurrentPipe::SendContinue(THREADID tid) {
	EventBuffer e;
	e.type = Continue;
	e.threadid = tid;
	Send(NULL, &e);
}

/**********************************************************************************/

void ConcurrentPipe::Broadcast(EventBuffer* e) {
	ScopeMutex m(&map_mutex_);

	for(TidToShadowThreadMap::iterator itr = tid_to_shadowthread_.begin(); itr != tid_to_shadowthread_.end(); ++itr) {
		ShadowThread* shadowthread = itr->second;
		safe_assert(shadowthread != NULL);

		e->threadid = shadowthread->tid();
		shadowthread->SignalRecv(e);
	}
}

/**********************************************************************************/

ShadowThread* ConcurrentPipe::GetShadowThread(THREADID tid) {
	ScopeMutex m(&map_mutex_);

	ShadowThread* thread = NULL;
	TidToShadowThreadMap::iterator itr = tid_to_shadowthread_.find(tid);
	if(itr != tid_to_shadowthread_.end()) {
		thread = itr->second;
		safe_assert(thread != NULL);
	}
	return thread;
}

/**********************************************************************************/

void ConcurrentPipe::RegisterShadowThread(ShadowThread* shadowthread) {
	ScopeMutex m(&map_mutex_);

	const THREADID tid = shadowthread->tid();
	TidToShadowThreadMap::iterator itr = tid_to_shadowthread_.find(tid);
	if(itr == tid_to_shadowthread_.end()) {
		tid_to_shadowthread_[tid] = shadowthread;
	}
}

/**********************************************************************************/

void ConcurrentPipe::UnregisterShadowThread(ShadowThread* shadowthread) {
	ScopeMutex m(&map_mutex_);

	const THREADID tid = shadowthread->tid();
	TidToShadowThreadMap::iterator itr = tid_to_shadowthread_.find(tid);
	safe_check(itr != tid_to_shadowthread_.end());
	tid_to_shadowthread_.erase(itr);}

/**********************************************************************************/

size_t ConcurrentPipe::NumShadowThreads() {
	ScopeMutex m(&map_mutex_);

	return tid_to_shadowthread_.size();
}

/**********************************************************************************/

} // end namespace

