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

/*************************************************************************************/

// extern'ed variables
TrueTransitionPredicate __true_transition_predicate__;
FalseTransitionPredicate __false_transition_predicate__;

TransitionPredicate* TransitionPredicate::True() { return &__true_transition_predicate__; }
TransitionPredicate* TransitionPredicate::False() { return &__false_transition_predicate__; }

/*************************************************************************************/

TransitionPredicate* TransitionPredicate::operator!() {
	return new NotTransitionPredicate(this);
}

TransitionPredicate* TransitionPredicate::operator &&(const TransitionPredicate& pred) {
	return new NAryTransitionPredicate(NAryAND, this, const_cast<TransitionPredicate*>(&pred));
}

TransitionPredicate* TransitionPredicate::operator ||(const TransitionPredicate& pred) {
	return new NAryTransitionPredicate(NAryOR, this, const_cast<TransitionPredicate*>(&pred));
}

TransitionPredicate* TransitionPredicate::operator && (const bool& b) {
	return (this->operator&& (b ? TransitionPredicate::True() : TransitionPredicate::False()));
}

TransitionPredicate* TransitionPredicate::operator || (const bool& b) {
	return (this->operator|| (b ? TransitionPredicate::True() : TransitionPredicate::False()));
}

/********************************************************************************/

#define for_each_transinfo(info, tinfos) \
	TransitionInfoList* __list__ = tinfos; \
	TransitionInfoList::iterator __itr__ = __list__->begin(); \
	TransitionInfo* info = (__itr__ != __list__->end() ? &(*__itr__) : NULL); \
	for (; __itr__ != __list__->end(); info = ((++__itr__) != __list__->end() ? &(*__itr__) : NULL))

/********************************************************************************/

//class TPBeforeEnds : public PreStateTransitionPredicate {
//public:
//	TPBeforeEnds(void* addr = NULL) : PreStateTransitionPredicate(), addr_(addr) {}
//	~TPBeforeEnds() {}
//
//	bool EvalState(Coroutine* t = NULL) {
//		for_each_transinfo(info, t->trinfolist()) {
//			EndingTransitionInfo* einfo = ASINSTANCEOF(info, EndingTransitionInfo*);
//			if(einfo != NULL) {
//				return true;
//			}
//		}
//		return false;
//	}
//
//private:
//	DECL_FIELD(void*, addr)
//};
//
///********************************************************************************/
//
//class TPBeforeReads : public PreStateTransitionPredicate {
//public:
//	TPBeforeReads(void* addr = NULL) : PreStateTransitionPredicate(), addr_(addr) {}
//	~TPBeforeReads() {}
//
//	bool EvalState(Coroutine* t = NULL) {
//		for_each_transinfo(info, t->trinfolist()) {
//			MemAccessTransitionInfo* minfo = ASINSTANCEOF(info, MemAccessTransitionInfo*);
//			if(minfo != NULL && minfo->type() == TRANS_MEM_READ && (addr_ == NULL || minfo->addr() == addr_)) {
//				return true;
//			}
//		}
//		return false;
//
//	}
//
//private:
//	DECL_FIELD(void*, addr)
//};

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/
/********************************************************************************/


} // end namespace


