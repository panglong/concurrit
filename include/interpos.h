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

#include "common.h"

#include <cstdatomic>

namespace concurrit {

/********************************************************************************/

class PthreadOriginals {
public:

	static void initialize();

	static inline volatile bool is_initialized() { return _initialized; }

	static int pthread_create(pthread_t * param0, const pthread_attr_t * param1, void *(* param2)(void *), void * param3);

	static int pthread_join(pthread_t param0, void ** param1);

	static void pthread_exit(void * param0);

	static int pthread_cancel(pthread_t thread);

	static int (* volatile _pthread_create) (pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);
	static int (* volatile _pthread_join) (pthread_t, void **);
	static void (* volatile _pthread_exit) (void *);
	static int (* volatile _pthread_cancel) (pthread_t);

	static volatile bool _initialized;
};

/********************************************************************************/

extern "C" int pthread_create(pthread_t* thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);
extern "C" int pthread_join(pthread_t thread, void ** value_ptr);
extern "C" void pthread_exit(void * value_ptr);
extern "C" int pthread_cancel(pthread_t thread);

/********************************************************************************/

// default implementation calls original functions

class PthreadHandler {
public:

	virtual int pthread_create(pthread_t* thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);

	virtual int pthread_join(pthread_t thread, void ** value_ptr);

	virtual void pthread_exit(void * param0);

	virtual int pthread_cancel(pthread_t thread);

	PthreadHandler() {}
	virtual ~PthreadHandler() {}

	static PthreadHandler* Current;
};

/********************************************************************************/

class ConcurritPthreadHandler : public PthreadHandler {
public:
	ConcurritPthreadHandler() : PthreadHandler() {}
	~ConcurritPthreadHandler() {}

	int pthread_create(pthread_t* thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);

	int pthread_join(pthread_t thread, void ** value_ptr);

	void pthread_exit(void * param0);

	int pthread_cancel(pthread_t thread);
};


/********************************************************************************/

} // end namespace



