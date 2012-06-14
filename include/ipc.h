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


#ifndef IPC_H_
#define IPC_H_

#include "common.h"

#include <sys/stat.h>
#include <fcntl.h>
// #include <fullduplex.h>

namespace concurrit {

/********************************************************************************/
#define PIPEDIR	"/tmp/concurrit/pipe"

struct PipeNamePair {
	std::string in_name;
	std::string out_name;
};

inline PipeNamePair PipeNamesForSUT(THREADID tid) {
	safe_assert(tid >= 0);

	PipeNamePair pair;
	char buff[256];

	snprintf(buff, 256, PIPEDIR "/tid%d_0", tid);
	pair.in_name = std::string(buff);

	snprintf(buff, 256, PIPEDIR "/tid%d_1", tid);
	pair.out_name = std::string(buff);

	return pair;
}

inline PipeNamePair PipeNamesForDSL(THREADID tid) {
	PipeNamePair pair = PipeNamesForSUT(tid);
	return {pair.out_name, pair.in_name};
}

/********************************************************************************/

class EventPipe {
public:
	EventPipe() {
		Init(NULL, NULL);
	}

	EventPipe(PipeNamePair& names) {
		Init(names);
	}

	void Init(PipeNamePair& names) {
		Init(names.in_name.c_str(), names.out_name.c_str());
	}


	void Init(const char* in_name, const char* out_name) {
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

	~EventPipe() {
		if(in_name_ != NULL) delete in_name_;
		if(out_name_ != NULL) delete out_name_;
	}

	static void MkFifo(const char* name) {
		safe_assert(name != NULL);

		int ret_val = mkfifo(name, 0666);

		if ((ret_val == -1) && (errno != EEXIST)) {
			safe_fail("Error creating the named pipe %s", name);
		}
	}

	void Open() {
		safe_assert(!is_open_);

		if(in_name_ != NULL) {
			in_fd_ = open(in_name_, O_RDONLY);
		}

		if(out_name_ != NULL) {
			out_fd_ = open(out_name_, O_WRONLY);
		}

		is_open_ = true;
	}

	void Close() {
		safe_assert(is_open_);

		if(in_name_ != NULL) {
			close(in_fd_);
		}

		if(out_name_ != NULL) {
			close(out_fd_);
		}

		is_open_ = false;
	}

#define DoSend(x)	write(out_fd_, static_cast<void*>(&x), sizeof(x))
#define DoRecv(x)	read(in_fd_, static_cast<void*>(&x), sizeof(x))

#define DECL_SEND_RECV(F) 			\
	void F(EventBuffer* event) {	\
		Do##F(event->type);			\
		Do##F(event->threadid);		\
		switch(event->type) {		\
		case MemAccessBefore:		\
		case MemAccessAfter:		\
		case ThreadEnd:				\
			break;					\
		case MemRead:				\
		case MemWrite:				\
			Do##F(event->addr);		\
			Do##F(event->size);		\
			break;					\
		case FuncEnter:				\
			Do##F(event->addr);		\
			Do##F(event->arg0);		\
			Do##F(event->arg1);		\
			break;					\
		case FuncReturn:			\
			Do##F(event->addr);		\
			Do##F(event->retval);	\
			break;					\
		case FuncCall:				\
			Do##F(event->addr);		\
			Do##F(event->addr_target); \
			Do##F(event->arg0);		\
			Do##F(event->arg1);		\
			break;					\
		}							\
	}								\

	DECL_SEND_RECV(Send)

	DECL_SEND_RECV(Recv)

private:
	DECL_FIELD(const char*, in_name)
	DECL_FIELD(const char*, out_name)
	DECL_FIELD(int, in_fd)
	DECL_FIELD(int, out_fd)
	DECL_FIELD(bool, is_open)
};

/********************************************************************************/

class ShadowThread {
public:
	ShadowThread(THREADID threadid, bool is_server) : tid_(threadid), thread_(NULL) {
		memset(&event_, 0, sizeof(EventBuffer));

		PipeNamePair pipe_names = is_server ? PipeNamesForDSL(tid_) : PipeNamesForSUT(tid_);
		pipe_.Init(pipe_names);
		pipe_.Open();
	}

	virtual ~ShadowThread(){
		if(pipe_.is_open()) {
			pipe_.Close();
		}
	}

	void WaitForEventAndSkip(EventKind type) {
		int timer = 0;
		for(pipe_.Recv(&event_); event_.type != type; pipe_.Recv(&event_)) {
			if(timer > 1000) {
				safe_fail("Too many iterations for waiting event kind %d!", type);
			}
			++timer;
		}
		SendContinue();
	}

	void SendContinue() {
		event_.type = Continue;
		event_.threadid = tid_;
		pipe_.Send(&event_);
	}

	virtual void* Run() = 0;

private:
	DECL_FIELD(THREADID, tid)
	DECL_FIELD_REF(EventPipe, pipe)
	DECL_FIELD(EventBuffer, event)
	DECL_FIELD(Coroutine*, thread)
};

/********************************************************************************/


} // end namespace

#endif /* IPC_H_ */
