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


#ifndef THREADVAR_H_
#define THREADVAR_H_

#include "common.h"
#include <boost/shared_ptr.hpp>


namespace concurrit {

class Coroutine;

/********************************************************************************/

class FuncVar {
public:
	explicit FuncVar(const char* name, void* addr) : addr_(PTR2ADDRINT(safe_notnull(addr))) {
		MYLOG(2) << "Creating function variable for " << name << " at address " << addr;
	}
	~FuncVar() {}

	operator ADDRINT() { return addr_; }
	operator void*() { return ADDRINT2PTR(addr_); }

	void set_addr(void* addr) { set_addr(PTR2ADDRINT(addr)); }

private:
	DECL_FIELD(ADDRINT, addr)
};

/********************************************************************************/

class ThreadVar {
public:
	explicit ThreadVar(Coroutine* thread = NULL, const std::string& name = "<unknown>")
	: name_(name), thread_(thread) {
		MYLOG(1) << "Creating thread variable " << name;
	} // , id_(ThreadVar::get_id()) {}
	virtual ~ThreadVar() {}

	std::string ToString();

	inline void clear_thread() { thread_ = NULL; }
	inline bool is_empty() { return thread_ == NULL; }

	THREADID tid();

//protected:
//	static int get_id();
//
private:
	DECL_FIELD(std::string, name)
	DECL_FIELD(Coroutine*, thread)
//	DECL_FIELD_CONST(int, id)
//	DECL_STATIC_FIELD(int, next_id)
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

//bool operator==(const ThreadVar& tx, const ThreadVar& yx);
//bool operator!=(const ThreadVar& tx, const ThreadVar& ty);
//bool operator<(const ThreadVar& a, const ThreadVar& b);

typedef boost::shared_ptr<ThreadVar> ThreadVarPtr;

// thread variable assignment. do not use the standard assignment =
ThreadVarPtr& operator << (ThreadVarPtr& to, const ThreadVarPtr& from);
ThreadVarPtr& operator << (ThreadVarPtr& to, Coroutine* from);

/********************************************************************************/

struct ThreadVarPtr_compare {
    bool operator()(const ThreadVarPtr& tx, const ThreadVarPtr& ty) const;
};

class ThreadVarPtrSet : public std::set<ThreadVarPtr, ThreadVarPtr_compare> {
public:
	ThreadVarPtrSet() : std::set<ThreadVarPtr, ThreadVarPtr_compare>() {}
	~ThreadVarPtrSet() {}

	ThreadVarPtr FindByThread(Coroutine* thread) {
		for(iterator itr = begin(); itr != end(); ++itr) {
			ThreadVarPtr t = (*itr);
			if(t->thread() == thread) {
				return t;
			}
		}
		return ThreadVarPtr();
	}

	void Add(const ThreadVarPtr& t) {
		safe_assert(t != NULL);
		safe_assert(this->find(t) == this->end());
		this->insert(t);
	}

	void Remove(const ThreadVarPtr& t) {
		safe_assert(t != NULL);
		safe_assert(this->find(t) != this->end());
		this->erase(t);
	}
};

/********************************************************************************/

// constructing coroutine sets from comma-separated arguments
//ThreadVarPtrSet MakeThreadVarPtrSet(ThreadVarPtr t, ...);
ThreadVarPtrSet MakeThreadVarPtrSet(ThreadVarPtr t1 = ThreadVarPtr(),
									ThreadVarPtr t2 = ThreadVarPtr(),
									ThreadVarPtr t3 = ThreadVarPtr(),
									ThreadVarPtr t4 = ThreadVarPtr(),
									ThreadVarPtr t5 = ThreadVarPtr(),
									ThreadVarPtr t6 = ThreadVarPtr(),
									ThreadVarPtr t7 = ThreadVarPtr(),
									ThreadVarPtr t8 = ThreadVarPtr()
									);

inline ThreadVarPtrSet MakeThreadVarPtrSet(const ThreadVarPtrSet& s) {
	return s;
}

/********************************************************************************/

struct ThreadVarPtr_distinct_compare {
    bool operator()(const ThreadVarPtr& tx, const ThreadVarPtr& ty) const;
};

class ThreadVarPtrDistinctSet : public std::set<ThreadVarPtr, ThreadVarPtr_distinct_compare> {
public:
	ThreadVarPtrDistinctSet() : std::set<ThreadVarPtr, ThreadVarPtr_distinct_compare>() {}
	~ThreadVarPtrDistinctSet() {}

	ThreadVarPtr FindByThread(Coroutine* thread) {
		for(iterator itr = begin(); itr != end(); ++itr) {
			ThreadVarPtr t = (*itr);
			if(t->thread() == thread) {
				return t;
			}
		}
		return ThreadVarPtr();
	}
};

/********************************************************************************/

//// thread variable scope and definitions
//
//class ThreadVarScope : public ThreadVarPtrSet {
//public:
//	ThreadVarScope() {}
//	~ThreadVarScope() {}
//
//	void Add(const ThreadVarPtr& t) {
//		safe_assert(t != NULL);
//		safe_assert(this->find(t) == this->end());
//		this->insert(t);
//	}
//
//	void Remove(const ThreadVarPtr& t) {
//		safe_assert(t != NULL);
//		safe_assert(this->find(t) != this->end());
//		this->erase(t);
//	}
//
//};

/********************************************************************************/

//class ThreadVarDef {
//public:
//	ThreadVarDef(ThreadVarScope* scope, ThreadVarPtr& t): scope_(scope), var_(t) {
//		safe_assert(scope != NULL);
//		safe_assert(t != NULL);
//		// remove existing thread info, if any
//		t->clear_thread();
//		scope_->Add(var_);
//	}
//	~ThreadVarDef(){
//		scope_->Remove(var_);
//	}
//private:
//	DECL_FIELD(ThreadVarScope*, scope)
//	DECL_FIELD(ThreadVarPtr, var)
//};

/********************************************************************************/

} // end namespace


#endif /* THREADVAR_H_ */
