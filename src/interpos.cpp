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

volatile bool PthreadOriginals::_initialized = false;
int (* volatile PthreadOriginals::_pthread_create) (pthread_t *, const pthread_attr_t *, void *(*)(void *), void *) = NULL;
int (* volatile PthreadOriginals::_pthread_join) (pthread_t, void **) = NULL;
void (* volatile PthreadOriginals::_pthread_exit) (void *) = NULL;
int (* volatile PthreadOriginals::_pthread_cancel) (pthread_t) = NULL;

/********************************************************************************/

#define init_original(f, type) \
	{ \
    	_##f = (type) dlsym(RTLD_NEXT, #f); \
		if(_##f == NULL) { \
			fprintf(stderr, "originals RTLD_NEXT init of %s failed, using the RTLD_DEFAULT init.\n", #f); \
			_##f = (type) dlsym(RTLD_DEFAULT, #f); \
		} \
		CHECK(_##f != NULL) << "originals " << #f << " init failed!"; \
    }\

/********************************************************************************/

void PthreadOriginals::initialize() {
	safe_assert(!_initialized);

	init_original(pthread_create, int (* volatile) (pthread_t *, const pthread_attr_t *, void *(*)(void *), void *));

	init_original(pthread_join, int (* volatile) (pthread_t, void **));

	init_original(pthread_exit, void (* volatile) (void *));

	init_original(pthread_cancel, int (* volatile) (pthread_t));

	_initialized = true;
}

/********************************************************************************/

int PthreadOriginals::pthread_create(pthread_t * param0, const pthread_attr_t * param1, void *(* param2)(void *), void * param3) {
	CHECK(_pthread_create != NULL) << "ERROR: original pthread_create is NULL\n";

	return _pthread_create(param0, param1, param2, param3);
}

int PthreadOriginals::pthread_join(pthread_t param0, void ** param1) {
	CHECK(_pthread_join != NULL) << "ERROR: original pthread_join is NULL\n";

	return _pthread_join(param0, param1);
}

void PthreadOriginals::pthread_exit(void * param0) {
	CHECK(_pthread_exit != NULL) << "ERROR: original pthread_exit is NULL\n";

	_pthread_exit(param0);
}

int PthreadOriginals::pthread_cancel(pthread_t thread) {
	CHECK(_pthread_cancel != NULL) << "ERROR: original pthread_cancel is NULL\n";

	return _pthread_cancel(thread);
}

/********************************************************************************/

// we ensure that pthread calls that we interpose run atomically
static Mutex concurrit_pthread_lock;

int pthread_create(pthread_t* thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg) {
	ScopeMutex m(&concurrit_pthread_lock);
	safe_assert(PthreadHandler::Current != NULL);
	return PthreadHandler::Current->pthread_create(thread, attr, start_routine, arg);
}
int pthread_join(pthread_t thread, void ** value_ptr) {
	ScopeMutex m(&concurrit_pthread_lock);
	safe_assert(PthreadHandler::Current != NULL);
	return PthreadHandler::Current->pthread_join(thread, value_ptr);
}
void pthread_exit(void * param0) {
	ScopeMutex m(&concurrit_pthread_lock);
	safe_assert(PthreadHandler::Current != NULL);
	return PthreadHandler::Current->pthread_exit(param0);
}
int pthread_cancel(pthread_t thread) {
	ScopeMutex m(&concurrit_pthread_lock);
	safe_assert(PthreadHandler::Current != NULL);
	return PthreadHandler::Current->pthread_cancel(thread);
}

/********************************************************************************/

static PthreadHandler DefaultPthreadHandler;
PthreadHandler* PthreadHandler::Current = &DefaultPthreadHandler;

/********************************************************************************/

int PthreadHandler::pthread_create(pthread_t* thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg) {
	safe_assert(PthreadOriginals::is_initialized() && PthreadOriginals::_pthread_create != NULL);
	return PthreadOriginals::pthread_create(thread, attr, start_routine, arg);
}

int PthreadHandler::pthread_join(pthread_t thread, void ** value_ptr) {
	safe_assert(PthreadOriginals::is_initialized() && PthreadOriginals::_pthread_join != NULL);
	return PthreadOriginals::pthread_join(thread, value_ptr);
}

void PthreadHandler::pthread_exit(void * param0) {
	safe_assert(PthreadOriginals::is_initialized() && PthreadOriginals::_pthread_exit != NULL);
	PthreadOriginals::pthread_exit(param0);
}

int PthreadHandler::pthread_cancel(pthread_t thread) {
	safe_assert(PthreadOriginals::is_initialized() && PthreadOriginals::_pthread_cancel != NULL);
	return PthreadOriginals::pthread_cancel(thread);
}

/********************************************************************************/

int ConcurritPthreadHandler::pthread_create(pthread_t* thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg) {
	if(!Concurrit::IsInitialized()) {
		return PthreadHandler::pthread_create(thread, attr, start_routine, arg);
	}

	MYLOG(2) << "Creating new thread via interpositioned pthread_create.";

	Scenario* scenario = safe_notnull(Scenario::Current());
	ThreadVarPtr var = scenario->CreatePThread(start_routine, arg, thread, attr);
	safe_assert(var != NULL && !var->is_empty());

	// update counter
	scenario->counter("Num Threads").increment();

	if(__pthread_errno__ != PTH_SUCCESS) {
		safe_fail("Create error: %s\n", PTHResultToString(__pthread_errno__));
	}
	safe_assert(*thread != PTH_INVALID_THREAD);

	return __pthread_errno__;
}

int ConcurritPthreadHandler::pthread_join(pthread_t thread, void ** value_ptr) {
	if(!Concurrit::IsInitialized()) {
		return PthreadHandler::pthread_join(thread, value_ptr);
	}

	MYLOG(2) << "Joining thread via interpositioned pthread_join.";

	Scenario* scenario = safe_notnull(Scenario::Current());
	Coroutine* co = safe_notnull(scenario->group())->GetMember(thread);
	scenario->JoinPThread(co, value_ptr);

	return __pthread_errno__;
}

void ConcurritPthreadHandler::pthread_exit(void * param0) {
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

int ConcurritPthreadHandler::pthread_cancel(pthread_t thread) {
	if(!Concurrit::IsInitialized()) {
		return PthreadHandler::pthread_cancel(thread);
	}

	MYLOG(2) << "Exiting thread via interpositioned pthread_cancel.";

	safe_fail("pthread_cancel is not supported yet!!!");

	return -1;
}

/********************************************************************************/


} // end namespace


