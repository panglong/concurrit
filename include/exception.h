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
					   UNKNOWN = 9};

class BacktrackException : public std::exception {
public:
	BacktrackException(BacktrackReason reason = UNKNOWN) throw() : std::exception(), reason_(reason) {

	}

	virtual ~BacktrackException() throw() {

	}

	virtual const char* what() const throw()
	{
		std::stringstream s;
		s << "Backtrack due to ";
#define print_enum(e) case e: s << #e; break;
		switch(reason_) {
			print_enum(SEARCH_ENDS)
			print_enum(SUCCESS)
			print_enum(SPEC_UNSATISFIED)
			print_enum(THREADS_ALLENDED)
			print_enum(TREENODE_COVERED)
			print_enum(ASSUME_FAILS)
			print_enum(TIMEOUT)
			print_enum(EXCEPTION)
			print_enum(UNKNOWN)
		default:
			bool UnknownBacktrackReason = false;
			safe_assert(UnknownBacktrackReason);
			break;
		}
#undef print_enum
		return s.str().c_str();
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

	~AssertionViolationException() throw() {
		delete loc_;
	}

	virtual const char* what() const throw()
	{
		std::stringstream s(std::stringstream::out);
		s << "Assertion violated: " << condition_ << "\n";
		s << "Source location: " << (loc_ != NULL ? loc_->ToString() : "<unknown>") << "\n";
		return s.str().c_str();
	}
private:
	DECL_FIELD(std::string, condition)
	DECL_FIELD(SourceLocation*, loc)
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

	virtual const char* what() const throw()
	{
		std::stringstream s(std::stringstream::out);
		s << "Assumption violated: " << condition_ << "\n";
		s << "Source location: " << (loc_ != NULL ? loc_->ToString() : "<unknown>") << "\n";
		return s.str().c_str();
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

	virtual const char* what() const throw()
	{
		std::stringstream s(std::stringstream::out);
		s << "Internal exception: " << message_ << "\n";
		s << "Source location: " << (loc_ != NULL ? loc_->ToString() : "<unknown>") << "\n";
		return s.str().c_str();
	}
private:
	DECL_FIELD(std::string, message)
	DECL_FIELD(SourceLocation*, loc)
};

/********************************************************************************/

// general counit exception
class ConcurritException : public std::exception {
public:
	ConcurritException(std::exception* e, Coroutine* owner = NULL, const std::string& where = "", ConcurritException* next = NULL) throw() : std::exception() {
		safe_assert(e != NULL);
		where_ = where;
		cause_ = e;
		owner_ = owner;
		next_ = next;
	}

	~ConcurritException() throw() {
		// old comment: do not delete the cause!
		BacktrackException* be = ASINSTANCEOF(cause_, BacktrackException*);
		if(be != NULL) {
			delete be;
		}

		// but can delete the next
		if(next_ != NULL) {
			delete next_;
		}
	}

	virtual const char* what() const throw()
	{
		std::stringstream s(std::stringstream::out);
		s << "Exception in " << where_ << "\n";
		s << "Cause: " << cause_->what() << "\n";
		return s.str().c_str();
	}

	// also assume exception
	bool is_backtrack() {
		ConcurritException* ce = this;
		while(ce != NULL) {
			if(INSTANCEOF(cause_, BacktrackException*)) {
				return true;
			}
			ce = ce->next_;
		}
		return false;
	}

	bool is_assertion_violation() {
		ConcurritException* ce = this;
		while(ce != NULL) {
			if(INSTANCEOF(cause_, AssertionViolationException*)) {
				return true;
			}
			ce = ce->next_;
		}
		return false;
	}

	bool is_internal() {
		ConcurritException* ce = this;
		while(ce != NULL) {
			if(INSTANCEOF(cause_, InternalException*)) {
				return true;
			}
			ce = ce->next_;
		}
		return false;
	}

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
	BacktrackException* e = new BacktrackException(reason);
	e->set_reason(reason);
	return e;
}

//inline TerminateSearchException* GetTerminateSearchException() {
//	return CHECK_NOTNULL(__terminate_search_exception__);
//}

inline void TRIGGER_BACKTRACK(BacktrackReason reason = UNKNOWN, bool wrap = false) {
	BacktrackException* e = GetBacktrackException(reason);
	VLOG(2) << "TRIGGER_BACKTRACK: " << e->what();
	if(wrap) {
		throw new ConcurritException(e);
	} else {
		throw e;
	}
}

inline void TRIGGER_TERMINATE_SEARCH() {
	TRIGGER_BACKTRACK(SEARCH_ENDS);
}

//inline std::exception* WRAP_EXCEPTION(const std::string& m, std::exception* e) {
//	VLOG(2) << "WRAPPED_EXCEPTION: " << (m) << " : " << (e)->what();
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
	VLOG(2) << "TRIGGER_INTERNAL_EXCEPTION: " << (m);
	throw new InternalException(m, RECORD_SRCLOC());
}


//#define TRIGGER_BACKTRACK()				throw new BacktrackException()
//#define TRIGGER_WRAPPED_EXCEPTION(m, e)	throw new ConcurritException(m, e)
//#define TRIGGER_WRAPPED_BACKTRACK(m)		TRIGGER_WRAPPED_EXCEPTION(m, new BacktrackException())


} // end namespace

#endif /* EXCEPTION_H_ */
