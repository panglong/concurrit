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

#ifndef EXCEPTION_H_
#define EXCEPTION_H_

#include "common.h"
#include "sharedaccess.h"

namespace concurrit {

class Coroutine;

/*
 * Exceptions
 */

enum BacktrackReason { SEARCH_ENDS = 1,
					   SUCCESS = 2,
					   SPEC_UNSATISFIED = 3,
					   THREADS_ALLENDED = 4,
					   TREENODE_COVERED = 5,
					   ASSUME_FAILS = 6,
					   TIMEOUT = 7,
					   EXCEPTION = 8,
					   PTH_EXIT = 9,
					   REPLAY_FAILS = 10,
					   UNKNOWN = -1};

class BacktrackException : public std::exception {
public:
	BacktrackException(BacktrackReason reason = UNKNOWN) throw() : std::exception(), reason_(reason) {}

	virtual ~BacktrackException() throw() {}

	virtual const char* what() const throw() {
		return (std::string("Backtrack due to ") + ReasonToString(reason_)).c_str();
	}

	static std::string ReasonToString(BacktrackReason reason) {
#define print_enum(e) case e: return std::string(#e);
		switch(reason) {
			print_enum(SEARCH_ENDS)
			print_enum(SUCCESS)
			print_enum(SPEC_UNSATISFIED)
			print_enum(THREADS_ALLENDED)
			print_enum(TREENODE_COVERED)
			print_enum(ASSUME_FAILS)
			print_enum(TIMEOUT)
			print_enum(EXCEPTION)
			print_enum(PTH_EXIT)
			print_enum(REPLAY_FAILS)
			print_enum(UNKNOWN)
		default:
			safe_fail("UnknownBacktrackReason");
			break;
		}
#undef print_enum
		return std::string();
	}
private:
	DECL_FIELD(BacktrackReason, reason)
};

/********************************************************************************/

//class TerminateSearchException : public BacktrackException {
//public:
//	TerminateSearchException() throw() : BacktrackException() {
//
//	}
//
//	virtual ~TerminateSearchException() throw() {
//
//	}
//
//	virtual const char* what() const throw()
//	{
//		return "Terminate Search";
//	}
//private:
//};

/********************************************************************************/

class AssertionViolationException : public std::exception {
public:
	AssertionViolationException(const char* cond, SourceLocation* loc) throw()
			: std::exception() {
		condition_ = cond == NULL ? "Unknown" : std::string(cond);
		loc_ = loc;
	}

	virtual ~AssertionViolationException() throw() {
		if(loc_ != NULL) delete loc_;
	}

	virtual const char* what() const throw() {
		std::string s = format_string("Assertion violated: %s\n"
					  	  	  	  	  "Source location: %s\n",
									  condition_.c_str(),
									  SourceLocation::ToString(loc_).c_str());
		return s.c_str();
	}
private:
	DECL_FIELD(std::string, condition)
	DECL_FIELD(SourceLocation*, loc)
};

/********************************************************************************/

class DeadlockException : public AssertionViolationException {
public:
	DeadlockException() throw()
			: AssertionViolationException("DEADLOCK", NULL) {}

	~DeadlockException() throw() {}
};

/********************************************************************************/

class AssumptionViolationException : public BacktrackException {
public:
	AssumptionViolationException(const char* cond, SourceLocation* loc) throw()
			: BacktrackException(ASSUME_FAILS) {
		condition_ = cond == NULL ? "Unknown" : std::string(cond);
		loc_ = loc;
	}

	~AssumptionViolationException() throw() {
		delete loc_;
	}

	virtual const char* what() const throw() {
		std::string s = format_string("Assumption violated: %s\n"
									  "Source location: %s\n",
									  condition_.c_str(),
									  SourceLocation::ToString(loc_).c_str());
		return s.c_str();
	}
private:
	DECL_FIELD(std::string, condition)
	DECL_FIELD(SourceLocation*, loc)
};

/********************************************************************************/

class InternalException : public std::exception {
public:
	InternalException(const std::string& msg, SourceLocation* loc) throw() : message_(msg), loc_(loc) {}

