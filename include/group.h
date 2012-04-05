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

#ifndef COROUTINEGROUP_H_
#define COROUTINEGROUP_H_

#include "common.h"

namespace concurrit {

class Scenario;

typedef std::map<THREADID, Coroutine*> MembersMap;

/*
 * represents a set of coroutines
 */

class CoroutineGroup {

public:

	CoroutineGroup();
	~CoroutineGroup() {}

	void AddMember(Coroutine* member);

	// these are for already added members
	void TakeOutMember(Coroutine* member);
	void PutBackMember(Coroutine* member);

	void Restart();
	void Finish();
	void WaitForAllEnd();

	bool HasMember(Coroutine* member);
	bool HasMember(THREADID tid);
	Coroutine* GetMember(THREADID tid);

	/*
	 * choose a next enabled thread (chosen randomly)
	 * returns NULL is no one is enabled: deadlock
	 * if except_targets is non-null, then they are not chosen
	 * if only_targets is non-null, then only they are chosen
	 */
	Coroutine* GetNextEnabled(CoroutinePtrSet* except_targets = NULL, CoroutinePtrSet* only_targets = NULL);

	CoroutinePtrSet GetEnabledSet();
	CoroutinePtrSet GetMemberSet();

	int GetNumMembers();

	/*
	 * return if all of the members have ended
	 */
	bool IsAllEnded();

//	bool CheckCurrent(Coroutine* current);

protected:
	static void init_main();
	static void delete_main();

private:
	DECL_FIELD(Scenario*, scenario)
	//DECL_FIELD(Coroutine*, current)
	DECL_STATIC_FIELD(Coroutine*, main)
//	DECL_FIELD(std::exception*, exception)
	DECL_FIELD(THREADID, next_tid)

	DECL_FIELD_REF(MembersMap, members)
	CoroutinePtrSet* member_set();

	friend class Concurrit;
	friend class Scenario;
	friend class Coroutine;
};

#define for_each_member(co) \
	MembersMap::iterator __itr__ = members_.begin(); \
	Coroutine* co = (__itr__ != members_.end() ? __itr__->second : NULL); \
	for (; __itr__ != members_.end(); co = ((++__itr__) != members_.end() ? __itr__->second : NULL))


/********************************************************************************/


// represents a set of coroutines to restrict the group to the rest of those
class WithoutGroup {
public:
	WithoutGroup(CoroutineGroup* group, CoroutinePtrSet set) : group_(group), set_(set) {
		for_each_coroutine(set_, co) {
			// remove from the group
			group_->TakeOutMember(co);
		}
	}

	~WithoutGroup() {
		for_each_coroutine(set_, co) {
			// add back to the group
			group_->PutBackMember(co);
		}
	}
private:
	DECL_FIELD(CoroutineGroup*, group)
	DECL_FIELD_REF(CoroutinePtrSet, set)
};

/********************************************************************************/

// represents a set of coroutines to restrict the group to only those
class WithGroup {
public:
	WithGroup(CoroutineGroup* group, CoroutinePtrSet set) {
		CoroutinePtrSet others = group->GetMemberSet();
		for_each_coroutine(set, co) {
			CoroutinePtrSet::iterator iter = others.find(co);
			if(iter != others.end()) {
				others.erase(iter);
			}
		}
		without_ = new WithoutGroup(group, others); // takes out others
	}

	~WithGroup() {
		delete without_; // puts back others
	}

private:
	DECL_FIELD_REF(WithoutGroup*, without)
};

/********************************************************************************/

} // end namespace

#endif /* COROUTINEGROUP_H_ */
