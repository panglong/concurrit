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
#include "threadvar.h"
#include "thread.h"
#include <boost/shared_ptr.hpp>

namespace concurrit {

/********************************************************************************/

//enum TPVALUE { TPFALSE = 0, TPTRUE = 1, TPUNKNOWN = 2, TPINVALID = -1 };
//TPVALUE TPNOT(TPVALUE v);
//TPVALUE TPAND(TPVALUE v1, TPVALUE v2);
//TPVALUE TPOR(TPVALUE v1, TPVALUE v2);

/********************************************************************************/
class TransitionPredicate;
typedef boost::shared_ptr<TransitionPredicate> TransitionPredicatePtr;

class TransitionPredicate {
public:
	TransitionPredicate() {}
	virtual ~TransitionPredicate() {}

	virtual bool EvalState(Coroutine* t = NULL) = 0;

	bool EvalState(const ThreadVarPtr& var) {
		return EvalState(var->thread());
	}

	static TransitionPredicatePtr True();
	static TransitionPredicatePtr False();

	virtual std::string ToString() {
		return ""; // TODO(elmas): implement
	}

//	// evaluate immediatelly
//	operator bool () {
//		return EvalState(NULL);
//	}

	bool operator () (const ThreadVarPtr& t) {
		return EvalState(t == NULL ? NULL : t->thread());
	}
};

TransitionPredicatePtr operator ! (const TransitionPredicatePtr& pred);
TransitionPredicatePtr operator && (const TransitionPredicatePtr& pred1, const TransitionPredicatePtr& pred2);
TransitionPredicatePtr operator || (const TransitionPredicatePtr& pred1, const TransitionPredicatePtr& pred2);

//TransitionPredicatePtr operator && (const TransitionPredicatePtr& pred, const bool& b);
//TransitionPredicatePtr operator || (const TransitionPredicatePtr& pred, const bool& b);

/********************************************************************************/

class TrueTransitionPredicate : public TransitionPredicate {
public:
	TrueTransitionPredicate() : TransitionPredicate() {}
	~TrueTransitionPredicate() {}
	bool EvalState(Coroutine* t = NULL) { return true; }
};

class FalseTransitionPredicate : public TransitionPredicate {
public:
	FalseTransitionPredicate() : TransitionPredicate() {}
	~FalseTransitionPredicate() {}
	bool EvalState(Coroutine* t = NULL) { return false; }
};

/********************************************************************************/

class NotTransitionPredicate : public TransitionPredicate {
public:
	explicit NotTransitionPredicate(TransitionPredicatePtr pred) : TransitionPredicate(), pred_(pred) {}
	NotTransitionPredicate(TransitionPredicate* pred) : TransitionPredicate(), pred_(TransitionPredicatePtr(pred)) {}
	~NotTransitionPredicate() {}
	bool EvalState(Coroutine* t = NULL) { return !(pred_->EvalState(t)); }
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

