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

boost::shared_ptr<AuxVar1<ADDRINT, uint32_t, 0, 0>> AuxState::Reads;
boost::shared_ptr<AuxVar1<ADDRINT, uint32_t, 0, 0>> AuxState::Writes;

boost::shared_ptr<AuxVar1<ADDRINT, bool, 0, false>> AuxState::CallsFrom;
boost::shared_ptr<AuxVar1<ADDRINT, bool, 0, false>> AuxState::CallsTo;

boost::shared_ptr<AuxVar1<ADDRINT, bool, 0, false>> AuxState::Enters;
boost::shared_ptr<AuxVar1<ADDRINT, bool, 0, false>> AuxState::Returns;

boost::shared_ptr<AuxVar1<ADDRINT, int, 0, 0>> AuxState::InFunc;
boost::shared_ptr<AuxVar1<ADDRINT, int, 0, 0>> AuxState::NumInFunc;

boost::shared_ptr<AuxVar1<ADDRINT, ADDRINT, 0, 0>> AuxState::Arg0;
boost::shared_ptr<AuxVar1<ADDRINT, ADDRINT, 0, 0>> AuxState::Arg1;

boost::shared_ptr<AuxVar1<ADDRINT, ADDRINT, 0, 0>> AuxState::RetVal;

boost::shared_ptr<AuxVar0<int, -1>> AuxState::Pc;
boost::shared_ptr<AuxVar0<bool, false>> AuxState::AtPc;

ThreadVarPtr AuxState::Tid;

/*************************************************************************************/

void AuxState::Init() {
	boost::shared_ptr<AuxVar0<bool, false>> _ends(new StaticAuxVar0<bool, false>("Ends"));
	AuxState::Ends = _ends;

	boost::shared_ptr<AuxVar1<ADDRINT, uint32_t, 0, 0>> _reads(new StaticAuxVar1<ADDRINT, uint32_t, 0, false>("Reads"));
	AuxState::Reads = _reads;

	boost::shared_ptr<AuxVar1<ADDRINT, uint32_t, 0, 0>> _writes(new StaticAuxVar1<ADDRINT, uint32_t, 0, false>("Writes"));
	AuxState::Writes = _writes;

	boost::shared_ptr<AuxVar1<ADDRINT, bool, 0, false>> _callsfrom(new StaticAuxVar1<ADDRINT, bool, 0, false>("CallsFrom"));
	AuxState::CallsFrom = _callsfrom;

	boost::shared_ptr<AuxVar1<ADDRINT, bool, 0, false>> _callsto(new StaticAuxVar1<ADDRINT, bool, 0, false>("CallsTo"));
	AuxState::CallsTo = _callsto;

	boost::shared_ptr<AuxVar1<ADDRINT, bool, 0, false>> _enters(new StaticAuxVar1<ADDRINT, bool, 0, false>("Enters"));
	AuxState::Enters = _enters;

	boost::shared_ptr<AuxVar1<ADDRINT, bool, 0, false>> _returns(new StaticAuxVar1<ADDRINT, bool, 0, false>("Returns"));
	AuxState::Returns = _returns;

	boost::shared_ptr<AuxVar1<ADDRINT, int, 0, 0>> _infunc(new StaticAuxVar1<ADDRINT, int, 0, 0>("InFunc"));
	AuxState::InFunc = _infunc;

	boost::shared_ptr<AuxVar1<ADDRINT, int, 0, 0>> _numinfunc(new StaticAuxVar1<ADDRINT, int, 0, 0>("NumInFunc"));
	AuxState::NumInFunc = _numinfunc;

	boost::shared_ptr<AuxVar1<ADDRINT, ADDRINT, 0, 0>> _arg0(new StaticAuxVar1<ADDRINT, ADDRINT, 0, 0>("Arg0"));
	AuxState::Arg0 = _arg0;

	boost::shared_ptr<AuxVar1<ADDRINT, ADDRINT, 0, 0>> _arg1(new StaticAuxVar1<ADDRINT, ADDRINT, 0, 0>("Arg1"));
	AuxState::Arg1 = _arg1;

	boost::shared_ptr<AuxVar1<ADDRINT, ADDRINT, 0, 0>> _retval(new StaticAuxVar1<ADDRINT, ADDRINT, 0, 0>("RetVal"));
	AuxState::RetVal = _retval;

	boost::shared_ptr<AuxVar0<int, -1>> _pc(new StaticAuxVar0<int, -1>("Pc"));
	AuxState::Pc = _pc;

	boost::shared_ptr<AuxVar0<bool, false>> _atpc(new StaticAuxVar0<bool, false>("AtPc"));
	AuxState::AtPc = _atpc;

	ThreadVarPtr _tid(new StaticThreadVar(NULL, "TID"));
	AuxState::Tid = _tid;
}

