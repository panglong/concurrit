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

class PreStateTransitionPredicate : public TransitionPredicate {
public:
	PreStateTransitionPredicate() : TransitionPredicate(), result_(TPINVALID) {}
	~PreStateTransitionPredicate() {}

	// function to overrride
	virtual bool EvalState(Coroutine* t = NULL) = 0;

	TPVALUE EvalPreState(Coroutine* t = NULL) {
		bool result = EvalState(t);
		result_ = (result ? TPTRUE : TPFALSE);
		return result_;
	}

	bool EvalPostState(Coroutine* t = NULL) {
		safe_assert(result_ == TPTRUE || result_ == TPFALSE);
		return (result_ == TPTRUE);
	}
private:
	DECL_FIELD(TPVALUE, result)
};

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

/********************************************************************************/

// base class for auxiliary variables

class AuxVar {
public:
	AuxVar(const std::string& name) : name_(name) {}
	virtual ~AuxVar(){}

	virtual void reset(THREADID t = -1) = 0;
	virtual bool isset(THREADID t = -1) = 0;

private:
	DECL_FIELD(std::string, name)
};

/********************************************************************************/

template<typename T, T undef_value_>
class AuxVar0 : public AuxVar {
	typedef std::map<THREADID, T> M;
public:
	AuxVar0(const char* name = "") : AuxVar(name) {}
	~AuxVar0(){}

	TransitionPredicate* operator ()(const T& value, ThreadVar* tvar = NULL);

	TransitionPredicate* operator ()(AuxVar0* var, ThreadVar* tvar = NULL);

	virtual void reset(THREADID t = -1) {
		set(undef_value_, t);
	}

	virtual bool isset(THREADID t = -1) {
		return get(t) != undef_value_;
	}

	//================================================
	virtual T get(THREADID t = -1) {
		typename M::iterator itr = map_.find(t);
		if(itr == map_.end()) {
			return undef_value_;
		}
		return itr->second;
	}

	virtual void set(const T& value, THREADID t = -1) {
		map_[t] = value;
	}

private:
	DECL_FIELD(M, map)
};

/********************************************************************************/

template<typename T, T undef_value_>
class AuxConst0 : public AuxVar0<T, undef_value_> {
public:
	AuxConst0(const T& value = undef_value_) : AuxVar0<T,undef_value_>("const"), value_(value) {}
	~AuxConst0(){}

	void reset(THREADID t = -1) {
		value_ = undef_value_;
	}

	bool isset(THREADID t = -1) {
		return value_ != undef_value_;
	}

	//================================================
	T get(THREADID t = -1) {
		return value_;
	}

	void set(const T& value, THREADID t = -1) {
		value_ = value;
	}

private:
	DECL_FIELD(T, value)
};

/********************************************************************************/

template<typename T, T undef_value_>
class AuxVar0Pre : public PreStateTransitionPredicate {
	typedef AuxVar0<T,undef_value_> AuxVarType;
public:
	AuxVar0Pre(AuxVarType* var1, AuxVarType* var2, ThreadVar* tvar = NULL) : PreStateTransitionPredicate(), var1_(var1), var2_(var2), tvar_(tvar) {}
	~AuxVar0Pre(){}

	bool EvalState(Coroutine* t = NULL) {
		safe_assert(t != NULL);
		safe_assert(tvar_ == NULL || tvar_->thread() != NULL);

		THREADID tid = tvar_ == NULL ? -1 : tvar_->thread()->coid();

		if(t->coid() != tid) {
			return false;
		}
		return var1_->get(tid) == var2_->get(tid);
	}

private:
	DECL_FIELD(AuxVarType*, var1)
	DECL_FIELD(AuxVarType*, var2)
	DECL_FIELD(ThreadVar*, tvar)
};


template<typename T, T undef_value_>
TransitionPredicate* AuxVar0<T,undef_value_>::operator ()(const T& value, ThreadVar* tvar /*= NULL*/) {
	return this->operator()(new AuxConst0<T,undef_value_>(value), tvar);
}

template<typename T, T undef_value_>
TransitionPredicate* AuxVar0<T,undef_value_>::operator ()(AuxVar0<T, undef_value_>* var, ThreadVar* tvar /*= NULL*/) {
	return new AuxVar0Pre<T,undef_value_>(this, var, tvar);
}

/********************************************************************************/

template<typename K, typename T, K undef_key_, T undef_value_>
class AuxVar1 : public AuxVar {
protected:
	typedef std::map<K, T> MM;
	typedef std::map<THREADID, MM> M;
	typedef AuxVar0<K,undef_key_> AuxKeyType;
	typedef AuxVar0<T,undef_value_> AuxValueType;
	typedef AuxConst0<K,undef_key_> AuxKeyConstType;
	typedef AuxConst0<T,undef_value_> AuxValueConstType;
public:

	AuxVar1(const char* name = "") : AuxVar(name) {}
	~AuxVar1(){}

