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
#include "thread.h"
#include "tbb/concurrent_hash_map.h"
#include <boost/shared_ptr.hpp>

namespace concurrit {

class Coroutine;

/********************************************************************************/

class FuncVar {
public:
	explicit FuncVar(void* addr) : addr_(PTR2ADDRINT(addr)) {}
	~FuncVar() {}

	operator ADDRINT() { return addr_; }
	operator void*() { return ADDRINT2PTR(addr_); }

private:
	DECL_FIELD(ADDRINT, addr)
};

/********************************************************************************/

class ThreadVar {
public:
	explicit ThreadVar(Coroutine* thread = NULL, const std::string& name = "<unknown>")
	: name_(name), thread_(thread) {}
	virtual ~ThreadVar() {}

	std::string ToString() {
		return thread_ == NULL ? std::string("no-tid") : to_string(thread_->tid());
	}

	inline void clear_thread() { thread_ = NULL; }
	inline bool is_empty() { return thread_ == NULL; }

private:
	DECL_FIELD(std::string, name)
	DECL_FIELD(Coroutine*, thread)
};

// Thread variable that should not be deleted
class StaticThreadVar : public ThreadVar {
public:
	explicit StaticThreadVar(Coroutine* thread = NULL, const std::string& name = "<unknown>")
	: ThreadVar(thread, name) {}
	~StaticThreadVar() {
		if(Concurrit::IsInitialized()) {
			safe_fail("StaticThreadVar %s should not be deleted while Concurrit is active!", name_.c_str());
		}
	}
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
};

TransitionPredicatePtr operator ! (const TransitionPredicatePtr& pred);
TransitionPredicatePtr operator && (const TransitionPredicatePtr& pred1, const TransitionPredicatePtr& pred2);
TransitionPredicatePtr operator || (const TransitionPredicatePtr& pred1, const TransitionPredicatePtr& pred2);

TransitionPredicatePtr operator && (const TransitionPredicatePtr& pred, const bool& b);
TransitionPredicatePtr operator || (const TransitionPredicatePtr& pred, const bool& b);

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
		CHECK(result_ == TPTRUE || result_ == TPFALSE) << "result_ is " << to_string(result_);
		return (result_ == TPTRUE);
	}
private:
	DECL_FIELD(TPVALUE, result)
};

/********************************************************************************/

class NotTransitionPredicate : public TransitionPredicate {
public:
	explicit NotTransitionPredicate(TransitionPredicatePtr pred) : TransitionPredicate(), pred_(pred) {}
	NotTransitionPredicate(TransitionPredicate* pred) : TransitionPredicate(), pred_(TransitionPredicatePtr(pred)) {}
	~NotTransitionPredicate() {}
	TPVALUE EvalPreState(Coroutine* t = NULL) { return TPNOT(pred_->EvalPreState(t)); }
	bool EvalPostState(Coroutine* t = NULL) { return !(pred_->EvalPostState(t)); }
private:
	DECL_FIELD(TransitionPredicatePtr, pred)
};

/********************************************************************************/

enum NAryOp { NAryAND = 1, NAryOR = 2 };

template<NAryOp op_>
class NAryTransitionPredicate : public TransitionPredicate, public std::vector<TransitionPredicatePtr> {
public:
	NAryTransitionPredicate(std::vector<TransitionPredicatePtr>* preds = NULL)
	: TransitionPredicate(), std::vector<TransitionPredicatePtr>() {
		if(preds != NULL) {
			for(iterator itr = preds->begin(); itr != preds->end(); ++itr) {
				push_back(*itr);
			}
		}
	}
	NAryTransitionPredicate(TransitionPredicatePtr pred1, TransitionPredicatePtr pred2)
	: TransitionPredicate(), std::vector<TransitionPredicatePtr>() {
		push_back(pred1);
		push_back(pred2);
	}
	~NAryTransitionPredicate() {}

