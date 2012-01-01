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


Coroutine::Coroutine(const char* name, ThreadEntryFunction entry_function, void* entry_arg, int stack_size)
: Thread(name, entry_function, entry_arg, stack_size)
{
	status_ = PASSIVE;
	group_ = NULL;
	yield_point_ = NULL;
	coid_ = -1;
	vc_clear(vc_);
}

/********************************************************************************/

Coroutine::~Coroutine() {
	// noop
}

/********************************************************************************/

void Coroutine::StartMain() {
	safe_assert(name_ == MAIN_NAME);
	status_ = ENABLED;
	set_coid(0);
	pthread_t pth_self = pthread_self();
	safe_assert(pth_self != PTH_INVALID_THREAD);
	// for non-main coroutines, this is called in ThreadEntry
	attach_pthread(pth_self);
}

void Coroutine::FinishMain() {
	safe_assert(name_ == MAIN_NAME);
	status_ = TERMINATED;
	set_coid(-1);
	pthread_t pth_self = pthread_self();
	safe_assert(pth_self != PTH_INVALID_THREAD);
	// for non-main coroutines, this is called in ThreadEntry
	detach_pthread(pth_self);
}

bool Coroutine::IsMain() {
	if(name() == MAIN_NAME) {
		safe_assert(CHECK_NOTNULL(group_)->main() == this);
		return true;
	}
	return false;
}

/********************************************************************************/

Coroutine* Coroutine::Current() {
	Coroutine* co = ASINSTANCEOF(Thread::Current(), Coroutine*);
	safe_assert(co != NULL);

	safe_assert(co->group_ == NULL || co->group_->current() == co);

	return co;
}

/********************************************************************************/

void Coroutine::Restart() {
	safe_assert(status_ > PASSIVE); // no need to start a non-started one

	// reset yield point to null
	yield_point_ = NULL;

	vc_clear(vc_);

	this->Start();
}

/********************************************************************************/

// call this only for non-main coroutines.
// for main, FinishMain is called when ending the entire testing environment.
void Coroutine::Finish() {
	safe_assert(!IsMain());
	safe_assert(status_ > PASSIVE); // no need to start an non-started one
	if(status_ < TERMINATED) { // if terminated, we are done
		// send the finish message
		VLOG(2) << CO_TITLE << "Sending finish signal";
		MessageType msg = MSG_TERMINATE;
		this->channel_.SendNoWait(msg);
		this->Join();
	}
}

/********************************************************************************/

void Coroutine::Start() {
	safe_assert(yield_point_ == NULL);

	Channel<MessageType>::BeginAtomic();

	if(status_ == PASSIVE || status_ == TERMINATED) {
		VLOG(2) << CO_TITLE << "Starting new thread";
		Thread::Start();
	} else if(status_ < TERMINATED) {
		// send the restart message
		MessageType msg = MSG_RESTART;
		channel_.SendNoWait(msg);
	}

	// check if this is the main (for now only main can start coroutines)
	safe_assert(Coroutine::Current() == group_->main());
	// wait for started message
	VLOG(2) << CO_TITLE << "Waiting the thread to start";
	MessageType msg = channel_.WaitReceive();
	CHECK(msg == MSG_STARTED) << "Expected a started message from " << this->name();
	VLOG(2) << CO_TITLE << "Got start signal from the thread";

	Channel<MessageType>::EndAtomic();
}

/********************************************************************************/

void* Coroutine::Run() {
	safe_assert(!this->IsMain());
	void* return_value = NULL;
	CHECK(status_ == PASSIVE) << "Wrong status " << status_ << ", expected " << PASSIVE;

	for(;true;) {
		try {
			safe_assert(status_ >= PASSIVE);

			status_ = ENABLED;

			return_value = NULL;

			CoroutineGroup* group = CHECK_NOTNULL(group_);
			safe_assert(group != NULL);

			// notify main about out startup
			VLOG(2) << CO_TITLE << "Sending started message and waiting for a transfer.";
			MessageType msg = MSG_STARTED;
			msg = channel_.SendWaitReceive(&channel_, msg);
			HandleMessage(msg);

			VLOG(2) << CO_TITLE << "First transfer";

			try {
				// run the actual function
				return_value = call_function();

				status_ = ENDED;

				// last yield
				yield(ENDING_LABEL);

			} catch(std::exception* e) {
				// record the exception in scenario
				group->set_exception(e);
				// send main exception message
				this->Transfer(group->main(), MSG_EXCEPTION);
			}

		} catch(MessageType& m) {
			// only terminate and restart messages are thrown
			if(m == MSG_TERMINATE) {
				VLOG(2) << CO_TITLE << "terminating";
				break; // terminate
			} else if(m == MSG_RESTART) {
				VLOG(2) << CO_TITLE << "restarting";
				continue;
			} else {
				bool invalid_message_type = false;
				safe_assert(invalid_message_type);
			}
		}
	} // end loop

	status_ = TERMINATED;

	return return_value;
}

