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

PTHREADORIGINALS_STATIC_FIELD_DEFINITIONS

/********************************************************************************/

PTHREAD_FUNCTION_DEFINITIONS

/********************************************************************************/

class ConcurritPthreadHandler : public PthreadHandler {
public:
	ConcurritPthreadHandler() : PthreadHandler() {}
	~ConcurritPthreadHandler() {}

	int pthread_create(pthread_t* thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg) {
		if(!Concurrit::IsInitialized()) {
			return PthreadHandler::pthread_create(thread, attr, start_routine, arg);
		}

		MYLOG(2) << "Creating new thread via interpositioned pthread_create.";

		Scenario* scenario = safe_notnull(Scenario::Current());
		ThreadVarPtr var = scenario->CreatePThread(start_routine, arg, thread, attr);
		safe_assert(var != NULL && !var->is_empty());

		if(__pthread_errno__ != PTH_SUCCESS) {
			safe_fail("Create error: %s\n", PTHResultToString(__pthread_errno__));
		}
		safe_assert(*thread != PTH_INVALID_THREAD);

		return __pthread_errno__;
	}

	int pthread_join(pthread_t thread, void ** value_ptr) {
		if(!Concurrit::IsInitialized()) {
			return PthreadHandler::pthread_join(thread, value_ptr);
		}

		MYLOG(2) << "Joining thread via interpositioned pthread_join.";

		Scenario* scenario = safe_notnull(Scenario::Current());
		Coroutine* co = safe_notnull(scenario->group())->GetMember(thread);
		scenario->JoinPThread(co, value_ptr);

		return __pthread_errno__;
	}

	void pthread_exit(void * param0) {
		if(!Concurrit::IsInitialized()) {
			return PthreadHandler::pthread_exit(param0);
		}

		MYLOG(2) << "Exiting thread via interpositioned pthread_exit.";

		Coroutine* co = safe_notnull(Coroutine::Current());

		// capture the return value
		co->set_return_value(param0);

		// trigger backtrackexception
		TRIGGER_BACKTRACK(PTH_EXIT);
	}

	int pthread_cancel(pthread_t thread) {
		if(!Concurrit::IsInitialized()) {
			return PthreadHandler::pthread_cancel(thread);
		}

		MYLOG(2) << "Exiting thread via interpositioned pthread_cancel.";

		CHECK(false) << "pthread_cancel is not supported yet!!!";

		return -1;
	}
};

/********************************************************************************/

PTHREADHANDLER_CURRENT_DEFINITION(new ConcurritPthreadHandler())

/********************************************************************************/


} // end namespace