/*************************************************************************************/

void AuxState::Reset(THREADID t /*= -1*/) {
	Reads->reset(t);
	Writes->reset(t);

	CallsFrom->reset(t);
	CallsTo->reset(t);

	Enters->reset(t);
	Returns->reset(t);

	AtPc->reset(t);
}

/*************************************************************************************/

void AuxState::Clear() {
	Ends->clear();

	Reads->clear();
	Writes->clear();

	CallsFrom->clear();
	CallsTo->clear();

	Enters->clear();
	Returns->clear();

	Pc->clear();
	AtPc->clear();

	InFunc->clear();
	NumInFunc->clear();

	Arg0->clear();
	Arg1->clear();

	RetVal->clear();

	Tid->clear_thread();
}

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
	TransitionPredicatePtr p(new NAryTransitionPredicate<NAryAND>(pred1, pred2));
	return p;
}

TransitionPredicatePtr operator ||(const TransitionPredicatePtr& pred1, const TransitionPredicatePtr& pred2) {
	TransitionPredicatePtr p(new NAryTransitionPredicate<NAryOR>(pred1, pred2));
	return p;
}

/********************************************************************************/

//TransitionPredicatePtr operator && (const TransitionPredicatePtr& pred, const bool& b) {
//	TransitionPredicatePtr p(new NAryTransitionPredicate<NAryAND>(pred, (b ? TransitionPredicate::True() : TransitionPredicate::False())));
//	return p;
//}
//
//TransitionPredicatePtr operator || (const TransitionPredicatePtr& pred, const bool& b) {
//	TransitionPredicatePtr p(new NAryTransitionPredicate<NAryOR>(pred, (b ? TransitionPredicate::True() : TransitionPredicate::False())));
//	return p;
//}

/********************************************************************************/

class TPThreadVarsEqual : public TransitionPredicate {
public:
	TPThreadVarsEqual(const ThreadVarPtr& tvar1, const ThreadVarPtr& tvar2) : TransitionPredicate(), tvar1_(tvar1), tvar2_(tvar2) {}
	~TPThreadVarsEqual() {}

