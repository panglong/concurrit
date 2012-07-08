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

/********************************************************************************/

BacktrackException* __backtrack_exception__ = new BacktrackException();
//TerminateSearchException* __terminate_search_exception__ = new TerminateSearchException();
//ConcurritException*    __concurrit_exception__ = new ConcurritException(NULL, NULL, "", NULL);

/********************************************************************************/

ConcurritException::ConcurritException(std::exception* e, Coroutine* owner /*= NULL*/, const std::string& where /*= ""*/, ConcurritException* next /*= NULL*/) throw() : std::exception() {
	safe_assert(e != NULL);
	where_ = where;
	cause_ = e;
	owner_ = owner;
	next_ = next;
}

ConcurritException::~ConcurritException() throw() {
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

const char* ConcurritException::what() const throw() {
	std::string s = format_string("Exception in: %s\n"
								  "Cause: %s\n"
								  "%s",
								  where_.c_str(),
								  cause_->what(),
								  (next_ != NULL ? next_->what() : ""));
	return s.c_str();
}

bool ConcurritException::contains(std::exception* e) {
	ConcurritException* ce = this;
	while(ce != NULL) {
		if(ce->cause_ == e) {
			return true;
		}
		ce = ce->next_;
	}
	return false;
}


// also assume exception
BacktrackException* ConcurritException::get_backtrack() {
	ConcurritException* ce = this;
	while(ce != NULL) {
		if(INSTANCEOF(ce->cause_, BacktrackException*)) {
			return static_cast<BacktrackException*>(ce->cause_);
		}
		ce = ce->next_;
	}
	return NULL;
}

AssertionViolationException* ConcurritException::get_assertion_violation() {
	ConcurritException* ce = this;
	while(ce != NULL) {
		if(INSTANCEOF(ce->cause_, AssertionViolationException*)) {
			return static_cast<AssertionViolationException*>(ce->cause_);
		}
		ce = ce->next_;
	}
	return NULL;
}

InternalException* ConcurritException::get_internal() {
	ConcurritException* ce = this;
	while(ce != NULL) {
		if(INSTANCEOF(ce->cause_, InternalException*)) {
			return static_cast<InternalException*>(ce->cause_);
		}
		ce = ce->next_;
	}
	return NULL;
}

std::exception* ConcurritException::get_non_backtrack() {
	ConcurritException* ce = this;
	while(ce != NULL) {
		if(!INSTANCEOF(ce->cause_, BacktrackException*)) {
			return ce->cause_;
		}
		ce = ce->next_;
	}
	return NULL;
}


} // end namespace
