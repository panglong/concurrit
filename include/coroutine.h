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

#ifndef COROUTINE_H_
#define COROUTINE_H_

#include "common.h"
#include "thread.h"
#include "channel.h"
#include "threadvar.h"

namespace concurrit {

class CoroutineGroup;
class ExecutionTree;

#define MAIN_TID 	0

enum MessageType {MSG_INVALID = 0, MSG_STARTED = 1, MSG_TRANSFER = 2, MSG_RESTART = 3, MSG_TERMINATE = 4, MSG_EXCEPTION = 5};

// absent means does not exist, an existing coroutine does not take absent status
typedef int StatusType;
const int ABSENT = 0, PASSIVE = 1, ENABLED = 2, WAITING = 3/*for message*/, BLOCKED = 4, ENDED = 5, TERMINATED = 6;

// for log messages
#define CO_TITLE	"[" << tid_ << "] "

// coroutine is simulated by a thread
class Coroutine : public Thread
{
public:

	explicit Coroutine(THREADID tid, ThreadEntryFunction entry_function, void* entry_arg = NULL, int stack_size = 0);
	virtual ~Coroutine();

	void Start(pthread_t* pid = NULL, const pthread_attr_t* attr = NULL);
	virtual void* Run();

	void Finish();
	bool WaitForEnd(long timeout = -1);

	static void StartMain();
	static void FinishMain();
	bool IsMain();

	void SetStarted();
	void SetEnded();

	static Coroutine* Current();

	void Transfer(Coroutine* target, MessageType msg = MSG_TRANSFER);
	void Transfer(Channel<MessageType>* channel, MessageType msg = MSG_TRANSFER);

	virtual void HandleMessage(MessageType msg);

//	SchedulePoint* OnYield(Coroutine* target, std::string& label, SourceLocation* loc = NULL, SharedAccess* access = NULL);
//	void OnAccess(SharedAccess* access);

	// return the next access this coroutine will do when scheduled
//	AccessLocPair GetNextAccess();

	inline bool is_ended() {
		return (status_ >= ENDED);
	}

	void StartControlledTransition();
	void FinishControlledTransition();

	inline char* instr_callback_info() {
		return instr_callback_info_;
	}

private:

	DECL_VOL_FIELD(StatusType, status)
	DECL_FIELD(CoroutineGroup*, group)
	DECL_FIELD_REF(Channel<MessageType>, channel)

//	DECL_FIELD(SchedulePoint*, yield_point)

//	DECL_FIELD(VC, vc)

//	DECL_FIELD_REF(TransitionInfoList, trinfolist)
//	DECL_FIELD(ExecutionTree*, current_node)

	DECL_FIELD(std::exception*, exception)
	DECL_FIELD_REF(Semaphore, sem_end)

	DECL_FIELD(ThreadVarPtr, tvar)

//	DECL_FIELD(bool, is_driver_thread)

	DECL_FIELD(SourceLocation*, srcloc)
	char instr_callback_info_[256];

	DECL_STATIC_FIELD(Coroutine*, main)

	DISALLOW_COPY_AND_ASSIGN(Coroutine)
};

typedef std::set<Coroutine*> CoroutinePtrSet;
#define for_each_coroutine(s, co) \
	CoroutinePtrSet::iterator __itr__ = (s).begin(); \
	Coroutine* co = (__itr__ != (s).end() ? (*__itr__) : NULL); \
	for (; __itr__ != (s).end(); co = ((++__itr__) != (s).end() ? (*__itr__) : NULL))


/*
 * we define coroutine_t as a pointer to a Coroutine instance
 * coroutine_t allows us to make coroutines transparent to the testcase
 */
typedef Coroutine* coroutine_t;

} // end namespace

#endif /* COROUTINE_H_ */