	bool EvalState(Coroutine* t = NULL) {
		safe_assert(tvar1_ != NULL && tvar2_ != NULL);
		Coroutine* co1 = tvar1_->thread();
		Coroutine* co2 = tvar2_->thread();
		if(co1 == NULL || co2 == NULL) {
			MYLOG(2) << "Evaluating TPThreadVarsEqual, one of them is NULL";
			return true;
		}
		MYLOG(2) << "Evaluating TPThreadVarsEqual for " << co1->tid() << " and " << co2->tid();
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

class TPThreadVarsNotEqual : public TransitionPredicate {
public:
	TPThreadVarsNotEqual(const ThreadVarPtr& tvar1, const ThreadVarPtr& tvar2) : TransitionPredicate(), tvar1_(tvar1), tvar2_(tvar2) {}
	~TPThreadVarsNotEqual() {}

	bool EvalState(Coroutine* t = NULL) {
		safe_assert(tvar1_ != NULL && tvar2_ != NULL);
		Coroutine* co1 = tvar1_->thread();
		Coroutine* co2 = tvar2_->thread();
		if(co1 == NULL || co2 == NULL) {
			MYLOG(2) << "Evaluating TPThreadVarsEqual, one of them is NULL";
			return true;
		}
		MYLOG(2) << "Evaluating TPThreadVarsEqual for " << co1->tid() << " and " << co2->tid();
		return co1->tid() != co2->tid();
	}

	static TransitionPredicatePtr create(const ThreadVarPtr& tvar1, const ThreadVarPtr& tvar2) {
		TransitionPredicatePtr p(new TPThreadVarsNotEqual(tvar1, tvar2));
		return p;
	}

private:
	DECL_FIELD(ThreadVarPtr, tvar1)
	DECL_FIELD(ThreadVarPtr, tvar2)
};

/********************************************************************************/

AssertionInstaller::AssertionInstaller(Scenario* scenario, const TransitionPredicatePtr& pred)
: scenario_(scenario), pred_(pred) {
	safe_assert(scenario_ != NULL);
	safe_assert(pred_ != NULL);
	scenario_->trans_assertions()->push_back(pred_);
}

AssertionInstaller::~AssertionInstaller() {
	safe_assert(scenario_ != NULL);
	safe_assert(pred_ != NULL);
	safe_assert(!scenario_->trans_assertions()->empty());
	safe_assert(scenario_->trans_assertions()->back().get() == pred_.get());
	scenario_->trans_assertions()->pop_back();
}

/********************************************************************************/

ConstraintInstaller::ConstraintInstaller(Scenario* scenario, const TransitionPredicatePtr& pred)
: scenario_(scenario), pred_(pred) {
	safe_assert(scenario_ != NULL);
	safe_assert(pred_ != NULL);
	scenario_->trans_constraints()->push_back(pred_);
}

ConstraintInstaller::~ConstraintInstaller() {
	safe_assert(scenario_ != NULL);
	safe_assert(pred_ != NULL);
	safe_assert(!scenario_->trans_constraints()->empty());
	safe_assert(scenario_->trans_constraints()->back().get() == pred_.get());
	scenario_->trans_constraints()->pop_back();
}

/********************************************************************************/

TransitionPredicatePtr operator == (const ThreadVarPtr& t1, const ThreadVarPtr& t2) {
	return TPThreadVarsEqual::create(t1, t2);
}

TransitionPredicatePtr operator != (const ThreadVarPtr& t1, const ThreadVarPtr& t2) {
	return TPThreadVarsNotEqual::create(t1, t2);
}

/********************************************************************************/

// ThreadVar

std::string ThreadVar::ToString() {
	return thread_ == NULL ? std::string("no-tid") : to_string(thread_->tid());
}

//bool operator==(const ThreadVar& tx, const ThreadVar& ty) {
////	Coroutine* co1 = const_cast<ThreadVar&>(tx).thread();
////	Coroutine* co2 = const_cast<ThreadVar&>(tx).thread();
////	if(co1 == NULL || co2 == NULL) {
////		return co1 == co2;
////	}
////	return co1->tid() == co2->tid();
//	return tx.id() == ty.id();
//}
//
//bool operator!=(const ThreadVar& tx, const ThreadVar& ty) {
//	return !(tx == ty);
//}
//
//bool operator<(const ThreadVar& tx, const ThreadVar& ty) {
////	Coroutine* co1 = const_cast<ThreadVar&>(tx).thread();
////	Coroutine* co2 = const_cast<ThreadVar&>(tx).thread();
////	if(co1 == NULL || co2 == NULL) {
////		return PTR2ADDRINT(co1) < PTR2ADDRINT(co2);
////	}
////	return co1->tid() < co2->tid();
//	return tx.id() < ty.id();
//}

/********************************************************************************/

// operator <
bool ThreadVarPtr_compare::operator()(const ThreadVarPtr& tx, const ThreadVarPtr& ty) const {
	safe_assert(tx != NULL && ty != NULL);
	return PTR2ADDRINT(tx.get()) < PTR2ADDRINT(ty.get());
}

/********************************************************************************/

// operator <
bool ThreadVarPtr_distinct_compare::operator()(const ThreadVarPtr& tx, const ThreadVarPtr& ty) const {
	safe_assert(tx != NULL && ty != NULL);
	return tx->tid() < ty->tid();
}

/********************************************************************************/

ThreadVarPtr& operator << (ThreadVarPtr& to, const ThreadVarPtr& from) {
	safe_assert(to != NULL);
	safe_assert(from != NULL);

	to->set_thread(from->thread());
	return to;
}

/********************************************************************************/

ThreadVarPtr& operator << (ThreadVarPtr& to, Coroutine* from) {
	safe_assert(to != NULL);
	safe_assert(from != NULL);
	to->set_thread(from);
	return to;
}

/********************************************************************************/

THREADID ThreadVar::tid() {
	safe_assert(!is_empty()); // TODO(elmas): should we allow tid() for empty variables?
	return thread_->tid();
}

/********************************************************************************/
//int ThreadVar::next_id_ = 0;
//
//// static
//int ThreadVar::get_id() {
//	return next_id_++;
//}
/********************************************************************************/

// constructing coroutine sets from comma-separated arguments
ThreadVarPtrSet MakeThreadVarPtrSet(ThreadVarPtr t1,
									ThreadVarPtr t2,
									ThreadVarPtr t3,
									ThreadVarPtr t4,
									ThreadVarPtr t5,
									ThreadVarPtr t6,
									ThreadVarPtr t7,
									ThreadVarPtr t8
									) {
	ThreadVarPtrSet set;
	if(t1.get() != NULL) set.insert(t1); else return set;
	if(t2.get() != NULL) set.insert(t2); else return set;
	if(t3.get() != NULL) set.insert(t3); else return set;
	if(t4.get() != NULL) set.insert(t4); else return set;
	if(t5.get() != NULL) set.insert(t5); else return set;
	if(t6.get() != NULL) set.insert(t6); else return set;
	if(t7.get() != NULL) set.insert(t7); else return set;
	if(t8.get() != NULL) set.insert(t8); else return set;
	return set;
}


//ThreadVarPtrSet MakeThreadVarPtrSet(ThreadVarPtr t, ...) {
//	va_list args;
//	ThreadVarPtrSet set;
//	va_start(args, t);
//	while (t.get() != NULL) {
//	   set.insert(t);
//	   t = va_arg(args, ThreadVarPtr);
//	}
//	va_end(args);
//	return set;
//}

/********************************************************************************/

bool TPInFunc::EvalState(Coroutine* t /*= NULL*/) {
//	safe_assert(t != NULL);

	THREADID tid = tvar_ == NULL ? t->tid() : safe_notnull(tvar_->thread())->tid();

	return AuxState::InFunc->get(addr_, tid) > 0;
}

TransitionPredicatePtr TPInFunc::create(const ADDRINT& addr, const ThreadVarPtr& tvar /*= ThreadVarPtr()*/) {
	safe_assert(addr != 0);
	TransitionPredicatePtr p(new TPInFunc(addr, tvar));
	return p;
}

/********************************************************************************/

// Thread expressions

ThreadExprPtr operator + (const ThreadExprPtr& e, const ThreadVarPtr& t) {
	ThreadExprPtr p(new PlusThreadExpr(e, t));
	return p;
}

ThreadExprPtr operator - (const ThreadExprPtr& e, const ThreadVarPtr& t) {
	ThreadExprPtr p(new MinusThreadExpr(e, t));
	return p;
}

ThreadExprPtr operator ! (const ThreadVarPtr& t) {
	ThreadExprPtr p(new NegThreadVarExpr(t));
	return p;
}


} // end namespace


