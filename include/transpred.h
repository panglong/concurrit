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
#include "tbb/concurrent_hash_map.h"
#include <boost/shared_ptr.hpp>

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
class TransitionPredicate;
typedef boost::shared_ptr<TransitionPredicate> TransitionPredicatePtr;

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

	static TransitionPredicatePtr True();
	static TransitionPredicatePtr False();

	TransitionPredicatePtr operator ! ();
	TransitionPredicatePtr operator && (const TransitionPredicatePtr& pred);
	TransitionPredicatePtr operator || (const TransitionPredicatePtr& pred);

	TransitionPredicatePtr operator && (const bool& b);
	TransitionPredicatePtr operator || (const bool& b);
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
	NotTransitionPredicate(TransitionPredicatePtr pred) : TransitionPredicate(), pred_(pred) {}
	NotTransitionPredicate(TransitionPredicate* pred) : TransitionPredicate(), pred_(TransitionPredicatePtr(pred)) {}
	~NotTransitionPredicate() {}
	TPVALUE EvalPreState(Coroutine* t = NULL) { return TPNOT(pred_->EvalPreState(t)); }
	bool EvalPostState(Coroutine* t = NULL) { return !(pred_->EvalPostState(t)); }
private:
	DECL_FIELD(TransitionPredicatePtr, pred)
};

/********************************************************************************/

enum NAryOp { NAryAND, NAryOR };
class NAryTransitionPredicate : public TransitionPredicate, public std::vector<TransitionPredicatePtr> {
public:
	NAryTransitionPredicate(NAryOp op = NAryAND, std::vector<TransitionPredicatePtr>* preds = NULL)
	: TransitionPredicate(), std::vector<TransitionPredicatePtr>(), op_(op) {
		if(preds != NULL) {
			for(iterator itr = preds->begin(), end = preds->end(); itr != end; ++itr) {
				push_back(*itr);
			}
		}
	}
	NAryTransitionPredicate(NAryOp op, TransitionPredicatePtr pred1, TransitionPredicatePtr pred2)
	: TransitionPredicate(), std::vector<TransitionPredicatePtr>(), op_(op) {
		push_back(pred1);
		push_back(pred2);
	}
	~NAryTransitionPredicate() {}

