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
	exception_ = NULL;
	transfer_on_start_ = false;
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
	exception_ = NULL;
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

//	safe_assert(co->group_ == NULL || co->group_->CheckCurrent(co));

	return co;
}

/********************************************************************************/

// call this only for non-main coroutines.
// for main, FinishMain is called when ending the entire testing environment.
void Coroutine::Finish() {
	safe_assert(!IsMain());
	safe_assert(status_ > PASSIVE); // no need to start an non-started one
	if(status_ < TERMINATED) { // if terminated, we are done
		if(status_ == ENDED || status_ == WAITING) {
			// send the finish message
			VLOG(2) << CO_TITLE << "Sending finish signal";
			this->channel_.SendNoWait(MSG_TERMINATE);
			this->Join();
			VLOG(2) << CO_TITLE << "Joined the thread";
		} else {
			// kill the thread
			VLOG(2) << CO_TITLE << "Cancelling the thread";
			this->Cancel();
			VLOG(2) << CO_TITLE << "Waiting for the thread";
			this->Join();
		}
	}

	status_ = TERMINATED;
}

/********************************************************************************/

void Coroutine::WaitForEnd() {
	safe_assert(BETWEEN(ENABLED, status_, ENDED));
 	VLOG(2) << "Waiting for coroutine " << name_ << " to end.";
 	// wait for the semaphore twice
	sem_end_.Wait();
	sem_end_.Wait(); // Wait(1000000); // wait for 1 sec
	VLOG(2) << "Detected coroutine " << name_ << " has ended.";
	safe_assert(status_ == ENDED);
//	while(status_ != WAITING && status_ != ENDED) {
//		Thread::Yield(true);
//	}
}

/********************************************************************************/

void Coroutine::Start() {
	safe_assert(yield_point_ == NULL);
	safe_assert(BETWEEN(PASSIVE, status_, TERMINATED));

	// reset yield point to null
	yield_point_ = NULL;
	vc_clear(vc_);
	exception_ = NULL;

	//---------------
	CHANNEL_BEGIN_ATOMIC();

	if(status_ == PASSIVE || status_ == TERMINATED) {
		VLOG(2) << CO_TITLE << "Starting new thread";
		Thread::Start();
	} else if(status_ == WAITING || status_ == ENDED) {
		// send the restart message
		VLOG(2) << CO_TITLE << "Sending restart message";
		channel_.SendNoWait(MSG_RESTART);
	} else {
		// kill the thread and restart
		VLOG(2) << CO_TITLE << "Cancelling and restarting the thread";
		this->Finish();
		Thread::Start();
	}

	// check if this is the main (for now only main can start coroutines)
	safe_assert(Coroutine::Current() == group_->main());

	// wait for started message
	VLOG(2) << CO_TITLE << "Waiting the thread to start";
	MessageType msg = channel_.WaitReceive();
	CHECK(msg == MSG_STARTED) << "Expected a started message from " << this->name();
	VLOG(2) << CO_TITLE << "Got start signal from the thread";

	// first signal
	sem_end_.Signal();

	CHANNEL_END_ATOMIC();
	//---------------

	// if conc == true, then send a non-waiting transfer message to run the new coroutine concurrently
	if(transfer_on_start_) {
		safe_assert(status_ == WAITING);
		channel_.SendNoWait(MSG_TRANSFER);
	}
}

/********************************************************************************/

void* Coroutine::Run() {
	safe_assert(!this->IsMain());
	void* return_value = NULL;
	CHECK(status_ == PASSIVE) << "Wrong status " << status_ << ", expected " << PASSIVE;

	for(;true;) {
		try {
			safe_assert(PASSIVE <= status_ && status_ < TERMINATED);
			status_ = ENABLED;

			return_value = NULL;

			CoroutineGroup* group = CHECK_NOTNULL(group_);

			Scenario* scenario = CHECK_NOTNULL(group->scenario());

			// notify main about out startup
			VLOG(2) << CO_TITLE << "Sending started message and waiting for a transfer.";
			Transfer(&channel_, MSG_STARTED);

			VLOG(2) << CO_TITLE << "First transfer";

			try {
				// run the actual function
				return_value = call_function();

			} catch(std::exception* e) {
				VLOG(2) << CO_TITLE << " threw an exception...";
				// record the exception in scenario
				exception_ = e;
				if(ConcurritExecutionMode == COOPERATIVE) {
					// send main exception message
					this->Transfer(group->main(), MSG_EXCEPTION);
				}
			}

			VLOG(2) << CO_TITLE << " is ending...";

			status_ = ENDED;

			//---------------
			CHANNEL_BEGIN_ATOMIC();

			// second signal
			sem_end_.Signal();

			// last yield
			if(ConcurritExecutionMode == COOPERATIVE) {
				yield(ENDING_LABEL);
			} else {
				MessageType msg = channel_.WaitReceive();
				HandleMessage(msg);
			}

			CHANNEL_END_ATOMIC();
			//---------------

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
//	safe_assert(group->current() == this);

	// update current to target
//	group->set_current(target);

	safe_assert(target->status() == WAITING || target->status() == ENDED);
	Transfer(target->channel(), msg);
}

/********************************************************************************/

void Coroutine::Transfer(Channel<MessageType>* channel, MessageType msg /*=MSG_TRANSFER*/) {
	safe_assert(BETWEEN(ENABLED, status_, ENDED));

	if(status_ < ENDED) {
		status_ = WAITING;
	}

	// send-receive main
	msg = channel_.SendWaitReceive(channel, msg);
	HandleMessage(msg);

	if(status_ < ENDED) {
		status_ = ENABLED;
	}
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
		std::exception* e = NULL;
		// notify scenario about exception
		CoroutinePtrSet* members = group_->member_set();
		for(CoroutinePtrSet::iterator itr = members->begin(); itr != members->end(); ++itr) {
			Coroutine* co = *itr;
			e = co->exception_;
			if(e != NULL) {
				break;
			}
		}
		CHECK(e != NULL) << "MSG_EXCEPTION is received but no exception is found!";
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

