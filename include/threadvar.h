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
	explicit FuncVar(void* addr) : addr_(PTR2ADDRINT(safe_notnull(addr))) {}
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

	std::string ToString();

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

bool operator==(const ThreadVar& tx, const ThreadVar& yx);
bool operator!=(const ThreadVar& tx, const ThreadVar& ty);
bool operator<(const ThreadVar& a, const ThreadVar& b);
typedef boost::shared_ptr<ThreadVar> ThreadVarPtr;

struct ThreadVarPtr_compare {
    bool operator()(const ThreadVarPtr& tx, const ThreadVarPtr& ty) const;
};

typedef std::set<ThreadVarPtr, ThreadVarPtr_compare> ThreadVarPtrSet;

/********************************************************************************/

} // end namespace


#endif /* THREADVAR_H_ */
