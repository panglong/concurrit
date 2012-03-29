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

#ifndef TRANSPRED_H_
#define TRANSPRED_H_

#include "common.h"
#include "transinfo.h"

namespace concurrit {

class Coroutine;

/********************************************************************************/

class ThreadVar {
public:
	ThreadVar(Coroutine* thread = NULL, const std::string& name = "<unknown>")
	: name_(name), thread_(thread) {}
	~ThreadVar() {}

private:
	DECL_FIELD(std::string, name)
	DECL_FIELD(Coroutine*, thread)
};

typedef boost::shared_ptr<ThreadVar> ThreadVarPtr;

/********************************************************************************/

enum TPVALUE { TPFALSE = 0, TPTRUE = 1, TPUNKNOWN = 2, TPINVALID = -1 };
TPVALUE TPNOT(TPVALUE v);
TPVALUE TPAND(TPVALUE v1, TPVALUE v2);
TPVALUE TPOR(TPVALUE v1, TPVALUE v2);

/********************************************************************************/

class TransitionPredicate {
public:
	TransitionPredicate() {}
	virtual ~TransitionPredicate() {}

	virtual TPVALUE EvalPreState(Coroutine* t = NULL) = 0;
	virtual bool EvalPostState(Coroutine* t = NULL) = 0;

	TPVALUE EvalPreState(const ThreadVarPtr& var) {
		return EvalPreState(var->thread());
	}
	bool EvalPostState(const ThreadVarPtr& var) {
		return EvalPostState(var->thread());
	}

	static TransitionPredicate* True();
	static TransitionPredicate* False();

	TransitionPredicate* operator ! ();
	TransitionPredicate* operator && (const TransitionPredicate& pred);
	TransitionPredicate* operator || (const TransitionPredicate& pred);

	TransitionPredicate* operator && (const bool& b);
	TransitionPredicate* operator || (const bool& b);
};

/********************************************************************************/

class TrueTransitionPredicate : public TransitionPredicate {
public:
	TrueTransitionPredicate() : TransitionPredicate() {}
	~TrueTransitionPredicate() {}
	TPVALUE EvalPreState(Coroutine* t = NULL) { return TPTRUE; }
	bool EvalPostState(Coroutine* t = NULL) { return TPTRUE; }
};

class FalseTransitionPredicate : public TransitionPredicate {
public:
	FalseTransitionPredicate() : TransitionPredicate() {}
	~FalseTransitionPredicate() {}
	TPVALUE EvalPreState(Coroutine* t = NULL) { return TPFALSE; }
	bool EvalPostState(Coroutine* t = NULL) { return TPFALSE; }
};

/********************************************************************************/

extern TrueTransitionPredicate __true_transition_predicate__;
extern FalseTransitionPredicate __false_transition_predicate__;

/********************************************************************************/

class NotTransitionPredicate : public TransitionPredicate {
public:
	NotTransitionPredicate(TransitionPredicate* pred) : TransitionPredicate(), pred_(pred) {}
	~NotTransitionPredicate() { if(pred_ != NULL) delete pred_; }
	TPVALUE EvalPreState(Coroutine* t = NULL) { return TPNOT(pred_->EvalPreState(t)); }
	bool EvalPostState(Coroutine* t = NULL) { return !(pred_->EvalPostState(t)); }
private:
	DECL_FIELD(TransitionPredicate*, pred)
};

/********************************************************************************/

enum NAryOp { NAryAND, NAryOR };
class NAryTransitionPredicate : public TransitionPredicate, public std::vector<TransitionPredicate*> {
public:
	NAryTransitionPredicate(NAryOp op = NAryAND, std::vector<TransitionPredicate*>* preds = NULL)
	: TransitionPredicate(), std::vector<TransitionPredicate*>(), op_(op) {
		if(preds != NULL) {
			for(iterator itr = preds->begin(), end = preds->end(); itr != end; ++itr) {
				push_back(*itr);
			}
		}
	}
	NAryTransitionPredicate(NAryOp op, TransitionPredicate* pred1, TransitionPredicate* pred2)
	: TransitionPredicate(), std::vector<TransitionPredicate*>(), op_(op) {
		push_back(pred1);
		push_back(pred2);
	}
	~NAryTransitionPredicate() {
		for(iterator itr = this->begin(), end = this->end(); itr != end; ++itr) {
			if(*itr != NULL) delete (*itr);
		}
	}

	TPVALUE EvalPreState(Coroutine* t = NULL) {
		safe_assert(!empty());
		TPVALUE v = (op_ == NAryAND) ? TPTRUE : TPFALSE;
		for(NAryTransitionPredicate::iterator itr = begin(); itr != end(); ++itr) {
			// update current
			TransitionPredicate* current = (*itr);
			// update v
			v = (op_ == NAryAND) ? TPAND(v, current->EvalPreState(t)) : TPOR(v, current->EvalPreState(t));
			if((op_ == NAryAND && v == TPFALSE) || (op_ == NAryOR && v == TPTRUE)) {
				break;
			}
		}
		return v;
	}

	bool EvalPostState(Coroutine* t = NULL) {
		safe_assert(!empty());
		bool v = (op_ == NAryAND) ? true : false;
		for(NAryTransitionPredicate::iterator itr = begin(); itr != end(); ++itr) {
			// update current
			TransitionPredicate* current = (*itr);
			// update v
			v = (op_ == NAryAND) ? (v && current->EvalPostState(t)) : (v || current->EvalPostState(t));
			if((op_ == NAryAND && v == false) || (op_ == NAryOR && v == true)) {
				break;
			}
		}
		return v;
	}
private:
	DECL_FIELD(NAryOp, op)
};

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

// constraints
class TransitionConstraint : public TransitionPredicate {
public:
	TransitionConstraint(Scenario* scenario);

	virtual ~TransitionConstraint();

	virtual TPVALUE EvalPreState(Coroutine* t = NULL) = 0;
	virtual bool EvalPostState(Coroutine* t = NULL) = 0;

private:
	DECL_FIELD(Scenario*, scenario)
};

/********************************************************************************/

class TransitionConstraintAll : public TransitionConstraint {
public:
	TransitionConstraintAll(Scenario* scenario, TransitionPredicate* pred)
	: TransitionConstraint(scenario), pred_(pred) {}

	~TransitionConstraintAll() { if(pred_ != NULL) delete pred_; }

	virtual TPVALUE EvalPreState(Coroutine* t = NULL) {
		return pred_->EvalPreState(t);
	}
	virtual bool EvalPostState(Coroutine* t = NULL) {
		return pred_->EvalPostState(t);
	}

private:
	DECL_FIELD(TransitionPredicate*, pred)
};


/********************************************************************************/

class TransitionConstraintFirst : public TransitionConstraintAll {
public:
	TransitionConstraintFirst(Scenario* scenario, TransitionPredicate* pred)
	: TransitionConstraintAll(scenario, pred), done_(false) {}

	~TransitionConstraintFirst() {}

	virtual TPVALUE EvalPreState(Coroutine* t = NULL) {
		if(!done_) {
			done_ = true;
			return pred_->EvalPreState(t);
		} else {
			return TPTRUE;
		}
	}

	virtual bool EvalPostState(Coroutine* t = NULL) {
		if(!done_) {
			done_ = true;
			return pred_->EvalPostState(t);
		} else {
			return TPTRUE;
		}
	}

private:
	DECL_FIELD(bool, done)
};

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/




} // end namespace


#endif /* TRANSPRED_H_ */