	bool EvalState(Coroutine* t = NULL) {
		bool v = (op_ == NAryAND) ? true : false;
		for(NAryTransitionPredicate<op_>::iterator itr = begin(); itr != end(); ++itr) {
			// update current
			TransitionPredicatePtr current = (*itr);
			// update v
			v = (op_ == NAryAND) ? (v && current->EvalState(t)) : (v || current->EvalState(t));
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
typedef NAryTransitionPredicate<NAryAND> TransitionAssertions;
typedef boost::shared_ptr<TransitionConstraints> TransitionConstraintsPtr;
typedef boost::shared_ptr<TransitionAssertions> TransitionAssertionsPtr;

/********************************************************************************/
/********************************************************************************/
/********************************************************************************/

class Scenario;

// assertions
class AssertionInstaller {
public:
	AssertionInstaller(Scenario* scenario, const TransitionPredicatePtr& pred);
	~AssertionInstaller();

private:
	DECL_FIELD(Scenario*, scenario)
	DECL_FIELD(TransitionPredicatePtr, pred)
};

/********************************************************************************/

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

	bool EvalState(Coroutine* t = NULL) {
		return pred_->EvalState(t);
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

	bool EvalState(Coroutine* t = NULL) {
		if(pred_->EvalState(t)) {
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
	DECL_FIELD_REF(RWLock, rwlock)
};

#define RLOCK() ScopeRLock __sm__(&rwlock_)
#define WLOCK() ScopeWLock __sm__(&rwlock_)

/********************************************************************************/

template<typename T, T undef_value_>
class AuxVar0 : public AuxVar {
	typedef std::map<THREADID, T> M;
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

		typename M::iterator itr = map_.find(t);
		if(itr == map_.end()) {
			return undef_value_;
		}
		T value = itr->second;
		safe_assert(value != undef_value_);
		return value;
	}

	virtual void set(const T& value, THREADID t = -1) {
		WLOCK();

		if(value == undef_value_) {
			typename M::iterator itr = map_.find(t);
			if(itr != map_.end()) map_.erase(itr);
		} else {
			map_[t] = value;
		}
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
class AuxVar0Pre : public TransitionPredicate {
	typedef AuxVar0<T,undef_value_> AuxVar0Type;
	typedef boost::shared_ptr<AuxVar0Type> AuxVar0Ptr;
	typedef StaticAuxVar0<T,undef_value_> StaticAuxVar0Type;
public:
	AuxVar0Pre(const AuxVar0Ptr& var1, const AuxVar0Ptr& var2, const ThreadVarPtr& tvar) : TransitionPredicate(), var1_(var1), var2_(var2), tvar_(tvar) {}
	~AuxVar0Pre(){}

	static TransitionPredicatePtr create (const AuxVar0Ptr& var1, const AuxVar0Ptr& var2, const ThreadVarPtr& tvar) {
		TransitionPredicatePtr p(new AuxVar0Pre<T,undef_value_>(var1, var2, tvar));
		return p;
	}

	bool EvalState(Coroutine* t = NULL) {
		safe_assert(t != NULL);

		THREADID tid = tvar_ == NULL ? t->tid() : safe_notnull(tvar_->thread())->tid();

		safe_assert(var1_ != NULL);
		safe_assert(INSTANCEOF(var1_.get(), StaticAuxVar0Type*));

		{ ScopeRLock lock_var1(var1_->rwlock());

			if(var2_ == NULL) {
				return var1_->isset(tid);
			} else
			if(!var2_->isset(tid)) {
				// if var1 is set for tid, assign this value to var2
				if(var1_->isset(tid)) {
					var2_->set(var1_->get(tid), tid);
					return true;
				} else {
					return false;
				}
			}
			if(var1_->isset(tid)) {
				return var1_->get(tid) == var2_->get(tid);
			}
			return false;
		}
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
	typedef std::map<K, T> MM;
	typedef std::map<THREADID, MM> M;
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
	TransitionPredicatePtr TP5(const AuxVarPtr& var, const K& key, const AuxValuePtr& value, const ThreadVarPtr& tvar);
	TransitionPredicatePtr TP4(const AuxVarPtr& var, const AuxKeyPtr& key, const AuxValuePtr& value, const ThreadVarPtr& tvar);

	//================================================

	// get first key
	K get_first_key(THREADID t = -1) {
		RLOCK();

		typename M::iterator itr = map_.find(t);
		if(itr != map_.end()) {
			if(itr->second.empty()) {
				return undef_key_;
			}
			K key = itr->second.begin()->first;
			safe_assert(key != undef_key_);
			return key;
		}
		unreachable(); // call isset(t) first!
		return undef_key_;
	}

	// get first value
	T get_first_value(THREADID t = -1) {
		RLOCK();

		typename M::iterator itr = map_.find(t);
		if(itr != map_.end()) {
			if(itr->second.empty()) {
				return undef_key_;
			}
			T value = itr->second.begin()->second;
			safe_assert(value != undef_value_);
			return value;
		}
		unreachable(); // call isset(t) first!
		return undef_value_;
	}

	T get(const K& key, THREADID t = -1) {
		RLOCK();

		safe_assert(key != undef_key_);
		typename M::iterator itr = map_.find(t);
		if(itr == map_.end()) {
			return undef_value_;
		}
		typename MM::iterator itr2 = itr->second.find(key);
		if(itr2 == itr->second.end()) {
			return undef_value_;
		}
		T value = itr2->second;
		safe_assert(value != undef_value_);
		return value;
	}

	void set(const K& key, const T& value, THREADID t = -1) {
		WLOCK();

		safe_assert(key != undef_key_);
		typename M::iterator itr = map_.find(t);
		if(itr == map_.end()) {
			map_[t] = MM();
		}

		MM& mm = map_[t];
		if(value == undef_value_) {
			typename MM::iterator itr2 = mm.find(key);
			if(itr2 != mm.end()) mm.erase(itr2);
		} else {
			mm[key] = value;
		}
	}

	bool isset(const K& key, THREADID t = -1) {
		RLOCK();

		safe_assert(key != undef_key_);
		typename M::iterator itr = map_.find(t);
		if(itr == map_.end()) {
			return false;
		}
		typename MM::iterator itr2 = itr->second.find(key);

		bool ret = itr2 != itr->second.end();
		safe_assert(!ret || (itr2->second != undef_value_));
		return ret;
	}

	bool isset(THREADID t = -1) {
		RLOCK();

		typename M::iterator itr = map_.find(t);
		if(itr == map_.end()) {
			return false;
		}
		return !itr->second.empty();
	}

	void reset(THREADID t = -1) {
		WLOCK();

		typename M::iterator itr = map_.find(t);
		if(itr != map_.end()) {
			itr->second.clear();
		}
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
class AuxVar1Pre : public TransitionPredicate {
	typedef AuxVar1<K,T,undef_key_,undef_value_> AuxVarType;
	typedef StaticAuxVar1<K,T,undef_key_,undef_value_> StaticAuxVarType;
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
	: TransitionPredicate(), var_(var), key_(key), value_(value), tvar_(tvar) {}
	~AuxVar1Pre(){}

	static TransitionPredicatePtr create (const AuxVarPtr& var, const AuxKeyPtr& key, const AuxValuePtr& value, const ThreadVarPtr& tvar) {
		TransitionPredicatePtr p(new AuxVar1Pre<K,T,undef_key_,undef_value_>(var, key, value, tvar));
		return p;
	}

	bool EvalState(Coroutine* t = NULL) {
		safe_assert(t != NULL);

		THREADID tid = tvar_ == NULL ? t->tid() : safe_notnull(tvar_->thread())->tid();

		safe_assert(var_ != NULL);
		safe_assert(INSTANCEOF(var_.get(), StaticAuxVarType*));

		{ ScopeRLock lock_var(var_->rwlock());

			if(key_ == NULL) {
				safe_assert(value_ == NULL);
				return var_->isset(tid);
			} else if(value_ == NULL) {
				if(!key_->isset(tid)) {
					// if var is set for tid, assign this value to key
					if(var_->isset(tid)) {
						// set first key
						key_->set(var_->get_first_key(tid), tid);
						return true;
					} else {
						return false;
					}
				}
				return var_->isset(key_->get(tid), tid);
			} else {
				safe_assert(key_->isset(tid));
				if(!value_->isset(tid)) {
					// if var is set for tid, assign this value to value
					if(var_->isset(key_->get(tid), tid)) {
						// set first value
						value_->set(var_->get(key_->get(tid), tid), tid);
						return true;
					} else {
						return false;
					}
				}
				if(var_->isset(key_->get(tid), tid)) {
					return var_->get(key_->get(tid), tid) == value_->get(tid);
				} else {
					return false;
				}
			}
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
TransitionPredicatePtr AuxVar1<K,T,undef_key_,undef_value_>::TP5(const AuxVarPtr& var, const K& key, const AuxValuePtr& value, const ThreadVarPtr& tvar) {
	AuxKeyConstPtr p(new AuxKeyConstType(key));
	return this->TP4(var, p, value, tvar);
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

	static boost::shared_ptr<AuxVar1<ADDRINT, uint32_t, 0, 0>> Reads;
	static boost::shared_ptr<AuxVar1<ADDRINT, uint32_t, 0, 0>> Writes;

	static boost::shared_ptr<AuxVar1<ADDRINT, bool, 0, false>> CallsFrom;
	static boost::shared_ptr<AuxVar1<ADDRINT, bool, 0, false>> CallsTo;

	static boost::shared_ptr<AuxVar1<ADDRINT, bool, 0, false>> Enters;
	static boost::shared_ptr<AuxVar1<ADDRINT, bool, 0, false>> Returns;

	static boost::shared_ptr<AuxVar1<ADDRINT, int, 0, 0>> InFunc;
	static boost::shared_ptr<AuxVar1<ADDRINT, int, 0, 0>> NumInFunc;

	static boost::shared_ptr<AuxVar1<ADDRINT, ADDRINT, 0, 0>> Arg0;
	static boost::shared_ptr<AuxVar1<ADDRINT, ADDRINT, 0, 0>> Arg1;

	static boost::shared_ptr<AuxVar1<ADDRINT, ADDRINT, 0, 0>> RetVal;

	static boost::shared_ptr<AuxVar0<int, -1>> Pc;
	static boost::shared_ptr<AuxVar0<bool, false>> AtPc;

	// current thread variable updated at each transition
	static ThreadVarPtr Tid;
};

/********************************************************************************/

class TPInFunc : public TransitionPredicate {
public:
	TPInFunc(const ADDRINT& addr, const ThreadVarPtr& tvar = ThreadVarPtr()) : TransitionPredicate(), tvar_(tvar), addr_(addr) {}
	~TPInFunc() {}

	bool EvalState(Coroutine* t = NULL);

	static TransitionPredicatePtr create(const ADDRINT& addr, const ThreadVarPtr& tvar = ThreadVarPtr());

private:
	DECL_FIELD(ThreadVarPtr, tvar)
	DECL_FIELD(ADDRINT, addr)
};

/********************************************************************************/

TransitionPredicatePtr operator == (const ThreadVarPtr& t1, const ThreadVarPtr& t2);
TransitionPredicatePtr operator != (const ThreadVarPtr& t1, const ThreadVarPtr& t2);

/********************************************************************************/

// Thread expressions

class ThreadExpr : public TransitionPredicate {
public:
	ThreadExpr() {}
	virtual ~ThreadExpr() {}

	virtual bool EvalState(Coroutine* t = NULL) = 0;

};

typedef boost::shared_ptr<ThreadExpr> ThreadExprPtr;

/********************************************************************************/

class AnyThreadExpr : public ThreadExpr {
public:
	AnyThreadExpr() {}
	~AnyThreadExpr() {}

	bool EvalState(Coroutine* t = NULL) { return true; }

	static ThreadExprPtr create() {
		ThreadExprPtr p(new AnyThreadExpr());
		return p;
	}
};

/********************************************************************************/

class ThreadVarExpr : public ThreadExpr {
public:
	ThreadVarExpr(const ThreadVarPtr& t) : var_(t) {}
	~ThreadVarExpr() {}

	bool EvalState(Coroutine* t = NULL) {
		TransitionPredicatePtr p = (AuxState::Tid == var_);
		return p->EvalState(t);
	}

	static ThreadExprPtr create(const ThreadVarPtr& t) {
		ThreadExprPtr p(new ThreadVarExpr(t));
		return p;
	}
private:
	DECL_FIELD(ThreadVarPtr, var)
};

/********************************************************************************/

class NegThreadVarExpr : public ThreadExpr {
public:
	NegThreadVarExpr(const ThreadVarPtr& t) : var_(t) {}
	~NegThreadVarExpr() {}

	bool EvalState(Coroutine* t = NULL) {
		TransitionPredicatePtr p = (AuxState::Tid != var_);
		return p->EvalState(t);
	}

	static ThreadExprPtr create(const ThreadVarPtr& t) {
		ThreadExprPtr p(new NegThreadVarExpr(t));
		return p;
	}
private:
	DECL_FIELD(ThreadVarPtr, var)
};

/********************************************************************************/

class PlusThreadExpr : public ThreadExpr {
public:
	PlusThreadExpr(const ThreadExprPtr& e, const ThreadVarPtr& t) : expr_(e), var_(t) {}
	~PlusThreadExpr() {}

	bool EvalState(Coroutine* t = NULL) {
		if(expr_->EvalState(t)) {
			return true;
		}

		TransitionPredicatePtr p = ThreadVarExpr::create(var_);
		return p->EvalState(t);
	}

	static ThreadExprPtr create(const ThreadExprPtr& e, const ThreadVarPtr& t) {
		ThreadExprPtr p(new PlusThreadExpr(e, t));
		return p;
	}
private:
	DECL_FIELD(ThreadExprPtr, expr)
	DECL_FIELD(ThreadVarPtr, var)
};

/********************************************************************************/

class MinusThreadExpr : public ThreadExpr {
public:
	MinusThreadExpr(const ThreadExprPtr& e, const ThreadVarPtr& t) : expr_(e), var_(t) {}
	~MinusThreadExpr() {}

	bool EvalState(Coroutine* t = NULL) {
		if(!expr_->EvalState(t)) {
			return false;
		}

		TransitionPredicatePtr p = NegThreadVarExpr::create(var_);
		return p->EvalState(t);
	}

	static ThreadExprPtr create(const ThreadExprPtr& e, const ThreadVarPtr& t) {
		ThreadExprPtr p(new MinusThreadExpr(e, t));
		return p;
	}
private:
	DECL_FIELD(ThreadExprPtr, expr)
	DECL_FIELD(ThreadVarPtr, var)
};

/********************************************************************************/

ThreadExprPtr operator + (const ThreadExprPtr& e, const ThreadVarPtr& t);
ThreadExprPtr operator - (const ThreadExprPtr& e, const ThreadVarPtr& t);

ThreadExprPtr operator ! (const ThreadVarPtr& t);

/********************************************************************************/

} // end namespace


#endif /* TRANSPRED_H_ */