	TPVALUE EvalPreState(Coroutine* t = NULL) {
		safe_assert(!empty());
		TPVALUE v = (op_ == NAryAND) ? TPTRUE : TPFALSE;
		for(NAryTransitionPredicate::iterator itr = begin(); itr != end(); ++itr) {
			// update current
			TransitionPredicatePtr current = (*itr);
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
			TransitionPredicatePtr current = (*itr);
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
	TransitionConstraintAll(Scenario* scenario, TransitionPredicatePtr pred)
	: TransitionConstraint(scenario), pred_(pred) {}

	~TransitionConstraintAll() {}

	virtual TPVALUE EvalPreState(Coroutine* t = NULL) {
		return pred_->EvalPreState(t);
	}
	virtual bool EvalPostState(Coroutine* t = NULL) {
		return pred_->EvalPostState(t);
	}

private:
	DECL_FIELD(TransitionPredicatePtr, pred)
};


/********************************************************************************/

class TransitionConstraintFirst : public TransitionConstraintAll {
public:
	TransitionConstraintFirst(Scenario* scenario, TransitionPredicatePtr pred)
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
	virtual void clear() = 0;

private:
	DECL_FIELD(std::string, name)
};

/********************************************************************************/

template<typename T, T undef_value_>
class AuxVar0 : public AuxVar {
	typedef tbb::concurrent_hash_map<THREADID, T> M;
	typedef boost::shared_ptr<AuxVar0<T,undef_value_>> AuxVar0Ptr;
public:
	AuxVar0(const char* name = "") : AuxVar(name) {}
	~AuxVar0(){}

	TransitionPredicatePtr operator ()(const T& value, const ThreadVarPtr& tvar = ThreadVarPtr());

	TransitionPredicatePtr operator ()(AuxVar0Ptr var, const ThreadVarPtr& tvar = ThreadVarPtr());

	virtual void reset(THREADID t = -1) {
		set(undef_value_, t);
	}

	virtual bool isset(THREADID t = -1) {
		return get(t) != undef_value_;
	}

	virtual void clear() {
		map_.clear();
	}

	//================================================
	virtual T get(THREADID t = -1) {
		typename M::const_accessor acc;
		if(!map_.find(acc, t)) {
			return undef_value_;
		}
		return acc->second;
	}

	virtual void set(const T& value, THREADID t = -1) {
		typename M::accessor acc;
		map_.insert(acc, t);
		acc->second = value;
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

	void clear() {
		AuxVar0<T, undef_value_>::clear();
		value_ = undef_value_;
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
	typedef boost::shared_ptr<AuxVar0<T,undef_value_>> AuxVar0Ptr;
public:
	AuxVar0Pre(const AuxVar0Ptr& var1, const AuxVar0Ptr& var2, const ThreadVarPtr& tvar = ThreadVarPtr()) : PreStateTransitionPredicate(), var1_(var1), var2_(var2), tvar_(tvar) {}
	~AuxVar0Pre(){}

	bool EvalState(Coroutine* t = NULL) {
		safe_assert(t != NULL);
		safe_assert(tvar_ == NULL || (tvar_->thread() == t));

		THREADID tid = tvar_ == NULL ? t->coid() : tvar_->thread()->coid();

		safe_assert(var1_ != NULL && var2_ != NULL);
		return var1_->get(tid) == var2_->get(tid);
	}

private:
	DECL_FIELD(AuxVar0Ptr, var1)
	DECL_FIELD(AuxVar0Ptr, var2)
	DECL_FIELD(ThreadVarPtr, tvar)
};


template<typename T, T undef_value_>
TransitionPredicatePtr AuxVar0<T,undef_value_>::operator ()(const T& value, const ThreadVarPtr& tvar /*= NULL*/) {
	return this->operator()(AuxVar0Ptr(new AuxConst0<T,undef_value_>(value)), tvar);
}

template<typename T, T undef_value_>
TransitionPredicatePtr AuxVar0<T,undef_value_>::operator ()(AuxVar0Ptr var, const ThreadVarPtr& tvar /*= NULL*/) {
	return TransitionPredicatePtr(new AuxVar0Pre<T,undef_value_>(AuxVar0Ptr(this), var, tvar));
}

/********************************************************************************/

template<typename K, typename T, K undef_key_, T undef_value_>
class AuxVar1 : public AuxVar {
protected:
	typedef tbb::concurrent_hash_map<K, T> MM;
	typedef tbb::concurrent_hash_map<THREADID, MM> M;
	typedef AuxVar0<K,undef_key_> AuxKeyType;
	typedef boost::shared_ptr<AuxKeyType> AuxKeyPtr;
	typedef AuxVar0<T,undef_value_> AuxValueType;
	typedef boost::shared_ptr<AuxValueType> AuxValuePtr;
	typedef AuxConst0<K,undef_key_> AuxKeyConstType;
	typedef boost::shared_ptr<AuxKeyConstType> AuxKeyConstPtr;
	typedef AuxConst0<T,undef_value_> AuxValueConstType;
	typedef boost::shared_ptr<AuxValueConstType> AuxValueConstPtr;
public:

	AuxVar1(const char* name = "") : AuxVar(name) {}
	~AuxVar1(){}

	TransitionPredicatePtr operator ()(const K& key, const T& value, const ThreadVarPtr& tvar = ThreadVarPtr()) {
		return this->operator()(AuxKeyConstPtr(new AuxKeyConstType(key)), AuxValueConstPtr(new AuxValueConstType(value)), tvar);
	}

	TransitionPredicatePtr operator ()(const K& key, const ThreadVarPtr& tvar = ThreadVarPtr()) {
		return this->operator()(AuxKeyConstPtr(new AuxKeyConstType(key)), AuxValueConstPtr(), tvar);
	}

	TransitionPredicatePtr operator ()(const ThreadVarPtr& tvar = ThreadVarPtr()) {
		return this->operator()(AuxKeyConstPtr(), AuxValueConstPtr(), tvar);
	}

	TransitionPredicatePtr operator ()(const AuxKeyPtr& key = AuxKeyPtr(), const AuxValuePtr& value = AuxValuePtr(), const ThreadVarPtr& tvar = ThreadVarPtr());

	//================================================

	T get(const K& key, THREADID t = -1) {
		typename M::const_accessor acc;
		if(!map_.find(acc, t)) {
			return undef_value_;
		}
		typename MM::const_accessor acc2;
		if(!acc->second.find(acc2, key)) {
			return undef_key_;
		}
		return acc2->second;
	}

	void set(const K& key = undef_key_, const T& value = undef_value_, THREADID t = -1) {
		typename M::accessor acc;
		if(!map_.find(acc, t)) {
			map_.insert(acc, t);
			acc->second = MM();
		}
		typename MM::accessor acc2;
		acc->second.insert(acc2, key);
		acc2->second = value;
	}

	void set(const K& key = undef_key_, THREADID t = -1) {
		set(key, undef_value_, t);
	}

	bool isset(const K& key = undef_key_, THREADID t = -1) {
		typename M::const_accessor acc;
		if(!map_.find(acc, t)) {
			return false;
		}
		if(key == undef_key_) {
			return !acc->second.empty();
		}
		typename MM::const_accessor acc2;
		return acc->second.find(acc2, key);
	}

	bool isset(THREADID t = -1) {
		typename M::const_accessor acc;
		if(!map_.find(acc, t)) {
			return false;
		}
		return !acc->second.empty();
	}

	void reset(THREADID t = -1) {
		map_.erase(t); // delete map of t
	}

	void clear() {
		map_.clear();
	}
private:
	DECL_FIELD(M, map)
};

/********************************************************************************/

template<typename K, typename T,  K undef_key_, T undef_value_>
class AuxVar1Pre : public PreStateTransitionPredicate {
	typedef AuxVar1<K,T,undef_key_,undef_value_> AuxVarType;
	typedef boost::shared_ptr<AuxVarType> AuxVarPtr;
	typedef AuxVar0<K,undef_key_> AuxKeyType;
	typedef boost::shared_ptr<AuxKeyType> AuxKeyPtr;
	typedef AuxVar0<T,undef_value_> AuxValueType;
	typedef boost::shared_ptr<AuxValueType> AuxValuePtr;
	typedef AuxConst0<K,undef_key_> AuxKeyConstType;
	typedef boost::shared_ptr<AuxKeyConstType> AuxKeyConstPtr;
	typedef AuxConst0<T,undef_value_> AuxValueConstType;
	typedef boost::shared_ptr<AuxValueConstType> AuxValueConstPtr;
public:
	AuxVar1Pre(const AuxVarPtr& var, const AuxKeyPtr& key, const AuxValuePtr& value, const ThreadVarPtr& tvar = ThreadVarPtr())
	: PreStateTransitionPredicate(), var_(var), key_(key), value_(value), tvar_(tvar) {}
	~AuxVar1Pre(){}

	bool EvalState(Coroutine* t = NULL) {
		safe_assert(t != NULL);
		safe_assert(tvar_ == NULL || (tvar_->thread() == t));

		THREADID tid = tvar_ == NULL ? t->coid() : tvar_->thread()->coid();

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
	DECL_FIELD(AuxVarPtr, var)
	DECL_FIELD(AuxKeyPtr, key)
	DECL_FIELD(AuxValuePtr, value)
	DECL_FIELD(ThreadVarPtr, tvar)
};

/********************************************************************************/

template<typename K, typename T, K undef_key_, T undef_value_>
TransitionPredicatePtr AuxVar1<K,T,undef_key_,undef_value_>::operator ()(const AuxKeyPtr& key /*= NULL*/, const AuxValuePtr& value /*= NULL*/, const ThreadVarPtr& tvar /*= NULL*/) {
	return TransitionPredicatePtr(new AuxVar1Pre<K,T,undef_key_,undef_value_>(AuxVarPtr(this), key, value, tvar));
}

/********************************************************************************/

class AuxState {
private:
	AuxState(){}
	~AuxState(){}
public:

	static void Reset(THREADID t = -1) {
		Ends->reset(t);
		Reads->reset(t);
		Writes->reset(t);
		CallsFrom->reset(t);
		CallsTo->reset(t);
		Enters->reset(t);
		Returns->reset(t);
	}

	static void Clear() {
		Ends->clear();
		Reads->clear();
		Writes->clear();
		CallsFrom->clear();
		CallsTo->clear();
		Enters->clear();
		Returns->clear();
	}

	// auxiliary variables
	static boost::shared_ptr<AuxVar0<bool, false>> Ends;

	static boost::shared_ptr<AuxVar1<ADDRINT, bool, -1, false>> Reads;
	static boost::shared_ptr<AuxVar1<ADDRINT, bool, -1, false>> Writes;

	static boost::shared_ptr<AuxVar1<ADDRINT, bool, -1, false>> CallsFrom;
	static boost::shared_ptr<AuxVar1<ADDRINT, bool, -1, false>> CallsTo;
	static boost::shared_ptr<AuxVar1<ADDRINT, bool, -1, false>> Enters;
	static boost::shared_ptr<AuxVar1<ADDRINT, bool, -1, false>> Returns;
};

/********************************************************************************/

#define ENDS()			AuxState::Ends()

#define READS()			AuxState::Reads()
#define WRITES()		AuxState::Writes()

#define READS_FROM(x)	AuxState::Reads(x)
#define WRITES_TO(x)	AuxState::Writes(x)


} // end namespace


#endif /* TRANSPRED_H_ */
