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
#include "schedule.h"
#include "thread.h"
#include "channel.h"
#include "vc.h"

namespace concurrit {

class CoroutineGroup;
class SchedulePoint;

#define MAIN_NAME 	"main"

enum MessageType {MSG_STARTED = 1, MSG_TRANSFER = 2, MSG_RESTART = 3, MSG_TERMINATE = 4, MSG_EXCEPTION = 5};

// absent means does not exist, an existing coroutine does not take absent status
typedef int StatusType;
const int ABSENT = 0, PASSIVE = 1, ENABLED = 2, ENDED = 3, TERMINATED = 4;

// for log messages
#define CO_TITLE	"[[" << name_ << "]] "

// coroutine is simulated by a thread
class Coroutine : public Thread
{
public:

	explicit Coroutine(const char* name, ThreadEntryFunction entry_function, void* entry_arg = NULL, int stack_size = 0);
	virtual ~Coroutine();

	void Start();
	virtual void* Run();

	void Restart();
	void Finish();

	void StartMain();
	void FinishMain();
	bool IsMain();

	static Coroutine* Current();

	void Transfer(Coroutine* target, MessageType msg = MSG_TRANSFER);

	virtual void HandleMessage(MessageType msg);

	SchedulePoint* OnYield(Coroutine* target, std::string& label, SourceLocation* loc = NULL, SharedAccess* access = NULL);
	void OnAccess(SharedAccess* access);

	// return the next access this coroutine will do when scheduled
	AccessLocPair GetNextAccess();

	// operations about delaying and resuming a coroutine
	inline bool IsDelayed() {
		return status_ < 0;
	}

	inline void MakeDelayed() {
		CHECK(!IsDelayed());
		status_ = 0 - status_;
	}
	inline void CancelDelayed() {
		CHECK(IsDelayed());
		status_ = abs(status_);
	}

	inline bool is_ended() {
		return (status_ >= ENDED);
	}

private:

	DECL_FIELD(THREADID, coid)
	DECL_FIELD(StatusType, status)
	DECL_FIELD(CoroutineGroup*, group)
	DECL_FIELD_REF(Channel<MessageType>, channel)

	DECL_FIELD(SchedulePoint*, yield_point)

	DECL_FIELD(VC, vc)

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
