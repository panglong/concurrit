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

#define init_original(f, type) \
	{ \
    	_##f = (type) dlsym(RTLD_NEXT, #f); \
		if(_##f == NULL) { \
			fprintf(stderr, "Concurrit: originals RTLD_NEXT init of %s failed, using the RTLD_DEFAULT init.\n", #f); \
			_##f = (type) dlsym(RTLD_DEFAULT, #f); \
		} \
		CHECK(_##f != NULL) << "Concurrit: originals " << #f << " init failed!"; \
    }\

/********************************************************************************/

volatile bool Originals::_initialized = false;

int (* volatile Originals::_pthread_create) (pthread_t *, const pthread_attr_t *, void *(*)(void *), void *) = NULL;
int (* volatile Originals::_pthread_join) (pthread_t, void **) = NULL;
void (* volatile Originals::_pthread_exit) (void *) = NULL;
int (* volatile Originals::_pthread_cancel) (pthread_t) = NULL;

/********************************************************************************/

void Originals::initialize() {
	safe_assert(!_initialized);

	init_original(pthread_create, int (* volatile) (pthread_t *, const pthread_attr_t *, void *(*)(void *), void *));

	init_original(pthread_join, int (* volatile) (pthread_t, void **));

	init_original(pthread_exit, void (* volatile) (void *));

	init_original(pthread_cancel, int (* volatile) (pthread_t));

	_initialized = true;
}

/********************************************************************************/

int Originals::pthread_create(pthread_t * param0, const pthread_attr_t * param1, void *(* param2)(void *), void * param3) {
    CHECK(_pthread_create != NULL)<< "Concurrit: ERROR: original pthread_create is NULL\n";

    return _pthread_create(param0, param1, param2, param3);
}

int Originals::pthread_join(pthread_t param0, void ** param1) {
	CHECK(_pthread_join != NULL)<< "Concurrit: ERROR: original pthread_join is NULL\n";

    return _pthread_join(param0, param1);
}

void Originals::pthread_exit(void * param0) {
	CHECK(_pthread_exit != NULL)<< "Concurrit: ERROR: original pthread_exit is NULL\n";

    _pthread_exit(param0);
}

int Originals::pthread_cancel(pthread_t thread) {
	CHECK(_pthread_cancel != NULL)<< "Concurrit: ERROR: original pthread_cancel is NULL\n";

    return _pthread_cancel(thread);
}

/********************************************************************************/

static int concurrit_pthread_create(pthread_t* thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg) {
	CHECK(Concurrit::IsInitialized()) << "Concurrit has not been initialized yet!";

	MYLOG(2) << "Creating new thread via interpositioned pthread_create.";

	ThreadVarPtr var = safe_notnull(Scenario::Current())->CreatePThread(start_routine, arg, thread, attr);
	safe_assert(var != NULL && !var->is_empty());

	if(__pthread_errno__ != PTH_SUCCESS) {
		safe_fail("Create error: %s\n", PTHResultToString(__pthread_errno__));
	}
	safe_assert(*thread != PTH_INVALID_THREAD);

	return __pthread_errno__;
}

static int concurrit_pthread_join(pthread_t thread, void ** value_ptr) {
	CHECK(Concurrit::IsInitialized()) << "Concurrit has not been initialized yet!";

	MYLOG(2) << "Joining thread via interpositioned pthread_join.";

	Coroutine* co = safe_notnull(safe_notnull(Scenario::Current())->group())->GetMember(thread);
	Scenario::Current()->JoinPThread(co, value_ptr);

	return __pthread_errno__;
}

static void concurrit_pthread_exit(void * param0) {
	CHECK(Concurrit::IsInitialized()) << "Concurrit has not been initialized yet!";

	MYLOG(2) << "Exiting thread via interpositioned pthread_exit.";

	Coroutine* co = safe_notnull(Coroutine::Current());

	// capture the return value
	co->set_return_value(param0);

	// trigger backtrackexception
	TRIGGER_BACKTRACK(PTH_EXIT);
}

static int concurrit_pthread_cancel(pthread_t thread) {
	CHECK(Concurrit::IsInitialized()) << "Concurrit has not been initialized yet!";

	MYLOG(2) << "Exiting thread via interpositioned pthread_cancel.";

	CHECK(false) << "pthread_cancel is not supported yet!!!";

	return -1;
}

/********************************************************************************/

int pthread_create(pthread_t* thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg) {
	if(Concurrit::IsInitialized()) {
		return concurrit_pthread_create(thread, attr, start_routine, arg);
	} else {
		safe_assert(Originals::is_initialized() && Originals::_pthread_create != NULL);
		return Originals::pthread_create(thread, attr, start_routine, arg);
	}
}

int pthread_join(pthread_t thread, void ** value_ptr) {
	if(Concurrit::IsInitialized()) {
		return concurrit_pthread_join(thread, value_ptr);
	} else {
		safe_assert(Originals::is_initialized() && Originals::_pthread_join != NULL);
		return Originals::pthread_join(thread, value_ptr);
	}
}

void pthread_exit(void * param0) {
	if(Concurrit::IsInitialized()) {
		concurrit_pthread_exit(param0);
	} else {
		safe_assert(Originals::is_initialized() && Originals::_pthread_exit != NULL);
		Originals::pthread_exit(param0);
	}
}

int pthread_cancel(pthread_t thread) {
	if(Concurrit::IsInitialized()) {
		return concurrit_pthread_cancel(thread);
	} else {
		safe_assert(Originals::is_initialized() && Originals::_pthread_cancel != NULL);
		return Originals::pthread_cancel(thread);
	}
}

/********************************************************************************/


} // end namespace