	TransitionPredicate* operator ()(const K& key, const T& value, ThreadVar* tvar = NULL) {
		return this->operator()(new AuxKeyConstType(key), new AuxValueConstType(value), tvar);
	}

	TransitionPredicate* operator ()(const K& key, ThreadVar* tvar = NULL) {
		return this->operator()(new AuxKeyConstType(key), NULL, tvar);
	}

	TransitionPredicate* operator ()(ThreadVar* tvar = NULL) {
		return this->operator()(NULL, NULL, tvar);
	}

	TransitionPredicate* operator ()(AuxKeyType* key = NULL, AuxValueType* value = NULL, ThreadVar* tvar = NULL);

	//================================================

	T get(const K& key, THREADID t = -1) {
		typename M::iterator itr = map_.find(t);
		if(itr == map_.end()) {
			return undef_value_;
		}
		typename MM::iterator itr2 = itr->second.find(key);
		if(itr2 == itr->second.end()) {
			return undef_key_;
		}
		return itr2->second;
	}

	void set(const K& key = undef_key_, const T& value = undef_value_, THREADID t = -1) {
		typename M::iterator itr = map_.find(t);
		if(itr == map_.end()) {
			map_[t] = MM();
		}
		map_[t][key] = value;
	}

	void set(const K& key = undef_key_, THREADID t = -1) {
		set(key, undef_value_, t);
	}

	bool isset(const K& key = undef_key_, THREADID t = -1) {
		typename M::iterator itr = map_.find(t);
		if(itr == map_.end()) {
			return false;
		}
		if(key == undef_key_) {
			return !itr->second.empty();
		}
		typename MM::iterator itr2 = itr->second.find(key);
		return (itr2 != itr->second.end());
	}

	bool isset(THREADID t = -1) {
		typename M::iterator itr = map_.find(t);
		if(itr == map_.end()) {
			return false;
		}
		return !itr->second.empty();
	}

	void reset(THREADID t = -1) {
		map_.erase(t); // delete map of t
	}

private:
	DECL_FIELD(M, map)
};

/********************************************************************************/

template<typename K, typename T,  K undef_key_, T undef_value_>
class AuxVar1Pre : public PreStateTransitionPredicate {
	typedef AuxVar1<K,T,undef_key_,undef_value_> AuxVarType;
	typedef AuxVar0<K,undef_key_> AuxKeyType;
	typedef AuxVar0<T,undef_value_> AuxValueType;
public:
	AuxVar1Pre(AuxVarType* var, AuxKeyType* key, AuxValueType* value, ThreadVar* tvar = NULL)
	: PreStateTransitionPredicate(), var_(var), key_(key), value_(value), tvar_(tvar) {}
	~AuxVar1Pre(){}

	bool EvalState(Coroutine* t = NULL) {
		safe_assert(t != NULL);
		safe_assert(tvar_ == NULL || tvar_->thread() != NULL);

		THREADID tid = tvar_ == NULL ? -1 : tvar_->thread()->coid();

		if(t->coid() != tid) {
			return false;
		}
		safe_assert(var_ != NULL);
		if(key_ == NULL) {
			safe_assert(value_ == NULL);
			return var_->isset(tid);
		} else if(value_ == NULL) {
			return var_->isset(key_->get(tid));
		} else {
			return var_->get(key_->get(tid), tid) == value_->get(tid);
		}
	}

private:
	DECL_FIELD(AuxVarType*, var)
	DECL_FIELD(AuxKeyType*, key)
	DECL_FIELD(AuxValueType*, value)
	DECL_FIELD(ThreadVar*, tvar)
};

/********************************************************************************/

template<typename K, typename T, K undef_key_, T undef_value_>
TransitionPredicate* AuxVar1<K,T,undef_key_,undef_value_>::operator ()(AuxKeyType* key /*= NULL*/, AuxValueType* value /*= NULL*/, ThreadVar* tvar /*= NULL*/) {
	return new AuxVar1Pre<K,T,undef_key_,undef_value_>(this, key, value, tvar);
}

/********************************************************************************/

class AuxState {
public:
	AuxState(){}
	~AuxState(){}

	void reset(THREADID t = -1) {
		Ends.reset(t);
		Reads.reset(t);
		Writes.reset(t);
		CallsFrom.reset(t);
		CallsTo.reset(t);
		Enters.reset(t);
		Returns.reset(t);
	}

	// auxiliary variables
	AuxVar0<bool, false> Ends;

	AuxVar1<ADDRINT, bool, -1, false> Reads;
	AuxVar1<ADDRINT, bool, -1, false> Writes;

	AuxVar1<ADDRINT, bool, -1, false> CallsFrom;
	AuxVar1<ADDRINT, bool, -1, false> CallsTo;
	AuxVar1<ADDRINT, bool, -1, false> Enters;
	AuxVar1<ADDRINT, bool, -1, false> Returns;
};





} // end namespace


#endif /* TRANSPRED_H_ */
