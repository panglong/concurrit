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

Coroutine* Coroutine::main_ = NULL;

Coroutine::Coroutine(THREADID tid, ThreadEntryFunction entry_function, void* entry_arg, int stack_size)
: Thread(tid, entry_function, entry_arg, stack_size)
{
	status_ = PASSIVE;
	group_ = NULL;
//	yield_point_ = NULL;
//	vc_clear(vc_);
	exception_ = NULL;
//	current_node_ = NULL;
//	is_driver_thread_ = false;

	srcloc_ = NULL;
	instr_callback_info_[0] = '\0';

	ThreadVarPtr p(new StaticThreadVar(this, "Self-ThreadVar"));
	tvar_ = p;
}

/********************************************************************************/

Coroutine::~Coroutine() {
	// noop
}

/********************************************************************************/

void Coroutine::StartMain() {
	safe_assert(main_ == NULL);
	Coroutine* co = new Coroutine(MAIN_TID, NULL, NULL, 0);

	co->status_ = ENABLED;

	pthread_t pth_self = pthread_self();
	safe_assert(pth_self != PTH_INVALID_THREAD);
	// for non-main coroutines, this is called in ThreadEntry
	co->attach_pthread(pth_self);

	main_ = co;
}

void Coroutine::FinishMain() {
	Coroutine* co = main_;
	safe_assert(co != NULL);

	safe_assert(co->tid_ == MAIN_TID);
	co->status_ = TERMINATED;
	co->tid_ = -1;

	pthread_t pth_self = pthread_self();
	safe_assert(pth_self != PTH_INVALID_THREAD);
	// for non-main coroutines, this is called in ThreadEntry
	co->detach_pthread(pth_self);

	delete co;
	main_ = NULL;
}

bool Coroutine::IsMain() {
	if(tid_ == MAIN_TID) {
		safe_assert(group_ == NULL || main_ == this);
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
			MYLOG(2) << CO_TITLE << "Sending finish signal";
			this->channel_.SendNoWait(MSG_TERMINATE);
			this->Join();
			MYLOG(2) << CO_TITLE << "Joined the thread";
		} else {
			fprintf(stderr, "Killing thread %d...", tid_);
			MYLOG(2) << CO_TITLE << "Cancelling the thread";
			this->Cancel(); // this->Kill(SIGUSR1);
			MYLOG(2) << CO_TITLE << "Waiting for the thread";
			this->Join();
			fprintf(stderr, "Done.\n");
		}
	}

	status_ = TERMINATED;
}

/********************************************************************************/

// returns true if waiting ends successfully before time-out
bool Coroutine::WaitForEnd(long timeout /*= -1*/) {
	if(timeout == -1) timeout = (Config::RunUncontrolled ? 0 : Config::MaxWaitTimeUSecs);

	safe_assert(BETWEEN(ENABLED, status_, ENDED));
	MYLOG(2) << "Waiting for coroutine " << tid_ << " to end.";

	// wait for the semaphore twice
	sem_end_.Wait(); // taken first token

	if(timeout <= 0) {
		sem_end_.Wait(); // taken second token
	} else {
		int result = sem_end_.WaitTimed(timeout); // take second token
		if(result == ETIMEDOUT) {
			MYLOG(2) << "Waiting coroutine " << tid_ << " has timed out.";
			sem_end_.Signal(); // put back the token taken above
			return false;
		}
		safe_assert(result == PTH_SUCCESS);
	}

	// then give back both signals
 	sem_end_.Signal(2);

	MYLOG(2) << "Detected coroutine " << tid_ << " has ended.";
	safe_assert(status_ == ENDED);

	return true;
}

/********************************************************************************/