	TPVALUE EvalPreState(Coroutine* t = NULL) {
		TPVALUE v = (op_ == NAryAND) ? TPTRUE : TPFALSE;
		for(NAryTransitionPredicate<op_>::iterator itr = begin(); itr != end(); ++itr) {
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
		bool v = (op_ == NAryAND) ? true : false;
		for(NAryTransitionPredicate<op_>::iterator itr = begin(); itr != end(); ++itr) {
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

	boost::shared_ptr<NAryTransitionPredicate<op_>> Clone() {
		if(empty()) {
			return boost::shared_ptr<NAryTransitionPredicate<op_>>();
		}
		boost::shared_ptr<NAryTransitionPredicate<op_>> p(new NAryTransitionPredicate<op_>());
		for(iterator itr = begin(); itr != end(); ++itr) {
			p->push_back(*itr);
		}
		return p;
	}
};

typedef NAryTransitionPredicate<NAryAND> TransitionConstraints;
typedef boost::shared_ptr<NAryTransitionPredicate<NAryAND>> TransitionConstraintsPtr;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class Scenario;

// constraints
class ConstraintInstaller {
public:
	ConstraintInstaller(Scenario* scenario, const TransitionPredicatePtr& pred);
	~ConstraintInstaller();

private:
	DECL_FIELD(Scenario*, scenario)
	DECL_FIELD(TransitionPredicatePtr, pred)
};

/********************************************************************************/

class TransitionConstraintAll : public TransitionPredicate {
public:
	explicit TransitionConstraintAll(const TransitionPredicatePtr& pred)
	: TransitionPredicate(), pred_(pred) {}

	~TransitionConstraintAll() {}

	TPVALUE EvalPreState(Coroutine* t = NULL) {
		return pred_->EvalPreState(t);
	}
	bool EvalPostState(Coroutine* t = NULL) {
		return pred_->EvalPostState(t);
	}

private:
	DECL_FIELD(TransitionPredicatePtr, pred)
};


/********************************************************************************/

class TransitionConstraintFirst : public TransitionPredicate {
public:
	explicit TransitionConstraintFirst(const TransitionPredicatePtr& pred)
	: TransitionPredicate(), pred_(pred) {}

	~TransitionConstraintFirst() {}

	TPVALUE EvalPreState(Coroutine* t = NULL) {
		return pred_->EvalPreState(t);
	}

	bool EvalPostState(Coroutine* t = NULL) {
		if(pred_->EvalPreState(t)) {
			pred_ = TransitionPredicate::True();
			return true;
		}
		return false;
	}

private:
	DECL_FIELD(TransitionPredicatePtr, pred)
};

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

/********************************************************************************/

// base class for auxiliary variables

class AuxVar {
public:
	explicit AuxVar(const std::string& name) : name_(name) {}
	virtual ~AuxVar(){
		if(name_ != "const") {
			fprintf(stderr, "Deleting non-constant auxiliary variable: %s!\n", name_.c_str());
		}
	}

	virtual void reset(THREADID t = -1) = 0;
	virtual bool isset(THREADID t = -1) = 0;
	virtual void clear() = 0;

private:
	DECL_FIELD(std::string, name)
	DECL_FIELD(RWLock, rwlock)
};

#define RLOCK() ScopeRLock __sm__(&rwlock_)
#define WLOCK() ScopeWLock __sm__(&rwlock_)

/********************************************************************************/

template<typename T, T undef_value_>
class AuxVar0 : public AuxVar {
	typedef tbb::concurrent_hash_map<THREADID, T> M;
	typedef boost::shared_ptr<AuxVar0<T,undef_value_>> AuxVar0Ptr;
public:
	explicit AuxVar0(const char* name = "") : AuxVar(name) {}
	virtual ~AuxVar0(){}

	TransitionPredicatePtr TP0(const AuxVar0Ptr& var1, const ThreadVarPtr& tvar);
	TransitionPredicatePtr TP1(const AuxVar0Ptr& var1, const T& value, const ThreadVarPtr& tvar);
	TransitionPredicatePtr TP2(const AuxVar0Ptr& var1, const AuxVar0Ptr& var2, const ThreadVarPtr& tvar);

	virtual void reset(THREADID t = -1) {
		set(undef_value_, t);
	}

	virtual bool isset(THREADID t = -1) {
		return get(t) != undef_value_;
	}

	virtual void clear() {
		WLOCK();

		map_.clear();
	}

	//================================================
	virtual T get(THREADID t = -1) {
		RLOCK();

		typename M::const_accessor acc;
		if(!map_.find(acc, t)) {
			return undef_value_;
		}
		return acc->second;
	}

	virtual void set(const T& value, THREADID t = -1) {
		WLOCK();

		typename M::accessor acc;
		map_.insert(acc, t);
		acc->second = value;
	}

private:
	DECL_FIELD(M, map)
};

/********************************************************************************/

template<typename T, T undef_value_>
class StaticAuxVar0 : public AuxVar0<T, undef_value_> {
public:
	explicit StaticAuxVar0(const char* name = "") : AuxVar0<T, undef_value_>(name) {}
	~StaticAuxVar0() {
		if(Concurrit::IsInitialized()) {
			safe_fail("StaticAuxVar0 %s should not be deleted while Concurrit is active!", AuxVar::name().c_str());
		}
	}
};

/********************************************************************************/

template<typename T, T undef_value_>
class AuxConst0 : public AuxVar0<T, undef_value_> {
public:
	explicit AuxConst0(const T& value) : AuxVar0<T,undef_value_>("const"), value_(value) {}
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
	AuxVar0Pre(const AuxVar0Ptr& var1, const AuxVar0Ptr& var2, const ThreadVarPtr& tvar) : PreStateTransitionPredicate(), var1_(var1), var2_(var2), tvar_(tvar) {}
	~AuxVar0Pre(){}

	static TransitionPredicatePtr create (const AuxVar0Ptr& var1, const AuxVar0Ptr& var2, const ThreadVarPtr& tvar) {
		TransitionPredicatePtr p(new AuxVar0Pre<T,undef_value_>(var1, var2, tvar));
		return p;
	}

	bool EvalState(Coroutine* t = NULL) {
		safe_assert(t != NULL);

		THREADID tid = tvar_ == NULL ? t->tid() : safe_notnull(tvar_->thread())->tid();

		safe_assert(var1_ != NULL);
		if(var2_ == NULL) {
			return var1_->isset(tid);
		}
		return var1_->get(tid) == var2_->get(tid);
	}

private:
	DECL_FIELD(AuxVar0Ptr, var1)
	DECL_FIELD(AuxVar0Ptr, var2)
	DECL_FIELD(ThreadVarPtr, tvar)
};

template<typename T, T undef_value_>
TransitionPredicatePtr AuxVar0<T,undef_value_>::TP0(const AuxVar0Ptr& var1, const ThreadVarPtr& tvar) {
	return this->TP2(var1, AuxVar0Ptr(), tvar);
}

template<typename T, T undef_value_>
TransitionPredicatePtr AuxVar0<T,undef_value_>::TP1(const AuxVar0Ptr& var1, const T& value, const ThreadVarPtr& tvar) {
	AuxVar0Ptr p(new AuxConst0<T,undef_value_>(value));
	return this->TP2(var1, p, tvar);
}

template<typename T, T undef_value_>
TransitionPredicatePtr AuxVar0<T,undef_value_>::TP2(const AuxVar0Ptr& var1, const AuxVar0Ptr& var2, const ThreadVarPtr& tvar) {
	return AuxVar0Pre<T,undef_value_>::create(var1, var2, tvar);
}

/********************************************************************************/

template<typename K, typename T, K undef_key_, T undef_value_>
class AuxVar1 : public AuxVar {
protected:
	typedef tbb::concurrent_hash_map<K, T> MM;
	typedef tbb::concurrent_hash_map<THREADID, MM> M;
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

	explicit AuxVar1(const char* name = "") : AuxVar(name) {}
	virtual ~AuxVar1(){}

	TransitionPredicatePtr TP0(const AuxVarPtr& var, const ThreadVarPtr& tvar);
	TransitionPredicatePtr TP1(const AuxVarPtr& var, const K& key, const ThreadVarPtr& tvar);
	TransitionPredicatePtr TP2(const AuxVarPtr& var, const AuxKeyPtr& key, const ThreadVarPtr& tvar);
	TransitionPredicatePtr TP3(const AuxVarPtr& var, const K& key, const T& value, const ThreadVarPtr& tvar);
	TransitionPredicatePtr TP4(const AuxVarPtr& var, const AuxKeyPtr& key, const AuxValuePtr& value, const ThreadVarPtr& tvar);

	//================================================

	T get(const K& key, THREADID t = -1) {
		RLOCK();

		typename M::const_accessor acc;
		if(!map_.find(acc, t)) {
			return undef_value_;
		}
		typename MM::const_accessor acc2;
		if(!acc->second.find(acc2, key)) {
			return undef_value_;
		}
		T value = acc2->second;
		safe_assert(value != undef_value_);
		return value;
	}

	void set(const K& key, const T& value, THREADID t = -1) {
		WLOCK();

		safe_assert(key != undef_key_);
		typename M::accessor acc;
		if(!map_.find(acc, t)) {
			map_.insert(acc, t);
			acc->second = MM();
		}
		if(value != undef_value_) {
			typename MM::accessor acc2;
			acc->second.insert(acc2, key);
			acc2->second = value;
		}
	}

	bool isset(const K& key, THREADID t = -1) {
		RLOCK();

		typename M::const_accessor acc;
		if(!map_.find(acc, t)) {
			return false;
		}
		typename MM::const_accessor acc2;
		bool _isset = acc->second.find(acc2, key);
		safe_assert(acc2->second != undef_value_);
		return _isset;
	}

	bool isset(THREADID t = -1) {
		RLOCK();

		typename M::const_accessor acc;
		if(!map_.find(acc, t)) {
			return false;
		}
		return !acc->second.empty();
	}

	void reset(THREADID t = -1) {
		WLOCK();

		typename M::accessor acc;
		if(map_.find(acc, t)) {
			acc->second.clear();
		}
//		map_.erase(t);
	}

	void clear() {
		WLOCK();

		map_.clear();
	}
private:
	DECL_FIELD(M, map)
};

/********************************************************************************/

template<typename K, typename T, K undef_key_, T undef_value_>
class StaticAuxVar1 : public AuxVar1<K, T, undef_key_, undef_value_> {
public:
	explicit StaticAuxVar1(const char* name = "") : AuxVar1<K, T, undef_key_, undef_value_>(name) {}
	~StaticAuxVar1() {
		if(Concurrit::IsInitialized()) {
			safe_fail("StaticAuxVar1 %s should not be deleted while Concurrit is active!", AuxVar::name().c_str());
		}
	}
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
	AuxVar1Pre(const AuxVarPtr& var, const AuxKeyPtr& key, const AuxValuePtr& value, const ThreadVarPtr& tvar)
	: PreStateTransitionPredicate(), var_(var), key_(key), value_(value), tvar_(tvar) {}
	~AuxVar1Pre(){}

	static TransitionPredicatePtr create (const AuxVarPtr& var, const AuxKeyPtr& key, const AuxValuePtr& value, const ThreadVarPtr& tvar) {
		TransitionPredicatePtr p(new AuxVar1Pre<K,T,undef_key_,undef_value_>(var, key, value, tvar));
		return p;
	}

	bool EvalState(Coroutine* t = NULL) {
		safe_assert(t != NULL);

		THREADID tid = tvar_ == NULL ? t->tid() : safe_notnull(tvar_->thread())->tid();

		safe_assert(var_ != NULL);
		if(key_ == NULL) {
			safe_assert(value_ == NULL);
			return var_->isset(tid);
		} else if(value_ == NULL) {
			return var_->isset(key_->get(tid), tid);
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
TransitionPredicatePtr AuxVar1<K,T,undef_key_,undef_value_>::TP0(const AuxVarPtr& var, const ThreadVarPtr& tvar) {
	return this->TP4(var, AuxKeyConstPtr(), AuxValueConstPtr(), tvar);
}

template<typename K, typename T, K undef_key_, T undef_value_>
TransitionPredicatePtr AuxVar1<K,T,undef_key_,undef_value_>::TP1(const AuxVarPtr& var, const K& key, const ThreadVarPtr& tvar) {
	AuxKeyConstPtr p(new AuxKeyConstType(key));
	return this->TP4(var, p, AuxValueConstPtr(), tvar);
}

template<typename K, typename T, K undef_key_, T undef_value_>
TransitionPredicatePtr AuxVar1<K,T,undef_key_,undef_value_>::TP2(const AuxVarPtr& var, const AuxKeyPtr& key, const ThreadVarPtr& tvar) {
	return this->TP4(var, key, AuxValueConstPtr(), tvar);
}

template<typename K, typename T, K undef_key_, T undef_value_>
TransitionPredicatePtr AuxVar1<K,T,undef_key_,undef_value_>::TP3(const AuxVarPtr& var, const K& key, const T& value, const ThreadVarPtr& tvar) {
	AuxKeyConstPtr p(new AuxKeyConstType(key));
	AuxValueConstPtr q(new AuxValueConstType(value));
	return this->TP4(var, p, q, tvar);
}

template<typename K, typename T, K undef_key_, T undef_value_>
TransitionPredicatePtr AuxVar1<K,T,undef_key_,undef_value_>::TP4(const AuxVarPtr& var, const AuxKeyPtr& key, const AuxValuePtr& value, const ThreadVarPtr& tvar) {
	return AuxVar1Pre<K,T,undef_key_,undef_value_>::create(var, key, value, tvar);
}

/********************************************************************************/

class AuxState {
private:
	AuxState(){}
	~AuxState(){}
public:

	static void Init();

	static void Reset(THREADID t = -1);

	static void Clear();

	// auxiliary variables
	static boost::shared_ptr<AuxVar0<bool, false>> Ends;

	static boost::shared_ptr<AuxVar1<ADDRINT, uint32_t, -1, 0>> Reads;
	static boost::shared_ptr<AuxVar1<ADDRINT, uint32_t, -1, 0>> Writes;

	static boost::shared_ptr<AuxVar1<ADDRINT, bool, -1, false>> CallsFrom;
	static boost::shared_ptr<AuxVar1<ADDRINT, bool, -1, false>> CallsTo;

	static boost::shared_ptr<AuxVar1<ADDRINT, bool, -1, false>> Enters;
	static boost::shared_ptr<AuxVar1<ADDRINT, bool, -1, false>> Returns;

	static boost::shared_ptr<AuxVar1<ADDRINT, int, -1, 0>> InFunc;
	static boost::shared_ptr<AuxVar1<ADDRINT, int, -1, 0>> NumInFunc;

	static boost::shared_ptr<AuxVar0<int, -1>> Pc;

	// current thread variable updated at each transition
	static ThreadVarPtr Tid;
};

/********************************************************************************/

class TPInFunc : public PreStateTransitionPredicate {
public:
	TPInFunc(const ADDRINT& addr, const ThreadVarPtr& tvar = ThreadVarPtr()) : PreStateTransitionPredicate(), tvar_(tvar), addr_(addr) {}
	~TPInFunc() {}

	bool EvalState(Coroutine* t = NULL) {
		safe_assert(t != NULL);

		THREADID tid = tvar_ == NULL ? t->tid() : safe_notnull(tvar_->thread())->tid();

		return AuxState::InFunc->get(addr_, tid) > 0;
	}

	static TransitionPredicatePtr create(const ADDRINT& addr, const ThreadVarPtr& tvar = ThreadVarPtr()) {
		safe_assert(addr != 0);
		TransitionPredicatePtr p(new TPInFunc(addr, tvar));
		return p;
	}

private:
	DECL_FIELD(ThreadVarPtr, tvar)
	DECL_FIELD(ADDRINT, addr)
};

/********************************************************************************/

TransitionPredicatePtr operator == (const ThreadVarPtr& t1, const ThreadVarPtr& t2);
TransitionPredicatePtr operator != (const ThreadVarPtr& t1, const ThreadVarPtr& t2);
TransitionPredicatePtr operator || (const ThreadVarPtr& t1, const ThreadVarPtr& t2);
TransitionPredicatePtr operator ! (const ThreadVarPtr& t1);

} // end namespace


#endif /* TRANSPRED_H_ */
