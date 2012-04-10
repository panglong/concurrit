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


boost::shared_ptr<AuxVar0<bool, false>> AuxState::Ends;

boost::shared_ptr<AuxVar1<ADDRINT, uint32_t, -1, false>> AuxState::Reads;
boost::shared_ptr<AuxVar1<ADDRINT, uint32_t, -1, false>> AuxState::Writes;

//boost::shared_ptr<AuxVar1<ADDRINT, bool, -1, false>> AuxState::CallsFrom;
//boost::shared_ptr<AuxVar1<ADDRINT, bool, -1, false>> AuxState::CallsTo;

boost::shared_ptr<AuxVar1<ADDRINT, bool, -1, false>> AuxState::Enters;
boost::shared_ptr<AuxVar1<ADDRINT, bool, -1, false>> AuxState::Returns;

boost::shared_ptr<AuxVar1<ADDRINT, int, -1, 0>> AuxState::InFunc;
boost::shared_ptr<AuxVar1<ADDRINT, int, -1, 0>> AuxState::NumInFunc;

boost::shared_ptr<AuxVar0<int, -1>> AuxState::Pc;

/*************************************************************************************/

// extern'ed variables
static TransitionPredicatePtr __true_transition_predicate__(new TrueTransitionPredicate());
static TransitionPredicatePtr __false_transition_predicate__(new FalseTransitionPredicate());

TransitionPredicatePtr TransitionPredicate::True() { return __true_transition_predicate__; }
TransitionPredicatePtr TransitionPredicate::False() { return __false_transition_predicate__; }

/*************************************************************************************/

TransitionPredicatePtr operator!(const TransitionPredicatePtr& pred) {
	TransitionPredicatePtr p(new NotTransitionPredicate(pred));
	return p;
}

/********************************************************************************/

TransitionPredicatePtr operator &&(const TransitionPredicatePtr& pred1, const TransitionPredicatePtr& pred2) {
	TransitionPredicatePtr p(new NAryTransitionPredicate(NAryAND, pred1, pred2));
	return p;
}

TransitionPredicatePtr operator ||(const TransitionPredicatePtr& pred1, const TransitionPredicatePtr& pred2) {
	TransitionPredicatePtr p(new NAryTransitionPredicate(NAryOR, pred1, pred2));
	return p;
}

/********************************************************************************/

TransitionPredicatePtr operator && (const TransitionPredicatePtr& pred, const bool& b) {
	TransitionPredicatePtr p(new NAryTransitionPredicate(NAryAND, pred, (b ? TransitionPredicate::True() : TransitionPredicate::False())));
	return p;
}

TransitionPredicatePtr operator || (const TransitionPredicatePtr& pred, const bool& b) {
	TransitionPredicatePtr p(new NAryTransitionPredicate(NAryOR, pred, (b ? TransitionPredicate::True() : TransitionPredicate::False())));
	return p;
}

/********************************************************************************/

class TPThreadVarsEqual : public PreStateTransitionPredicate {
public:
	TPThreadVarsEqual(const ThreadVarPtr& tvar1, const ThreadVarPtr& tvar2) : PreStateTransitionPredicate(), tvar1_(tvar1), tvar2_(tvar2) {}
	~TPThreadVarsEqual() {}

	bool EvalState(Coroutine* t = NULL) {
		Coroutine* co1 = safe_notnull(tvar1_->thread());
		Coroutine* co2 = safe_notnull(tvar2_->thread());
		return co1->tid() == co2->tid();
	}

	static TransitionPredicatePtr create(const ThreadVarPtr& tvar1, const ThreadVarPtr& tvar2) {
		TransitionPredicatePtr p(new TPThreadVarsEqual(tvar1, tvar2));
		return p;
	}

private:
	DECL_FIELD(ThreadVarPtr, tvar1)
	DECL_FIELD(ThreadVarPtr, tvar2)
};

/********************************************************************************/

TransitionPredicatePtr operator == (const ThreadVarPtr& t1, const ThreadVarPtr& t2) {
	return TPThreadVarsEqual::create(t1, t2);
}

TransitionPredicatePtr operator != (const ThreadVarPtr& t1, const ThreadVarPtr& t2) {
	return !(t1 == t2);
}

/********************************************************************************/

//#define for_each_transinfo(info, tinfos) \
//	TransitionInfoList* __list__ = tinfos; \
//	TransitionInfoList::iterator __itr__ = __list__->begin(); \
//	TransitionInfo* info = (__itr__ != __list__->end() ? &(*__itr__) : NULL); \
//	for (; __itr__ != __list__->end(); info = ((++__itr__) != __list__->end() ? &(*__itr__) : NULL))

/********************************************************************************/

//class TPEnds : public PreStateTransitionPredicate {
//public:
//	TPEnds(ThreadVarPtr var) : PreStateTransitionPredicate(), var_(var) {}
//	~TPEnds() {}
//
//	bool EvalState(Coroutine* t = NULL) {
//		safe_assert(t != NULL);
//
//		return AuxState::Ends.get(t->coid());
//	}
//
//private:
//	DECL_FIELD(ThreadVarPtr, var)
//};
//
///********************************************************************************/
//
//class TPReads : public PreStateTransitionPredicate {
//public:
//	TPReads(void* addr = NULL) : PreStateTransitionPredicate(), addr_(addr) {}
//	~TPReads() {}
//
//	bool EvalState(Coroutine* t = NULL) {
//		safe_assert(t != NULL);
//		return addr_ == NULL ?
//				AuxState::Reads.get(t->coid()) :
//				AuxState::Reads.get(t->coid(), PTR2ADDRINT(addr_));
//	}
//
//private:
//	DECL_FIELD(void*, addr)
//};
//
///********************************************************************************/
//
//class TPWrites: public PreStateTransitionPredicate {
//public:
//	TPWrites(void* addr = NULL) : PreStateTransitionPredicate(), addr_(addr) {}
//	~TPWrites() {}
//
//	bool EvalState(Coroutine* t = NULL) {
//		safe_assert(t != NULL);
//		return addr_ == NULL ?
//				AuxState::Writes.get(t->coid()) :
//				AuxState::Writes.get(t->coid(), PTR2ADDRINT(addr_));
//	}
//
//private:
//	DECL_FIELD(void*, addr)
//};

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/


} // end namespace


