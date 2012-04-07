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

Coroutine* CoroutineGroup::main_ = NULL;

/********************************************************************************/

void CoroutineGroup::init_main() {
	safe_assert(main_ == NULL);
	main_ = new Coroutine(MAIN_TID, NULL, NULL, 0);
	main_->StartMain();
}

/********************************************************************************/

void CoroutineGroup::delete_main() {
	safe_assert(main_ != NULL);
	main_->FinishMain();
	delete main_;
	main_ = NULL;
}

/********************************************************************************/

/*
 * CoroutineGroup
 */

CoroutineGroup:: CoroutineGroup() {
	scenario_ = NULL;
	next_tid_ = 1;
	member_tidseq_.clear();
	next_idx_ = 0;
}

/********************************************************************************/

void CoroutineGroup::AddMember(Coroutine* member) {
	// check if not our member
	CHECK(!HasMember(member));
	safe_assert(!member->IsMain()); // cannot add main in here

	// set thread id if not set yet
	THREADID tid = member->tid();
	if(tid < 0) {
		tid = next_tid_;
		member->set_tid(tid);
	}
	next_tid_ = tid + 1;

	// once determined the tid, then insert it as new member
	members_[tid] = member;
	member->set_group(this);

	// put in the creation order
	safe_assert(next_idx_ == member_tidseq_.size());
	member_tidseq_.push_back(tid);
	++next_idx_;
}

/********************************************************************************/

Coroutine* CoroutineGroup::GetNextCreatedMember(THREADID tid /*= -1*/) {
	Coroutine* member = NULL;
	safe_assert(BETWEEN(0, next_idx_, member_tidseq_.size()));
	if(next_idx_ < member_tidseq_.size()) {
		// expecting recreation
		member = GetNthCreatedMember(next_idx_);
		++next_idx_;
	} else {
		safe_assert(next_idx_ == member_tidseq_.size());
	}
	safe_assert(member == NULL || !member->IsMain()); // cannot add main in here
	return member;
}

/********************************************************************************/

Coroutine* CoroutineGroup::GetNthCreatedMember(int i, THREADID tid /*= -1*/) {
	// this does not change next_idx_
	safe_assert(BETWEEN2(0, i, next_idx_, member_tidseq_.size()));
	if(i >= member_tidseq_.size()) {
		return NULL;
	}
	THREADID exp_tid = member_tidseq_[i];
	CHECK(tid == -1 || tid == exp_tid) << "Given tid does not match the expected one!";
	if(tid == -1) {
		tid = exp_tid;
	}
	Coroutine* member = GetMember(tid);
	CHECK(member != NULL && member->tid() == tid && member->status() > PASSIVE) << "Given member does not match the expected one!";
	return member;
}

/********************************************************************************/

void CoroutineGroup::TakeOutMember(Coroutine* member) {
	// check if already our member
	CHECK(HasMember(member));
	safe_assert(!member->IsMain()); // cannot add main in here
	MembersMap::iterator itr = members_.find(member->tid());
	safe_assert(itr != members_.end());
	members_.erase(itr);
}

void CoroutineGroup::PutBackMember(Coroutine* member) {
	// check if not our member
	CHECK(!HasMember(member));
	safe_assert(!member->IsMain()); // cannot add main in here
	members_[member->tid()] = member;
}

/********************************************************************************/

void CoroutineGroup::Finish() {
	// no need to finish main, it is always running
	for_each_member(co) {
		if(co->status() > PASSIVE) {
			co->Finish();
		}
		safe_assert(co->status() == TERMINATED);
	}
}


/********************************************************************************/

void CoroutineGroup::WaitForAllEnd() {
	for_each_member(co) {
		if(co->status() > PASSIVE) {
			co->WaitForEnd();
		}
	}

	safe_assert(IsAllEnded());
}

/********************************************************************************/

void CoroutineGroup::Restart() {
//	// no need to restart main, it is always running
//	for_each_member(co) {
//		if(co->status() > PASSIVE) { // may also be ended, but it is actually waiting for a signal
//			co->Restart();
//		}
//		safe_assert(co->yield_point() == NULL);
//	}

	// initialize main coroutine, (main is not added to the list of members)
	safe_assert(main_ != NULL);
	// restart main
	main_->set_yield_point(NULL);
	main_->set_group(this);

	next_idx_ = 0;
}

/********************************************************************************/

Coroutine* CoroutineGroup::GetNextEnabled(CoroutinePtrSet* except_targets /*= NULL*/, CoroutinePtrSet* only_targets /*= NULL*/) {
	safe_assert(scenario_ != NULL);
	safe_assert(NULL_OR_NONEMPTY(except_targets));
	safe_assert(NULL_OR_NONEMPTY(only_targets));
	CoroutinePtrSet* targets = only_targets != NULL ? only_targets : GetMemberSet();
	for (CoroutinePtrSet::iterator itr = targets->begin() ; itr != targets->end(); ++itr) {
		Coroutine* co = *itr;
		StatusType status = co->status();
		if(ENABLED <= status && status < BLOCKED) {
			if(except_targets == NULL || except_targets->find(co) == except_targets->end()) {
				return co;
			}
		}
	}
	return NULL;
}

/********************************************************************************/

CoroutinePtrSet CoroutineGroup::GetEnabledSet() {
	CoroutinePtrSet enabled;
	for_each_member(co) {
		StatusType status = co->status();
		if(ENABLED <= status && status < BLOCKED) {
			enabled.insert(co);
		}
	}
	return enabled;
}

/********************************************************************************/

int CoroutineGroup::GetNumMembers() {
	return members_.size();
}

/********************************************************************************/

CoroutinePtrSet* CoroutineGroup::GetMemberSet(CoroutinePtrSet* set /*= NULL*/) {
	CoroutinePtrSet* m = (set != NULL) ? set : new CoroutinePtrSet();
	for_each_member(co) {
		m->insert(co);
	}
	return m;
}

/********************************************************************************/

bool CoroutineGroup::IsAllEnded() {
	for_each_member(co) {
		if (!co->is_ended()) {
			return false;
		}
	}
	return true;
}

/********************************************************************************/

//bool CoroutineGroup::CheckCurrent(Coroutine* current) {
//	if(ConcurritExecutionMode == SINGLE_RUNNER) {
//		return current_ == current;
//	}
//	return true;
//}

/********************************************************************************/

bool CoroutineGroup::HasMember(THREADID tid) {
	return NULL != GetMember(tid);
}

bool CoroutineGroup::HasMember(Coroutine* member) {
	Coroutine* co = GetMember(member->tid());
	if(co == NULL) {
		return false;
	}
	CHECK(co->group() == this) << "The member " << co->tid() << "'s group is not this group!";
	return true;
}

/********************************************************************************/

Coroutine* CoroutineGroup::GetMember(THREADID tid) {
	if(tid == MAIN_TID) {
		safe_assert(main_ != NULL);
		return main_;
	}
	MembersMap::iterator itr = members_.find(tid);
	if(itr == members_.end()) {
		return NULL;
	} else {
		Coroutine* co = itr->second;
		safe_assert(co != NULL);
		safe_assert(co->group() != NULL);
		return co;
	}
}

/********************************************************************************/

Coroutine* CoroutineGroup::GetMember(const pthread_t& pid) {
	for_each_member(co) {
		if(co->pthread() == pid) {
			return co;
		}
	}
	return NULL;
}

/********************************************************************************/

} // end namespace
