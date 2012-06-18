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

	snprintf(buff, 256, PIPEDIR "/in_%d", id);
	pair.in_name = std::string(buff);

	snprintf(buff, 256, PIPEDIR "/out_%d", id);
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

	MYLOG(1) << "Creating fifo " << name;

	struct stat stt;

	if(stat(name, &stt) != 0) {
		if(errno == ENOENT) {
			int ret_val = mkfifo(name, 0666);

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

	MYLOG(1) << "Created fifo " << name;
}

/**********************************************************************************/

void EventPipe::Open(bool open_for_read_first) {
	safe_assert(!is_open_);

	if(open_for_read_first) {

		if(in_name_ != NULL) {
			in_fd_ = open(in_name_, O_RDONLY & ~O_NONBLOCK);
		}

		if(out_name_ != NULL) {
			out_fd_ = open(out_name_, O_WRONLY & ~O_NONBLOCK);
		}

	} else {

		if(out_name_ != NULL) {
			out_fd_ = open(out_name_, O_WRONLY & ~O_NONBLOCK);
		}

		if(in_name_ != NULL) {
			in_fd_ = open(in_name_, O_RDONLY & ~O_NONBLOCK);
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

ShadowThread::ShadowThread(THREADID tid, EventPipe* pipe) : tid_(tid), pipe_(pipe), event_(NULL) {
	memset(&event_, 0, sizeof(EventBuffer));
	sem_.Init(0);
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
	pipe_->Send(this, e);
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
	safe_assert(event_ == NULL);
	safe_assert(event != NULL);
	event_ = event;

	safe_assert(sem_.Get() <= 1);
	sem_.Wait();
}

/**********************************************************************************/

// copy event from argument and signal the semaphore
void ShadowThread::SignalRecv(EventBuffer* event) {
	// copy
	safe_assert(event_ != NULL);
	memcpy(event_, event, sizeof(EventBuffer));

	safe_assert(tid_ == event_->threadid);

	event_ = NULL;

	// signal
	safe_assert(sem_.Get() <= 0);
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
: EventPipe(names), event_handler_(event_handler != NULL ? event_handler : new EventHandler()), worker_thread_(NULL) {
	safe_assert(event_handler_ != NULL);
}

/**********************************************************************************/

void ConcurrentPipe::Send(ShadowThread* thread, EventBuffer* event) {
	safe_assert(event != NULL);
	safe_assert(thread == NULL || thread->tid() == event->threadid);

	send_mutex_.Lock();

	safe_assert(event_handler_ != NULL);
	bool cancel = !event_handler_->OnSend(event);

	if(!cancel) {
		EventPipe::Send(event);
	}

	send_mutex_.Unlock();
}

/**********************************************************************************/

void ConcurrentPipe::Recv(ShadowThread* thread, EventBuffer* event) {
	safe_assert(thread != NULL && event != NULL);
	thread->WaitRecv(event);
}

/**********************************************************************************/

//override
void ConcurrentPipe::Open(bool open_for_read_first) {
	EventPipe::Open(open_for_read_first);

	MYLOG(1) << "Creating worker thread";

	// start thread
	safe_assert(event_handler_ != NULL);
	worker_thread_ = new Thread(56789, ConcurrentPipe::thread_func, this);
	worker_thread_->Start();
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

void* ConcurrentPipe::thread_func(void* arg) {
	ConcurrentPipe* pipe = static_cast<ConcurrentPipe*>(arg);
	safe_assert(pipe != NULL);

	EventHandler* event_handler = pipe->event_handler();
	safe_assert(event_handler != NULL);

	EventBuffer event;

	MYLOG(1) << "ConcurrentPipe worker thread starting";

	for(;;) {
		// do receive
		pipe->EventPipe::Recv(&event);

		bool cancel = !event_handler->OnRecv(&event);

		if(!cancel) {
			// notify receiver
			ShadowThread* thread = pipe->GetShadowThread(event.threadid);
			if(thread != NULL) { // otherwise ignore the message

				thread->SignalRecv(&event);
			}
		}
	}

	MYLOG(1) << "ConcurrentPipe worker thread ending";

	return NULL;
}

/**********************************************************************************/

void ConcurrentPipe::Broadcast(EventBuffer* e) {
	map_mutex_.Lock();
	for(TidToShadowThreadMap::const_iterator itr = tid_to_shadowthread_.begin(), end = tid_to_shadowthread_.end(); itr != end; ++itr) {
		ShadowThread* shadowthread = itr->second;

		e->threadid = shadowthread->tid();
		shadowthread->SignalRecv(e);
	}
	map_mutex_.Unlock();
}

void ConcurrentPipe::SendContinue(THREADID tid) {
	EventBuffer e;
	e.type = Continue;
	e.threadid = tid;
	Send(NULL, &e);
}

/**********************************************************************************/

ShadowThread* ConcurrentPipe::GetShadowThread(THREADID tid) {
	ShadowThread* thread = NULL;
	map_mutex_.Lock();
	TidToShadowThreadMap::iterator itr = tid_to_shadowthread_.find(tid);
	if(itr != tid_to_shadowthread_.end()) {
		thread = itr->second;
	}
	map_mutex_.Unlock();
	return thread;
}

/**********************************************************************************/

void ConcurrentPipe::RegisterShadowThread(ShadowThread* shadowthread) {
	const THREADID tid = shadowthread->tid();
	map_mutex_.Lock();
	TidToShadowThreadMap::iterator itr = tid_to_shadowthread_.find(tid);
	if(itr == tid_to_shadowthread_.end()) {
		tid_to_shadowthread_[tid] = shadowthread;
	}
	map_mutex_.Unlock();
}

/**********************************************************************************/

void ConcurrentPipe::UnregisterShadowThread(ShadowThread* shadowthread) {
	map_mutex_.Lock();
	size_t num = tid_to_shadowthread_.erase(shadowthread->tid());
	map_mutex_.Unlock();
	safe_assert(num == 1);
}

/**********************************************************************************/

} // end namespace