/********************************************************************************/

void Coroutine::Transfer(Coroutine* target, MessageType msg /*=MSG_TRANSFER*/) {
	safe_assert(target != NULL && target != this);

	VLOG(2) << CO_TITLE << "Transferring to " << target->name();

	// note: this may be main
	// check current coroutine
	safe_assert(this == Coroutine::Current());

	CoroutineGroup* group = this->group();
	safe_assert(group != NULL);
	safe_assert(group->current() == this);

	// update current to target
	group->set_current(target);

	// send-receive main
	msg = this->channel()->SendWaitReceive(target->channel(), msg);
	this->HandleMessage(msg);
}

/********************************************************************************/

void Coroutine::HandleMessage(MessageType msg) {
	VLOG(2) << CO_TITLE << "Handling message:" << msg;

	if(msg == MSG_TRANSFER) {
		BeginStrand(name_.c_str()); // this is to notify the PIN tool
	} else if(msg == MSG_RESTART || msg == MSG_TERMINATE) {
		throw msg; // let the Run() method catch it.
	} else if(msg == MSG_EXCEPTION) {
		safe_assert(this->IsMain());
		// notify scenario about exception
		std::exception* e = group_->exception();
		CHECK(e != NULL) << "MSG_EXCEPTION is received but no exception is found!";
		group_->set_exception(NULL);
		group_->scenario()->OnException(e);
	} else {
		bool invalid_message_type = false;
		safe_assert(invalid_message_type);
	}
}

/********************************************************************************/

SchedulePoint* Coroutine::OnYield(Coroutine* target, std::string& label, SourceLocation* loc /*=NULL*/, SharedAccess* access /*=NULL*/) {
	safe_assert(IsMain() || label != MAIN_LABEL);
	// record the yield point as our last yield point
	// if the label is the same, update the same yield point
	SchedulePoint* point = this->yield_point_;
	if(point != NULL) {
		// point is not null
		safe_assert(point->IsResolved());
		safe_assert(point->AsYield()->source() == this);
		size_t count = point->AsYield()->count();
		safe_assert(count > 0);
		if(!point->IsTransfer() && point->AsYield()->label() == label) {
			point->AsYield()->set_count(count + 1);
			// update access and location
			point->AsYield()->update_access_loc(access, loc);
			return point;
		}
	}
	// create a new point
	if(target == NULL) {
		safe_assert(this->IsMain());
		// we need a new yield point
		VLOG(2) << CO_TITLE << "Main::OnYield generating a new yield point with label " << label;
		point = new YieldPoint(this, label, 1, loc, access, true /*free_target*/, true /*free_count*/);
	} else if(target->IsMain()) {
		safe_assert(!this->IsMain());
		// we need a new yield point
		VLOG(2) << CO_TITLE << "Main::OnYield generating a new yield point with label " << label;
		point = new YieldPoint(this, label, 1, loc, access, false /*free_target*/, true /*free_count*/);
	} else {
		VLOG(2) << CO_TITLE << "OnYield generating a new transfer point with label " << label;
		point = new TransferPoint(new YieldPoint(this, label, 1, loc, access, false /*free_target*/, true /*free_count*/), target);
	}

	safe_assert(point != NULL);
	this->yield_point_ = point;
	safe_assert(point->IsResolved());

	return point;
}

/********************************************************************************/

AccessLocPair Coroutine::GetNextAccess() {
	SchedulePoint* point = yield_point_;
	if(point == NULL) {
		return AccessLocPair(); // no access
	}
	return AccessLocPair(point->AsYield()->access(), point->AsYield()->loc());
}

/********************************************************************************/

void Coroutine::OnAccess(SharedAccess* access) {
	group_->scenario()->OnAccess(this, access);
}

/********************************************************************************/

} // end namespace