void Coroutine::Start(pthread_t* pid /*= NULL*/, const pthread_attr_t* attr /*= NULL*/) {
//	safe_assert(yield_point_ == NULL);
	safe_assert(BETWEEN(PASSIVE, status_, TERMINATED));

	// reset yield point to null
//	yield_point_ = NULL;
//	vc_clear(vc_);
	exception_ = NULL;
//	current_node_ = NULL;
//	trinfolist_.clear();

	//---------------
	CHANNEL_BEGIN_ATOMIC();

	if(status_ == PASSIVE || status_ == TERMINATED) {
		MYLOG(2) << CO_TITLE << "Starting new thread";
		Thread::Start(pid, attr);
	} else if(status_ == WAITING || status_ == ENDED) {
		return_value_ = NULL;
		if(pid != NULL) *pid = pthread_;
		// send the restart message
		MYLOG(2) << CO_TITLE << "Sending restart message";
		channel_.SendNoWait(MSG_RESTART);
	} else {
		// kill the thread and restart
		unreachable();
//		MYLOG(2) << CO_TITLE << "Cancelling and restarting the thread";
//		this->Finish();
//		Thread::Start(pid, attr);
	}

	// wait for started message
	MYLOG(2) << CO_TITLE << "Waiting the thread to start";
	MessageType msg = channel_.WaitReceive();
	CHECK(msg == MSG_STARTED) << "Expected a started message from " << this->tid();
	MYLOG(2) << CO_TITLE << "Got start signal from the thread";

	// first signal
	sem_end_.Set(0);
	sem_end_.Signal();

	CHANNEL_END_ATOMIC();
	//---------------

	// if conc == true, then send a non-waiting transfer message to run the new coroutine concurrently
	safe_assert(status_ == WAITING);
	channel_.SendNoWait(MSG_TRANSFER);

	__pthread_errno__ = PTH_SUCCESS;
}

/********************************************************************************/

void Coroutine::SetStarted() {
	safe_assert(PASSIVE <= status_ && status_ < TERMINATED);
	status_ = ENABLED;

	// notify main about our startup
	MYLOG(2) << CO_TITLE << "Sending started message and waiting for a transfer.";
	Transfer(&channel_, MSG_STARTED);

	MYLOG(2) << CO_TITLE << "First transfer";

	// notifies pintool about the restart,
	// to reset pin related data structures
	ThreadRestart();
}

/********************************************************************************/

void Coroutine::SetEnded() {
	CHANNEL_BEGIN_ATOMIC();

	MYLOG(2) << CO_TITLE << " is ending...";
	status_ = ENDED;

	// second signal
	sem_end_.Signal();

	// last yield
	MessageType msg = channel_.WaitReceive();
	HandleMessage(msg);

	CHANNEL_END_ATOMIC();
}

/********************************************************************************/

void* Coroutine::Run() {
	safe_assert(!this->IsMain());
	void* return_value = NULL;
	CHECK(status_ == PASSIVE) << "Wrong status " << status_ << ", expected " << PASSIVE;

	Scenario* scenario = Scenario::NotNullCurrent();

	for(;true;) {
		try {
			SetStarted();

			return_value = NULL;

			try {
				// run the actual function
				return_value = call_function();

				//---------------
				if(PinMonitor::IsEnabled())
					PinMonitor::ThreadEnd(this, scenario);

			} catch(std::exception* e) {
				// first check if this is due to pthread_exit
				BacktrackException* be = ASINSTANCEOF(e, BacktrackException*);
				if(be != NULL && be->reason() == PTH_EXIT) {
					MYLOG(2) << CO_TITLE << "Simulating exit due to pthread_exit.";
					return_value = return_value_;
				} else { //==================================================
					// other kinds of errors
					MYLOG(2) << CO_TITLE << " threw an exception...";
					// record the exception in scenario
					safe_assert(!INSTANCEOF(e, ConcurritException*));
					exception_ = e;

					// (immediatelly) notify all others that there is an exception
					MYLOG(1) << "Coroutine threw exception, calling EndWithException...";
					scenario->exec_tree()->EndWithException(this, exception_);
				}
			}

			SetEnded();

		} catch(MessageType& m) {
			// only terminate and restart messages are thrown
			if(m == MSG_TERMINATE) {
				MYLOG(2) << CO_TITLE << "terminating";
				break; // terminate
			} else if(m == MSG_RESTART) {
				MYLOG(2) << CO_TITLE << "restarting";
				continue;
			} else {
				bool invalid_message_type = false;
				safe_assert(invalid_message_type);
			}
		}
	} // end loop

	//------------------------------------------------------------
	// call driver __fini__ if this is the driver thread
//	if(is_driver_thread_) {
//		MYLOG(1) << "Calling driver __fini__ function.";
//		MainFuncType fini_func = Concurrit::driver_fini();
//		if(fini_func != NULL) {
//			main_args* args = Concurrit::driver_args();
//			fini_func(args->argc_, args->argv_);
//			Concurrit::set_driver_fini(NULL); // since we are calling it once
//		}
//	}

	//------------------------------------------------------------
	status_ = TERMINATED;

	return return_value;
}