	~InternalException() throw() {
		if(loc_ != NULL) {
			delete loc_;
		}
	}

	virtual const char* what() const throw() {
		std::string s = format_string("Internal exception: %s\n"
									  "Source location: %s\n",
									  message_.c_str(),
									  SourceLocation::ToString(loc_).c_str());
		return s.c_str();
	}

private:
	DECL_FIELD(std::string, message)
	DECL_FIELD(SourceLocation*, loc)
};

/********************************************************************************/

// general counit exception
class ConcurritException : public std::exception {
public:
	ConcurritException(std::exception* e, Coroutine* owner = NULL, const std::string& where = "", ConcurritException* next = NULL) throw();

	~ConcurritException() throw();

	virtual const char* what() const throw();

	// also assume exception
	BacktrackException* get_backtrack();

	AssertionViolationException* get_assertion_violation();

	InternalException* get_internal();

	std::exception* get_non_backtrack();

	bool contains(std::exception* e);

private:
	DECL_FIELD(std::string, where)
	DECL_FIELD(std::exception*, cause)
	DECL_FIELD(Coroutine*, owner)
	DECL_FIELD(ConcurritException*, next)
};

/********************************************************************************/

//extern BacktrackException* __backtrack_exception__;
//extern TerminateSearchException* __terminate_search_exception__;
//extern ConcurritException*    __concurrit_exception__;

inline BacktrackException* GetBacktrackException(BacktrackReason reason = UNKNOWN) {
	return new BacktrackException(reason);
}

//inline TerminateSearchException* GetTerminateSearchException() {
//	return safe_notnull(__terminate_search_exception__);
//}

inline void TRIGGER_BACKTRACK(BacktrackReason reason = UNKNOWN, bool wrap = false) {
	BacktrackException* e = GetBacktrackException(reason);
	MYLOG(2) << "TRIGGER_BACKTRACK: " << e->what();
	if(wrap) {
		throw new ConcurritException(e);
	} else {
		throw e;
	}
}

inline void TRIGGER_TERMINATE_SEARCH() {
	TRIGGER_BACKTRACK(SEARCH_ENDS);
}

inline void TRIGGER_ASSERTION_VIOLATION(const char* expr, const char* filename, const char* funcname, int line) {
	throw new AssertionViolationException(expr, new SourceLocation(filename, funcname, line));
}

inline void TRIGGER_DEADLOCK() {
	throw new DeadlockException();
}

//inline std::exception* WRAP_EXCEPTION(const std::string& m, std::exception* e) {
//	MYLOG(2) << "WRAPPED_EXCEPTION: " << (m) << " : " << (e)->what();
//	__concurrit_exception__->set_where(m);
//	__concurrit_exception__->set_cause(e);
//	__concurrit_exception__->set_next(NULL);
//	return __concurrit_exception__;
//}
//
//inline std::exception* TRIGGER_WRAPPED_EXCEPTION(const std::string& m, std::exception* e) {
//	throw WRAP_EXCEPTION(m, e);
//}

//inline void TRIGGER_WRAPPED_BACKTRACK(const std::string& m) {
//	throw new ConcurritException((__backtrack_exception__), NULL, m, NULL);
//}

inline void TRIGGER_INTERNAL_EXCEPTION(const std::string& m) {
	MYLOG(2) << "TRIGGER_INTERNAL_EXCEPTION: " << (m);
	throw new InternalException(m, RECORD_SRCLOC());
}


//#define TRIGGER_BACKTRACK()				throw new BacktrackException()
//#define TRIGGER_WRAPPED_EXCEPTION(m, e)	throw new ConcurritException(m, e)
//#define TRIGGER_WRAPPED_BACKTRACK(m)		TRIGGER_WRAPPED_EXCEPTION(m, new BacktrackException())


} // end namespace

#endif /* EXCEPTION_H_ */
