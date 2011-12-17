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

#include "counit.h"

namespace counit {

/********************************************************************************/

Coroutine* CoroutineGroup::main_ = NULL;

/********************************************************************************/

void CoroutineGroup::init_main() {
	safe_assert(main_ == NULL);
	main_ = new Coroutine(MAIN_NAME, NULL, NULL, 0);
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
	// initialize main coroutine, (main is not added to the list of members)
	safe_assert(main_ != NULL);
	current_ = main_;
	main_->set_group(this);
	scenario_ = NULL;
	next_coid_ = 1;
}

/********************************************************************************/

CoroutineGroup:: CoroutineGroup(Scenario* scenario) {
	// initialize main coroutine, (main is not added to the list of members)
	safe_assert(main_ != NULL);
	current_ = main_;
	main_->set_group(this);
	scenario_ = scenario;
	next_coid_ = 1;
}

/********************************************************************************/

void CoroutineGroup::AddMember(Coroutine* member) {
	// check if not our member
	CHECK(!HasMember(member));
	safe_assert(member->name().length() > 0);
	safe_assert(!member->IsMain()); // cannot add main in here
	members_[member->name()] = member;
	member->set_group(this);
	member->set_coid(next_coid_);
	++next_coid_;
}

void CoroutineGroup::TakeOutMember(Coroutine* member) {
	// check if already our member
	CHECK(HasMember(member));
	safe_assert(member->name().length() > 0);
	safe_assert(!member->IsMain()); // cannot add main in here
	MembersMap::iterator itr = members_.find(member->name());
	safe_assert(itr != members_.end());
	members_.erase(itr);
}

void CoroutineGroup::PutBackMember(Coroutine* member) {
	// check if not our member
	CHECK(!HasMember(member));
	safe_assert(member->name().length() > 0);
	safe_assert(!member->IsMain()); // cannot add main in here
	members_[member->name()] = member;
}

/********************************************************************************/

void CoroutineGroup::Finish() {
	// no need to finish main, it is always running
	for_each_member(co) {
		if(co->status() > PASSIVE) {
			co->Finish();
		}
	}
}

/********************************************************************************/

void CoroutineGroup::Restart() {
	// no need to restart main, it is always running
	for_each_member(co) {
		if(co->status() > PASSIVE) { // may also be ended, but it is actually waiting for a signal
			co->Restart();
		}
		safe_assert(co->yield_point() == NULL);
	}
	// restart main
	main_->set_yield_point(NULL);
	main_->set_group(this);
}

/********************************************************************************/

Coroutine* CoroutineGroup::GetNextEnabled(CoroutinePtrSet* except_targets /*= NULL*/, CoroutinePtrSet* only_targets /*= NULL*/) {
	safe_assert(scenario_ != NULL);
	safe_assert(NULL_OR_NONEMPTY(except_targets));
	safe_assert(NULL_OR_NONEMPTY(only_targets));
	CoroutinePtrSet* targets = only_targets != NULL ? only_targets : member_set();
	for (CoroutinePtrSet::iterator itr = targets->begin() ; itr != targets->end(); ++itr) {
		Coroutine* co = *itr;
		if(co->status() == ENABLED) {
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
		if(co->status() == ENABLED) {
			enabled.insert(co);
		}
	}
	return enabled;
}

/********************************************************************************/

CoroutinePtrSet CoroutineGroup::GetMemberSet() {
	CoroutinePtrSet members;
	for_each_member(co) {
		members.insert(co);
	}
	return members;
}

/********************************************************************************/

CoroutinePtrSet* CoroutineGroup::member_set() {
	CoroutinePtrSet* m = new CoroutinePtrSet();
	for_each_member(co) {
		m->insert(co);
	}
	return m;
}

/********************************************************************************/

bool CoroutineGroup::IsAllEnded() {
	for_each_member(co) {
		if (co->status() < ENDED) {
			return false;
		}
	}
	return true;
}

/********************************************************************************/

bool CoroutineGroup::HasMember(const char* name) {
	return NULL != GetMember(name);
}

bool CoroutineGroup::HasMember(std::string& name) {
	return NULL != GetMember(name);
}

bool CoroutineGroup::HasMember(Coroutine* member) {
	Coroutine* co = GetMember(member->name());
	if(co == NULL) {
		return false;
	}
	CHECK(co->group() == this);
	return true;
}

/********************************************************************************/

Coroutine* CoroutineGroup::GetMember(const char* name) {
	std::string s(name);
	return GetMember(s);
}

Coroutine* CoroutineGroup::GetMember(std::string& name) {
	if(name == MAIN_NAME) {
		safe_assert(main_ != NULL);
		return main_;
	}
	std::map<std::string, Coroutine*>::iterator itr = members_.find(name);
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


/********************************************************************************/

} // end namespace