/********************************************************************************/

void Coroutine::Transfer(Coroutine* target, MessageType msg /*=MSG_TRANSFER*/) {
	safe_assert(target != NULL && target != this);

	MYLOG(2) << CO_TITLE << "Transferring to " << target->tid();

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
	MYLOG(2) << CO_TITLE << "Handling message:" << msg;

	if(msg == MSG_TRANSFER) {
//		BeginStrand(name_.c_str()); // this is to notify the PIN tool
	} else if(msg == MSG_RESTART || msg == MSG_TERMINATE) {
		throw msg; // let the Run() method catch it.
	} else if(msg == MSG_EXCEPTION) {
		safe_assert(this->IsMain());
		std::exception* e = NULL;
		// notify scenario about exception
		CoroutinePtrSet* members = group_->GetMemberSet();
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

//SchedulePoint* Coroutine::OnYield(Coroutine* target, std::string& label, SourceLocation* loc /*=NULL*/, SharedAccess* access /*=NULL*/) {
//	safe_assert(IsMain() || label != MAIN_LABEL);
//	// record the yield point as our last yield point
//	// if the label is the same, update the same yield point
//	SchedulePoint* point = this->yield_point_;
//	if(point != NULL) {
//		// point is not null
//		safe_assert(point->IsResolved());
//		safe_assert(point->AsYield()->source() == this);
//		size_t count = point->AsYield()->count();
//		safe_assert(count > 0);
//		if(!point->IsTransfer() && point->AsYield()->label() == label) {
//			point->AsYield()->set_count(count + 1);
//			// update access and location
//			point->AsYield()->update_access_loc(access, loc);
//			return point;
//		}
//	}
//	// create a new point
//	if(target == NULL) {
//		safe_assert(this->IsMain());
//		// we need a new yield point
//		MYLOG(2) << CO_TITLE << "Main::OnYield generating a new yield point with label " << label;
//		point = new YieldPoint(this, label, 1, loc, access, true /*free_target*/, true /*free_count*/);
//	} else if(target->IsMain()) {
//		safe_assert(!this->IsMain());
//		// we need a new yield point
//		MYLOG(2) << CO_TITLE << "Main::OnYield generating a new yield point with label " << label;
//		point = new YieldPoint(this, label, 1, loc, access, false /*free_target*/, true /*free_count*/);
//	} else {
//		MYLOG(2) << CO_TITLE << "OnYield generating a new transfer point with label " << label;
//		point = new TransferPoint(new YieldPoint(this, label, 1, loc, access, false /*free_target*/, true /*free_count*/), target);
//	}
//
//	safe_assert(point != NULL);
//	this->yield_point_ = point;
//	safe_assert(point->IsResolved());
//
//	return point;
//}

/********************************************************************************/

//AccessLocPair Coroutine::GetNextAccess() {
//	SchedulePoint* point = yield_point_;
//	if(point == NULL) {
//		return AccessLocPair(); // no access
//	}
//	return AccessLocPair(point->AsYield()->access(), point->AsYield()->loc());
//}

/********************************************************************************/

//void Coroutine::OnAccess(SharedAccess* access) {
//	group_->scenario()->OnAccess(this, access);
//}

/********************************************************************************/

void Coroutine::StartControlledTransition() {

	// make it blocked
	status_ = BLOCKED;
}

/********************************************************************************/

void Coroutine::FinishControlledTransition() {

	// remove auxiliary state
	AuxState::Reset(tid_);

	srcloc_ = NULL;

	instr_callback_info_[0] = '\0';

//	current_node_ = NULL;

	// make it enabled
	status_ = ENABLED;
}

/********************************************************************************/


} // end namespace

